#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "LidarComponent.generated.h"

class ALidarProcessor;

/** Coordinates asynchronous LIDAR tracing and forwards completed hit frames to the processor. */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DYNAMICUAVROUTING_API ULidarComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULidarComponent();

protected:
    /** Initializes component runtime state after the owning actor enters play. */
    virtual void BeginPlay() override;
    /** Polls outstanding async trace batches and completes finished scans. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    int32 NumRays = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float MaxDistance = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    TEnumAsByte<ECollisionChannel> LidarTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Debug")
    bool bDrawDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Debug")
    float DebugPointLifetime = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<FLinearColor> GradientColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<float> GradientHeights;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    TObjectPtr<ALidarProcessor> LidarProcessor = nullptr;

    // --- Public functions ---
    /** Launches a new asynchronous LIDAR scan batch from the owning actor transform. */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void FireLidarScan();

    /** Handles a fully resolved scan frame and forwards it into the processing pipeline. */
    void OnTraceBatchComplete(const TArray<FVector>& FramePoints);

    /** Polls in-flight async trace handles and resolves completed hit batches. */
    void ProcessPendingTraces();

private:
    struct FPendingTrace
    {
        TArray<FTraceHandle> Handles;
        TArray<FVector> Hits;
    };

    TArray<FPendingTrace> PendingTraces;

    /** Maps height to the configured debug gradient color. */
    FLinearColor GetColorForHeight(float Z) const;
};
