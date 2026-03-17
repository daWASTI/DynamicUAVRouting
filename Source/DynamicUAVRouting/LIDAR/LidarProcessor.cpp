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
    if (!ProcMeshComp) return;

    Vertices.Empty();
    Triangles.Empty();
    GridVertexMap.Empty();

    // Number of vertices in X/Y
    int32 NumX = FMath::Max(2, FMath::CeilToInt(PlaneWidth / StepSize) + 1);
    int32 NumY = FMath::Max(2, FMath::CeilToInt(PlaneLength / StepSize) + 1);

    // Step in world space so the plane exactly spans dimensions
    StepX = PlaneWidth / (NumX - 1);
    StepY = PlaneLength / (NumY - 1);

    for (int32 y = 0; y < NumY; y++)
    {
        for (int32 x = 0; x < NumX; x++)
        {
            Vertices.Add(FVector(x * StepX, y * StepY, 0.f));
            GridVertexMap.Add(FIntPoint(x, y), Vertices.Num() - 1);

            // Triangles for the plane
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
    for (int32 i = 0; i < Vertices.Num(); i++)
        Normals[i] = FVector::UpVector;

    ProcMeshComp->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, false);
}

void ALidarProcessor::AddPoints(const TArray<FVector>& NewPoints)
{
    if (!NiagaraComp || NewPoints.Num() == 0) return;

    TArray<FVector> SnappedNewPoints;

    for (const FVector& P : NewPoints)
    {
        int32 GridX = FMath::Clamp(FMath::FloorToInt(P.X / StepX), 0, FMath::FloorToInt(PlaneWidth / StepX));
        int32 GridY = FMath::Clamp(FMath::FloorToInt(P.Y / StepY), 0, FMath::FloorToInt(PlaneLength / StepY));

        FIntPoint Key(GridX, GridY);

        int32* VertexIndex = GridVertexMap.Find(Key);
        if (VertexIndex)
        {
            // Update Z
            Vertices[*VertexIndex].Z = P.Z;

            // Snapped point for Niagara
            SnappedNewPoints.Add(FVector(GridX * StepX, GridY * StepY, P.Z));
        }
    }

    if (SnappedNewPoints.Num() == 0) return;

    AggregatedPoints.Append(SnappedNewPoints);

    // Update Niagara
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComp,
        FName("PointPositions"),
        SnappedNewPoints
    );

    // Update procedural mesh
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