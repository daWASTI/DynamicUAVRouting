#include "LidarProcessor.h"
#include "LidarSmoothingTask.h"
#include "Async/Async.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NavigationSystem.h"

ALidarProcessor::ALidarProcessor()
{
    PrimaryActorTick.bCanEverTick = false;

    GradientHeights = { 0.f, 500.f, 1000.f, 2000.f };
    GradientColors = { FLinearColor::Blue, FLinearColor::Green, FLinearColor::Yellow, FLinearColor::Red };

    // Create LIDAR mesh
    LidarMeshComp = CreateDefaultSubobject<ULidarMeshComponent>(TEXT("LidarMesh"));
    RootComponent = LidarMeshComp;

}

void ALidarProcessor::BeginPlay()
{
    Super::BeginPlay();

    if (!NiagaraComp && NiagaraSystemAsset)
    {
        NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
            NiagaraSystemAsset,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            false);
    }

    InitializeProcMeshGrid();
}

void ALidarProcessor::InitializeProcMeshGrid()
{
    Vertices.Empty();
    RawVerts.Empty();
    Triangles.Empty();
    NiagaraPointPositions.Empty();
    NiagaraPointColors.Empty();

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

    if (NiagaraComp)
    {
        TArray<int32> EmptyIDs;
        TArray<FVector> EmptyPositions;
        TArray<FLinearColor> EmptyColors;
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NiagaraComp, TEXT("PointIDs"), EmptyIDs);
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComp, TEXT("PointPositions"), EmptyPositions);
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(NiagaraComp, TEXT("PointColors"), EmptyColors);
        NiagaraComp->SetVariableInt(TEXT("PointCount"), 0);
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

    TMap<int32, FVector> LowestPointByVertex;
    LowestPointByVertex.Reserve(PendingPoints.Num());

    const FTransform MeshTransform = LidarMeshComp ? LidarMeshComp->GetComponentTransform() : FTransform::Identity;

    for (const FVector& P : PendingPoints)
    {
        const FVector LocalPoint = MeshTransform.InverseTransformPosition(P);

        int32 GX = FMath::Clamp(FMath::FloorToInt(LocalPoint.X / StepX), 0, NumX - 1);
        int32 GY = FMath::Clamp(FMath::FloorToInt(LocalPoint.Y / StepY), 0, NumY - 1);
        int32 Idx = GY * NumX + GX;
        if (!RawVerts.IsValidIndex(Idx)) continue;

        const FVector SnappedPoint(GX * StepX, GY * StepY, LocalPoint.Z);
        if (FVector* ExistingPoint = LowestPointByVertex.Find(Idx))
        {
            if (SnappedPoint.Z < ExistingPoint->Z)
            {
                *ExistingPoint = SnappedPoint;
            }
        }
        else
        {
            LowestPointByVertex.Add(Idx, SnappedPoint);
        }
    }

    if (LowestPointByVertex.Num() == 0)
    {
        return;
    }

    TArray<int32> UpdatedVertices;
    UpdatedVertices.Reserve(LowestPointByVertex.Num());

    for (const TPair<int32, FVector>& VertexPointPair : LowestPointByVertex)
    {
        const int32 Idx = VertexPointPair.Key;
        const FVector& SnappedPoint = VertexPointPair.Value;

        if (!RawVerts.IsValidIndex(Idx))
        {
            continue;
        }

        RawVerts[Idx].Z = SnappedPoint.Z;
        UpdatedVertices.Add(Idx);
    }

    if (UpdatedVertices.Num() == 0) return;

    const TArray<FVector> BaseVerticesSnapshot = Vertices;
    const TArray<FVector> RawVertsSnapshot = RawVerts;
    TArray<int32> UpdatedVerticesSnapshot = MoveTemp(UpdatedVertices);
    const int32 SnapshotNumX = NumX;
    const int32 SnapshotNumY = NumY;
    const int32 SnapshotSmoothRadius = SmoothRadius;
    const float SnapshotSmoothStrength = SmoothStrength;
    bSmoothingTaskRunning = true;

    FOnLidarSmoothingComplete CompletionDelegate;
    CompletionDelegate.BindWeakLambda(
        this,
        [this](const TArray<FVector>& SmoothedVertices, const TArray<int32>& InUpdatedVertices)
        {
            HandleSmoothingComplete(SmoothedVertices, InUpdatedVertices);
        });

    Async(EAsyncExecution::ThreadPool,
        [BaseVerticesSnapshot,
         RawVertsSnapshot,
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
                 UpdatedVertices = MoveTemp(UpdatedVerticesSnapshot)]() mutable
                {
                    CompletionDelegate.ExecuteIfBound(SmoothedVertices, UpdatedVertices);
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
    NiagaraPointPositions.Empty();
    NiagaraPointColors.Empty();

    if (LidarMeshComp)
    {
        LidarMeshComp->ClearAllMeshSections();
        InitializeProcMeshGrid();
    }
}

void ALidarProcessor::HandleSmoothingComplete(const TArray<FVector>& SmoothedVertices, const TArray<int32>& UpdatedVertices)
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

    UpdateNiagaraPointCloud(AffectedVertices);

    UpdateProcMeshSection();
    bSmoothingTaskRunning = false;

    if (BufferedPointUpdates.Num() > 0)
    {
        StartBufferedSmoothingTask();
    }
}

void ALidarProcessor::UpdateNiagaraPointCloud(const TSet<int32>& PointIDs)
{
    if (!NiagaraComp)
    {
        return;
    }

    for (int32 PointID : PointIDs)
    {
        if (!Vertices.IsValidIndex(PointID))
        {
            NiagaraPointPositions.Remove(PointID);
            NiagaraPointColors.Remove(PointID);
            continue;
        }

        NiagaraPointPositions.Add(PointID, Vertices[PointID]);
        NiagaraPointColors.Add(PointID, GetColorForHeight(Vertices[PointID].Z));
    }

    TArray<int32> NiagaraIDs;
    NiagaraIDs.Reserve(NiagaraPointPositions.Num());
    NiagaraPointPositions.GetKeys(NiagaraIDs);
    NiagaraIDs.Sort();

    TArray<FVector> PointPositions;
    PointPositions.Reserve(NiagaraIDs.Num());

    TArray<FLinearColor> PointColors;
    PointColors.Reserve(NiagaraIDs.Num());

    for (int32 NiagaraID : NiagaraIDs)
    {
        if (const FVector* Position = NiagaraPointPositions.Find(NiagaraID))
        {
            PointPositions.Add(*Position);
            PointColors.Add(NiagaraPointColors.FindRef(NiagaraID));
        }
    }

    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(
        NiagaraComp,
        TEXT("PointIDs"),
        NiagaraIDs);
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComp,
        TEXT("PointPositions"),
        PointPositions);
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
        NiagaraComp,
        TEXT("PointColors"),
        PointColors);
    NiagaraComp->SetVariableInt(TEXT("PointCount"), NiagaraIDs.Num());
}

FLinearColor ALidarProcessor::GetColorForHeight(float Z) const
{
    const int32 NumStops = FMath::Min(GradientColors.Num(), GradientHeights.Num());
    if (NumStops == 0)
    {
        return FLinearColor::White;
    }

    if (Z <= GradientHeights[0])
    {
        return GradientColors[0];
    }

    if (Z >= GradientHeights[NumStops - 1])
    {
        return GradientColors[NumStops - 1];
    }

    for (int32 i = 0; i < NumStops - 1; ++i)
    {
        if (Z >= GradientHeights[i] && Z <= GradientHeights[i + 1])
        {
            const float Alpha = (Z - GradientHeights[i]) / (GradientHeights[i + 1] - GradientHeights[i]);
            return FLinearColor::LerpUsingHSV(GradientColors[i], GradientColors[i + 1], Alpha);
        }
    }

    return GradientColors[NumStops - 1];
}
