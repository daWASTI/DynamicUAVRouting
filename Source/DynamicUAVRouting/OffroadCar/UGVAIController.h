#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UGVAIController.generated.h"

class ADynamicUAVRoutingOffroadCar;

/**
 * AI Controller for UGV vehicles.
 * Simulates inputs instead of MoveTo.
 */
UCLASS()
class DYNAMICUAVROUTING_API AUGVAIController : public AAIController
{
    GENERATED_BODY()

public:
    AUGVAIController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** The controlled vehicle pawn */
    UPROPERTY()
    ADynamicUAVRoutingOffroadCar* ControlledUGV;

    /** Target location for navigation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FVector TargetLocation;

    /** Max steering input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float MaxSteerInput = 1.f;

    /** Max throttle input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float MaxThrottleInput = 1.f;

    /** Max brake input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float MaxBrakeInput = 1.f;

    /** Core AI drive function */
    void DriveTowardsTarget(float DeltaTime);

public:
    /** Set a new target location */
    UFUNCTION(BlueprintCallable)
    void SetTargetLocation(FVector NewTarget);
};