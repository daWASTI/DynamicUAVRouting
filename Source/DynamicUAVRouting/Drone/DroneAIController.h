#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DroneAIController.generated.h"

class APawn;

UCLASS(Blueprintable)
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

    UFUNCTION(BlueprintPure, Category = "AI")
    APawn* GetTargetPawn() const { return TargetPawn; }

protected:

    // Target reference
    UPROPERTY(BlueprintReadOnly, Category = "AI|Target")
    APawn* TargetPawn;

    // Offset in target local space (Forward, Right, Up)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Follow")
    FVector FollowOffset = FVector(400.f, 0.f, 300.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Follow")
    float PredictionTime = 0.8f;

    // Movement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
    float MaxSpeed = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
    float Acceleration = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
    float Damping = 2.0f;

    // Rotation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Rotation")
    float RotationInterpSpeed = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Rotation")
    float RotationDeadzoneDeg = 2.0f;

    // Orbit
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Orbit")
    float OrbitRadius = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Orbit")
    float OrbitSpeed = 1.0f;

    UPROPERTY(BlueprintReadWrite, Category = "AI|Orbit")
    float OrbitAngle = 0.f;

    // Runtime velocity (read-only in BP)
    UPROPERTY(BlueprintReadOnly, Category = "AI|Movement")
    FVector CurrentVelocity = FVector::ZeroVector;
};