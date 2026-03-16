#include "LidarVisualizer.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ALidarVisualizer::ALidarVisualizer()
{
    PrimaryActorTick.bCanEverTick = false;

    // Load Niagara system from content
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NiagaraSystemObj(
        TEXT("/Game/Core/VFX/LidarVisualization/NS_LidarPointCloud.NS_LidarPointCloud")
    );

    if (NiagaraSystemObj.Succeeded())
    {
        NiagaraSystemAsset = NiagaraSystemObj.Object;
    }
}

void ALidarVisualizer::BeginPlay()
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
}

void ALidarVisualizer::AddPoints(const TArray<FVector>& NewPoints)
{
    if (!NiagaraComp || NewPoints.Num() == 0) return;

    // Store cumulatively on CPU
    AggregatedPoints.Append(NewPoints);

    // --- UE5.7: Update Niagara array properly ---
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComp,
        FName("PointPositions"), // must match user parameter in Niagara
        NewPoints
    );
}

void ALidarVisualizer::ClearPoints()
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
}