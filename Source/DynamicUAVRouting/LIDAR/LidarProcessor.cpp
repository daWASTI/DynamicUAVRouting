#include "LidarProcessor.h"
#include "LidarSmoothingTask.h"
#include "Async/Async.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

ALidarProcessor::ALidarProcessor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create LIDAR mesh
    LidarMeshComp = CreateDefaultSubobject<ULidarMeshComponent>(TEXT("LidarMesh"));
    RootComponent = LidarMeshComp;

    // Niagara
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NiagaraObj(
        TEXT("/Game/Core/VFX/LidarVisualization/Niagara/NS_LidarPointCloud.NS_LidarPointCloud")
    );
    if (NiagaraObj.Succeeded())
    {
        NiagaraSystemAsset = NiagaraObj.Object;
    }
}

void ALidarProcessor::BeginPlay()
{
    Super::BeginPlay();

    if (NiagaraSystemAsset)
    {
        NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            NiagaraSystemAsset,
            GetActorLocation(),
            FRotator::ZeroRotator
        );
    }

    InitializeProcMeshGrid();
}

void ALidarProcessor::InitializeProcMeshGrid()
{
    Vertices.Empty();
    RawVerts.Empty();
    Triangles.Empty();

    int32 NumX = FMath::CeilToInt(PlaneWidth / StepSize) + 1;
    int32 NumY = FMath::CeilToInt(PlaneLength / StepSize) + 1;

    float StepX = PlaneWidth / (NumX - 1);
    float StepY = PlaneLength / (NumY - 1);

    for (int32 y = 0; y < NumY; y++)
    {
        for (int32 x = 0; x < NumX; x++)
        {
            FVector V(x * StepX, y * StepY, 0.f);
            Vertices.Add(V);
            RawVerts.Add(V);

            if (x < NumX - 1 && y < NumY - 1)
            {
                int32 I0 = y * NumX + x;
                int32 I1 = I0 + 1;
                int32 I2 = I0 + NumX;
                int32 I3 = I2 + 1;
                Triangles.Append({ I0, I2, I1, I1, I2, I3 });
            }
        }
    }

    TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num());
    TArray<FVector2D> UVs; UVs.Init(FVector2D::ZeroVector, Vertices.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(), Vertices.Num());

    LidarMeshComp->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, Tangents, true);

    if (ProcMeshMaterial)
    {
        LidarMeshComp->SetMaterial(0, ProcMeshMaterial);
    }
}

void ALidarProcessor::AddPoints(const TArray<FVector>& NewPoints)
{
    if (NewPoints.Num() == 0) return;

    BufferedPointUpdates.Append(NewPoints);

    if (!bSmoothingTaskRunning)
    {
        StartBufferedSmoothingTask();
    }
}

void ALidarProcessor::StartBufferedSmoothingTask()
{
    if (BufferedPointUpdates.Num() == 0)
    {
        return;
    }

    int32 NumX = FMath::CeilToInt(PlaneWidth / StepSize) + 1;
    int32 NumY = FMath::CeilToInt(PlaneLength / StepSize) + 1;

    float StepX = PlaneWidth / (NumX - 1);
    float StepY = PlaneLength / (NumY - 1);

    TArray<FVector> PendingPoints = MoveTemp(BufferedPointUpdates);
    BufferedPointUpdates.Reset();

    TMap<int32, FVector> LatestPointByVertex;
    LatestPointByVertex.Reserve(PendingPoints.Num());

    for (const FVector& P : PendingPoints)
    {
        int32 GX = FMath::Clamp(FMath::FloorToInt(P.X / StepX), 0, NumX - 1);
        int32 GY = FMath::Clamp(FMath::FloorToInt(P.Y / StepY), 0, NumY - 1);
        int32 Idx = GY * NumX + GX;
        if (!RawVerts.IsValidIndex(Idx)) continue;

        LatestPointByVertex.Add(Idx, FVector(GX * StepX, GY * StepY, P.Z));
    }

    if (LatestPointByVertex.Num() == 0)
    {
        return;
    }

    TArray<FVector> SnappedPoints;
    SnappedPoints.Reserve(LatestPointByVertex.Num());

    TArray<int32> UpdatedVertices;
    UpdatedVertices.Reserve(LatestPointByVertex.Num());

    for (const TPair<int32, FVector>& VertexPointPair : LatestPointByVertex)
    {
        const int32 Idx = VertexPointPair.Key;
        const FVector& SnappedPoint = VertexPointPair.Value;

        if (!RawVerts.IsValidIndex(Idx))
        {
            continue;
        }

        RawVerts[Idx].Z = SnappedPoint.Z;
        UpdatedVertices.Add(Idx);
        SnappedPoints.Add(SnappedPoint);
    }

    if (UpdatedVertices.Num() == 0) return;

    const TArray<FVector> BaseVerticesSnapshot = Vertices;
    const TArray<FVector> RawVertsSnapshot = RawVerts;
    TArray<FVector> SnappedPointsSnapshot = MoveTemp(SnappedPoints);
    TArray<int32> UpdatedVerticesSnapshot = MoveTemp(UpdatedVertices);
    const int32 SnapshotNumX = NumX;
    const int32 SnapshotNumY = NumY;
    const int32 SnapshotSmoothRadius = SmoothRadius;
    const float SnapshotSmoothStrength = SmoothStrength;
    bSmoothingTaskRunning = true;

    FOnLidarSmoothingComplete CompletionDelegate;
    CompletionDelegate.BindWeakLambda(
        this,
        [this](const TArray<FVector>& SmoothedVertices, const TArray<FVector>& InSnappedPoints, const TArray<int32>& InUpdatedVertices)
        {
            HandleSmoothingComplete(SmoothedVertices, InSnappedPoints, InUpdatedVertices);
        });

    Async(EAsyncExecution::ThreadPool,
        [BaseVerticesSnapshot,
         RawVertsSnapshot,
         SnappedPointsSnapshot,
         UpdatedVerticesSnapshot,
         SnapshotNumX,
         SnapshotNumY,
         SnapshotSmoothRadius,
         SnapshotSmoothStrength,
         CompletionDelegate = MoveTemp(CompletionDelegate)]() mutable
        {
            FLidarSmoothingTask SmoothingTask(
                BaseVerticesSnapshot,
                RawVertsSnapshot,
                UpdatedVerticesSnapshot,
                SnapshotNumX,
                SnapshotNumY,
                SnapshotSmoothRadius,
                SnapshotSmoothStrength);
            SmoothingTask.DoWork();

            AsyncTask(ENamedThreads::GameThread,
                [CompletionDelegate = MoveTemp(CompletionDelegate),
                 SmoothedVertices = MoveTemp(SmoothingTask.OutputVertices),
                 SnappedPoints = MoveTemp(SnappedPointsSnapshot),
                 UpdatedVertices = MoveTemp(UpdatedVerticesSnapshot)]() mutable
                {
                    CompletionDelegate.ExecuteIfBound(SmoothedVertices, SnappedPoints, UpdatedVertices);
                });
        });
}

void ALidarProcessor::UpdateProcMeshSection()
{
    LidarMeshComp->UpdateMeshSection(0, Vertices, {}, {}, {}, {});
}

void ALidarProcessor::ClearPoints()
{
    Vertices.Empty();
    RawVerts.Empty();
    Triangles.Empty();

    if (LidarMeshComp)
    {
        LidarMeshComp->ClearAllMeshSections();
        InitializeProcMeshGrid();
    }
}

void ALidarProcessor::HandleSmoothingComplete(const TArray<FVector>& SmoothedVertices, const TArray<FVector>& SnappedPoints, const TArray<int32>& UpdatedVertices)
{
    for (int32 UpdatedVertex : UpdatedVertices)
    {
        if (Vertices.IsValidIndex(UpdatedVertex) && SmoothedVertices.IsValidIndex(UpdatedVertex))
        {
            Vertices[UpdatedVertex] = SmoothedVertices[UpdatedVertex];
        }
    }

    const int32 NumX = FMath::CeilToInt(PlaneWidth / StepSize) + 1;
    const int32 NumY = FMath::CeilToInt(PlaneLength / StepSize) + 1;
    TSet<int32> AffectedVertices;
    AffectedVertices.Reserve(UpdatedVertices.Num() * (SmoothRadius * 4 + 1));

    for (int32 Idx : UpdatedVertices)
    {
        const int32 x = Idx % NumX;
        const int32 y = Idx / NumX;
        const int32 MinX = FMath::Max(0, x - SmoothRadius);
        const int32 MaxX = FMath::Min(NumX - 1, x + SmoothRadius);
        const int32 MinY = FMath::Max(0, y - SmoothRadius);
        const int32 MaxY = FMath::Min(NumY - 1, y + SmoothRadius);

        for (int32 iy = MinY; iy <= MaxY; iy++)
        {
            for (int32 ix = MinX; ix <= MaxX; ix++)
            {
                const int32 AffectedIndex = iy * NumX + ix;
                if (Vertices.IsValidIndex(AffectedIndex) && SmoothedVertices.IsValidIndex(AffectedIndex))
                {
                    AffectedVertices.Add(AffectedIndex);
                }
            }
        }
    }

    for (int32 AffectedIndex : AffectedVertices)
    {
        Vertices[AffectedIndex] = SmoothedVertices[AffectedIndex];
    }

    if (NiagaraComp)
    {
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
            NiagaraComp,
            TEXT("PointPositions"),
            SnappedPoints);
    }

    UpdateProcMeshSection();
    bSmoothingTaskRunning = false;

    if (BufferedPointUpdates.Num() > 0)
    {
        StartBufferedSmoothingTask();
    }
}
