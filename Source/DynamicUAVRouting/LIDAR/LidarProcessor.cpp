#include "LidarProcessor.h"
#include "LidarSmoothingTask.h"
#include "Async/Async.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Logging/LogMacros.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLidarProcessor, Log, All);

/** Establishes default mesh and gradient settings for runtime LIDAR processing. */
ALidarProcessor::ALidarProcessor()
{
    PrimaryActorTick.bCanEverTick = false;

    GradientHeights = { 0.f, 500.f, 1000.f, 2000.f };
    GradientColors = { FLinearColor::Blue, FLinearColor::Green, FLinearColor::Yellow, FLinearColor::Red };

    // Create LIDAR mesh
    LidarMeshComp = CreateDefaultSubobject<ULidarMeshComponent>(TEXT("LidarMesh"));
    RootComponent = LidarMeshComp;

}

/** Creates the runtime Niagara instance and initializes the procedural heightfield grid. */
void ALidarProcessor::BeginPlay()
{
    Super::BeginPlay();

    if (!NiagaraSystemAsset)
    {
        UE_LOG(LogLidarProcessor, Warning, TEXT("LidarProcessor '%s' has no NiagaraSystemAsset assigned at BeginPlay."), *GetName());
    }
    else
    {
        UE_LOG(LogLidarProcessor, Log, TEXT("LidarProcessor '%s' BeginPlay with NiagaraSystemAsset '%s'."), *GetName(), *GetNameSafe(NiagaraSystemAsset));
    }

    if (!NiagaraComp && NiagaraSystemAsset)
    {
        NiagaraComp = NewObject<UNiagaraComponent>(this, TEXT("LidarPointCloudNiagara"));
        if (NiagaraComp)
        {
            AddInstanceComponent(NiagaraComp);
            NiagaraComp->CreationMethod = EComponentCreationMethod::Instance;
            NiagaraComp->SetAsset(NiagaraSystemAsset);
            NiagaraComp->SetUsingAbsoluteLocation(false);
            NiagaraComp->SetUsingAbsoluteRotation(false);
            NiagaraComp->SetUsingAbsoluteScale(false);
            NiagaraComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
            NiagaraComp->RegisterComponent();
            UE_LOG(LogLidarProcessor, Log, TEXT("LidarProcessor '%s' created NiagaraComp '%s' with asset '%s'."), *GetName(), *GetNameSafe(NiagaraComp), *GetNameSafe(NiagaraComp->GetAsset()));
        }
        else
        {
            UE_LOG(LogLidarProcessor, Error, TEXT("LidarProcessor '%s' failed to create NiagaraComp."), *GetName());
        }
    }
    else if (NiagaraComp && NiagaraSystemAsset && NiagaraComp->GetAsset() != NiagaraSystemAsset)
    {
        NiagaraComp->SetAsset(NiagaraSystemAsset);
        UE_LOG(LogLidarProcessor, Log, TEXT("LidarProcessor '%s' updated existing NiagaraComp '%s' to asset '%s'."), *GetName(), *GetNameSafe(NiagaraComp), *GetNameSafe(NiagaraComp->GetAsset()));
    }

    if (NiagaraComp)
    {
        NiagaraComp->SetVisibility(true, true);
        NiagaraComp->SetHiddenInGame(false, true);
        NiagaraComp->SetAutoActivate(true);
        NiagaraComp->ReinitializeSystem();
        NiagaraComp->Activate(true);
        UE_LOG(LogLidarProcessor, Log, TEXT("LidarProcessor '%s' activated NiagaraComp '%s'. Asset after activation: '%s'."), *GetName(), *GetNameSafe(NiagaraComp), *GetNameSafe(NiagaraComp->GetAsset()));
    }
    else
    {
        UE_LOG(LogLidarProcessor, Warning, TEXT("LidarProcessor '%s' has no NiagaraComp after BeginPlay setup."), *GetName());
    }

    InitializeProcMeshGrid();
}

/** Allocates the local grid backing both the procedural surface and mesh-space smoothing. */
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

/** Queues a new raw hit frame for immediate visualization and deferred terrain integration. */
void ALidarProcessor::AddPoints(const TArray<FVector>& NewPoints)
{
    if (NewPoints.Num() == 0) return;

    UpdateNiagaraPointCloud(NewPoints);
    BufferedPointUpdates.Append(NewPoints);

    if (!bSmoothingTaskRunning)
    {
        StartBufferedSmoothingTask();
    }
}

/** Collapses buffered updates into the latest terrain changes and dispatches async smoothing. */
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

/** Commits the current vertex buffer to the procedural mesh component. */
void ALidarProcessor::UpdateProcMeshSection()
{
    LidarMeshComp->UpdateMeshSection(0, Vertices, {}, {}, {}, {});
}

/** Clears runtime mesh state and rebuilds the processor back to an empty baseline grid. */
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

/** Merges an async smoothing result into the live mesh and continues any queued work. */
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

    UpdateProcMeshSection();
    bSmoothingTaskRunning = false;

    if (BufferedPointUpdates.Num() > 0)
    {
        StartBufferedSmoothingTask();
    }
}

/** Publishes the current raw hit frame to Niagara as a local-space point cloud. */
void ALidarProcessor::UpdateNiagaraPointCloud(const TArray<FVector>& RawWorldPoints)
{
    if (!NiagaraComp)
    {
        return;
    }

    TArray<int32> NiagaraIDs;
    NiagaraIDs.Reserve(RawWorldPoints.Num());

    TArray<FVector> PointPositions;
    PointPositions.Reserve(RawWorldPoints.Num());

    TArray<FLinearColor> PointColors;
    PointColors.Reserve(RawWorldPoints.Num());

    const FTransform MeshTransform = LidarMeshComp ? LidarMeshComp->GetComponentTransform() : FTransform::Identity;

    for (int32 PointIndex = 0; PointIndex < RawWorldPoints.Num(); ++PointIndex)
    {
        const FVector& WorldPoint = RawWorldPoints[PointIndex];
        NiagaraIDs.Add(PointIndex);
        PointPositions.Add(MeshTransform.InverseTransformPosition(WorldPoint));
        PointColors.Add(GetColorForHeight(WorldPoint.Z));
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

    if (!bHasDeferredNiagaraReinit && RawWorldPoints.Num() > 0 && GetWorld())
    {
        bHasDeferredNiagaraReinit = true;
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(
            this,
            [this]()
            {
                if (NiagaraComp)
                {
                    NiagaraComp->ReinitializeSystem();
                    NiagaraComp->Activate(true);
                }
            }));
    }
}

/** Evaluates the configured visualization gradient for a given world-space height. */
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
