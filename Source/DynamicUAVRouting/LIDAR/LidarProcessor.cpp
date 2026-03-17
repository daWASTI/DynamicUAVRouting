#include "LidarProcessor.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ALidarProcessor::ALidarProcessor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Niagara system
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NiagaraSystemObj(
        TEXT("/Game/Core/VFX/LidarVisualization/NS_LidarPointCloud.NS_LidarPointCloud")
    );
    if (NiagaraSystemObj.Succeeded())
    {
        NiagaraSystemAsset = NiagaraSystemObj.Object;
    }

    // Procedural mesh
    ProcMeshComp = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMeshComp"));
    ProcMeshComp->SetupAttachment(RootComponent);
    ProcMeshComp->bUseAsyncCooking = true;

    ProcMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProcMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void ALidarProcessor::BeginPlay()
{
    Super::BeginPlay();

    // Spawn Niagara system
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
    GridVertexMap.Empty();

    int32 NumX = FMath::Max(2, FMath::CeilToInt(PlaneWidth / StepSize) + 1);
    int32 NumY = FMath::Max(2, FMath::CeilToInt(PlaneLength / StepSize) + 1);

    StepX = PlaneWidth / (NumX - 1);
    StepY = PlaneLength / (NumY - 1);

    for (int32 y = 0; y < NumY; y++)
    {
        for (int32 x = 0; x < NumX; x++)
        {
            FVector V(x * StepX, y * StepY, 0.f);
            Vertices.Add(V);
            RawVerts.Add(V);
            GridVertexMap.Add(FIntPoint(x, y), Vertices.Num() - 1);

            // Plane triangles
            if (x < NumX - 1 && y < NumY - 1)
            {
                int32 I0 = y * NumX + x;
                int32 I1 = y * NumX + (x + 1);
                int32 I2 = (y + 1) * NumX + x;
                int32 I3 = (y + 1) * NumX + (x + 1);
                Triangles.Append({ I0, I2, I1, I1, I2, I3 });
            }
        }
    }

    TArray<FVector> Normals; Normals.SetNum(Vertices.Num());
    TArray<FVector2D> UVs; UVs.SetNum(Vertices.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.SetNum(Vertices.Num());
    for (int32 i = 0; i < Vertices.Num(); i++) Normals[i] = FVector::UpVector;

    ProcMeshComp->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, false);
}

void ALidarProcessor::AddPoints(const TArray<FVector>& NewPoints)
{
    if (!NiagaraComp || NewPoints.Num() == 0) return;

    TArray<FVector> SnappedNewPoints;
    TSet<int32> UpdatedVertexIndices;

    // Snap points to grid and update RawVerts
    for (const FVector& P : NewPoints)
    {
        int32 GridX = FMath::Clamp(FMath::FloorToInt(P.X / StepX), 0, FMath::FloorToInt(PlaneWidth / StepX));
        int32 GridY = FMath::Clamp(FMath::FloorToInt(P.Y / StepY), 0, FMath::FloorToInt(PlaneLength / StepY));

        FIntPoint Key(GridX, GridY);
        int32* VertexIndex = GridVertexMap.Find(Key);
        if (VertexIndex)
        {
            RawVerts[*VertexIndex].Z = P.Z;
            UpdatedVertexIndices.Add(*VertexIndex);

            // Snapped for Niagara
            SnappedNewPoints.Add(FVector(GridX * StepX, GridY * StepY, P.Z));
        }
    }

    if (SnappedNewPoints.Num() == 0) return;

    AggregatedPoints.Append(SnappedNewPoints);

    // Smooth only updated vertices + neighbors
    SmoothVertices(UpdatedVertexIndices);

    // Update Niagara
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComp,
        FName("PointPositions"),
        SnappedNewPoints
    );

    // Update procedural mesh
    UpdateProcMeshSection();
}

void ALidarProcessor::SmoothVertices(const TSet<int32>& UpdatedVertices)
{
    if (UpdatedVertices.Num() == 0) return;

    int32 NumX = FMath::CeilToInt(PlaneWidth / StepX) + 1;
    int32 NumY = FMath::CeilToInt(PlaneLength / StepY) + 1;

    for (int32 Index : UpdatedVertices)
    {
        // compute grid coords
        int32 x = Index % NumX;
        int32 y = Index / NumX;

        int32 MinX = FMath::Max(0, x - SmoothRadius);
        int32 MaxX = FMath::Min(NumX - 1, x + SmoothRadius);
        int32 MinY = FMath::Max(0, y - SmoothRadius);
        int32 MaxY = FMath::Min(NumY - 1, y + SmoothRadius);

        for (int32 iy = MinY; iy <= MaxY; iy++)
        {
            for (int32 ix = MinX; ix <= MaxX; ix++)
            {
                int32 NeighborIndex = iy * NumX + ix;
                float SumZ = 0.f;
                int32 Count = 0;

                // compute neighbor average
                for (int32 ny = FMath::Max(0, iy - SmoothRadius); ny <= FMath::Min(NumY - 1, iy + SmoothRadius); ny++)
                {
                    for (int32 nx = FMath::Max(0, ix - SmoothRadius); nx <= FMath::Min(NumX - 1, ix + SmoothRadius); nx++)
                    {
                        int32 NIndex = ny * NumX + nx;
                        SumZ += RawVerts[NIndex].Z;
                        Count++;
                    }
                }

                float AvgZ = SumZ / Count;
                Vertices[NeighborIndex].Z = FMath::Lerp(Vertices[NeighborIndex].Z, AvgZ, SmoothStrength);
            }
        }
    }
}

void ALidarProcessor::UpdateProcMeshSection()
{
    if (!ProcMeshComp) return;

    ProcMeshComp->UpdateMeshSection(0, Vertices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>());
}

void ALidarProcessor::ClearPoints()
{
    AggregatedPoints.Empty();

    if (NiagaraComp)
    {
        TArray<FVector> EmptyArray;
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
            NiagaraComp,
            FName("PointPositions"),
            EmptyArray
        );
    }

    if (ProcMeshComp)
    {
        ProcMeshComp->ClearAllMeshSections();
        InitializeProcMeshGrid();
    }
}