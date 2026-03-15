#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DroneAIController.generated.h"

class APawn;

UCLASS()
class DYNAMICUAVROUTING_API ADroneAIController : public AAIController
{
    GENERATED_BODY()

public:
    ADroneAIController();

protected:

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetTargetPawn(APawn* NewTarget);

private:

    UPROPERTY()
    APawn* TargetPawn;

    UPROPERTY(EditAnywhere, Category = "AI")
    float FollowDistance = 200.f;
};