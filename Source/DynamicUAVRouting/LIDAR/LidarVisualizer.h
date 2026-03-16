#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "LidarVisualizer.generated.h"

/**
 * LidarVisualizer
 * GPU point cloud using Niagara; sends new points incrementally.
 */
UCLASS()
class DYNAMICUAVROUTING_API ALidarVisualizer : public AActor
{
    GENERATED_BODY()

public:
    ALidarVisualizer();

protected:
    virtual void BeginPlay() override;

public:
    /** Niagara system asset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    UNiagaraSystem* NiagaraSystemAsset;

    /** Spawned Niagara component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    UNiagaraComponent* NiagaraComp;

    /** Cumulative points on CPU */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    TArray<FVector> AggregatedPoints;

    /** Add new LIDAR points (sends only new points to Niagara) */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Clear all points */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void ClearPoints();
};