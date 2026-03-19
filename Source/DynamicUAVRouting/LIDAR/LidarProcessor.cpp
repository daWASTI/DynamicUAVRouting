#include "LidarProcessor.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

ALidarProcessor::ALidarProcessor()
{
    PrimaryActorTick.bCanEverTick = false;

    ProcMeshComp = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
    RootComponent = ProcMeshComp;

    ProcMeshComp->bUseAsyncCooking = true;
    ProcMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

    StepX = PlaneWidth / (NumX - 1);
    StepY = PlaneLength / (NumY - 1);

    int32 TotalVerts = NumX * NumY;

    Vertices.Reserve(TotalVerts);
    RawVerts.Reserve(TotalVerts);

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

    ProcMeshComp->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, Tangents, false);

    if (ProcMeshMaterial)
    {
        ProcMeshComp->SetMaterial(0, ProcMeshMaterial);
    }
}

void ALidarProcessor::AddPoints(const TArray<FVector>& NewPoints)
{
    if (!NiagaraComp || NewPoints.Num() == 0) return;

    int32 NumX = FMath::CeilToInt(PlaneWidth / StepX) + 1;
    int32 NumY = FMath::CeilToInt(PlaneLength / StepY) + 1;

    TArray<FVector> SnappedPoints;
    SnappedPoints.Reserve(NewPoints.Num());

    TArray<int32> UpdatedVertices;
    UpdatedVertices.Reserve(NewPoints.Num());

    TArray<bool> Touched;
    Touched.Init(false, RawVerts.Num());

    for (const FVector& P : NewPoints)
    {
        int32 GX = FMath::Clamp(FMath::FloorToInt(P.X / StepX), 0, NumX - 1);
        int32 GY = FMath::Clamp(FMath::FloorToInt(P.Y / StepY), 0, NumY - 1);

        int32 Idx = GY * NumX + GX;

        if (!RawVerts.IsValidIndex(Idx)) continue;

        RawVerts[Idx].Z = P.Z;

        if (!Touched[Idx])
        {
            Touched[Idx] = true;
            UpdatedVertices.Add(Idx);
        }

        SnappedPoints.Add(FVector(GX * StepX, GY * StepY, P.Z));
    }

    if (UpdatedVertices.Num() == 0) return;

    SmoothVertices(UpdatedVertices);

    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComp,
        "PointPositions",
        SnappedPoints
    );

    UpdateProcMeshSection();
}

void ALidarProcessor::SmoothVertices(const TArray<int32>& UpdatedVertices)
{
    if (UpdatedVertices.Num() == 0) return;

    int32 NumX = FMath::CeilToInt(PlaneWidth / StepX) + 1;
    int32 NumY = FMath::CeilToInt(PlaneLength / StepY) + 1;

    // Deduplicate + expand affected region
    TSet<int32> AffectedVertices;
    AffectedVertices.Reserve(UpdatedVertices.Num() * (SmoothRadius * 4 + 1));

    for (int32 Idx : UpdatedVertices)
    {
        int32 x = Idx % NumX;
        int32 y = Idx / NumX;

        int32 MinX = FMath::Max(0, x - SmoothRadius);
        int32 MaxX = FMath::Min(NumX - 1, x + SmoothRadius);
        int32 MinY = FMath::Max(0, y - SmoothRadius);
        int32 MaxY = FMath::Min(NumY - 1, y + SmoothRadius);

        for (int32 iy = MinY; iy <= MaxY; iy++)
        {
            for (int32 ix = MinX; ix <= MaxX; ix++)
            {
                AffectedVertices.Add(iy * NumX + ix);
            }
        }
    }

    // Smooth each affected vertex ONCE
    for (int32 I : AffectedVertices)
    {
        int32 x = I % NumX;
        int32 y = I / NumX;

        float SumZ = 0.f;
        float WeightSum = 0.f;

        for (int32 ny = FMath::Max(0, y - SmoothRadius); ny <= FMath::Min(NumY - 1, y + SmoothRadius); ny++)
        {
            for (int32 nx = FMath::Max(0, x - SmoothRadius); nx <= FMath::Min(NumX - 1, x + SmoothRadius); nx++)
            {
                int32 NeighborIdx = ny * NumX + nx;

                // Distance-based weight (smoother result)
                float Dist = FVector2D(nx - x, ny - y).Size();
                float Weight = 1.f / (1.f + Dist); // simple falloff

                SumZ += RawVerts[NeighborIdx].Z * Weight;
                WeightSum += Weight;
            }
        }

        float AvgZ = SumZ / WeightSum;

        // Blend toward smoothed value
        Vertices[I].Z = FMath::Lerp(Vertices[I].Z, AvgZ, SmoothStrength);
    }
}

void ALidarProcessor::UpdateProcMeshSection()
{
    ProcMeshComp->UpdateMeshSection(0, Vertices, {}, {}, {}, {});
}

void ALidarProcessor::ClearPoints()
{
    Vertices.Empty();
    RawVerts.Empty();
    Triangles.Empty();

    if (ProcMeshComp)
    {
        ProcMeshComp->ClearAllMeshSections();
        InitializeProcMeshGrid();
    }
}