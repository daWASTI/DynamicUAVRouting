#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "LidarComponent.generated.h"

class ALidarProcessor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DYNAMICUAVROUTING_API ULidarComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULidarComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    int32 NumRays = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float MaxDistance = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    TEnumAsByte<ECollisionChannel> LidarTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Debug")
    bool bDrawDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Debug")
    float DebugPointLifetime = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<FLinearColor> GradientColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<float> GradientHeights;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    ALidarProcessor* LidarProcessor = nullptr;

    // --- Public functions ---
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void FireLidarScan();

    // Called when all traces in a batch complete
    void OnTraceBatchComplete(const TArray<FVector>& FramePoints);

    // Tick polling for trace results
    void ProcessPendingTraces();

private:
    struct FPendingTrace
    {
        TArray<FTraceHandle> Handles;
        TArray<FVector> Hits;
    };

    TArray<FPendingTrace> PendingTraces;

    FLinearColor GetColorForHeight(float Z) const;
};
