#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LidarComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DYNAMICUAVROUTING_API ULidarComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULidarComponent();

protected:
    virtual void BeginPlay() override;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    int32 NumRays = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float MaxDistance = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float GridSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    bool bDrawDebug = true;

    // Gradient control: colors and corresponding absolute heights
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<FLinearColor> GradientColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<float> GradientHeights;

    // LIDAR functions
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    TArray<FVector> LIDARSnapshot();

    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void LIDARAggregate(const TArray<FVector>& Points);

    // Aggregated terrain map (grid)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    TMap<FIntPoint, float> HeightMap;

    // Cumulative array of all aggregated hits
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    TArray<FVector> AggregatedPoints;
};