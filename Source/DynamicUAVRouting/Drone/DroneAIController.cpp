#include "DroneAIController.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ADroneAIController::ADroneAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ADroneAIController::BeginPlay()
{
    Super::BeginPlay();

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerPawn)
    {
        SetTargetPawn(PlayerPawn);
    }
}

void ADroneAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!TargetPawn) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    const FVector DroneLocation = ControlledPawn->GetActorLocation();
    const FVector TargetLocation = TargetPawn->GetActorLocation();
    const FVector TargetVelocity = TargetPawn->GetVelocity();

    FVector DesiredLocation;
    FVector LookTarget;

    // =========================
    // Moving target
    // =========================
    if (TargetVelocity.SizeSquared() > 25.f)
    {
        const FVector Predicted = TargetLocation + TargetVelocity * PredictionTime;

        const FVector Forward = TargetPawn->GetActorForwardVector();
        const FVector Right = TargetPawn->GetActorRightVector();

        const FVector Offset =
            Forward * FollowOffset.X +
            Right * FollowOffset.Y +
            FVector::UpVector * FollowOffset.Z;

        DesiredLocation = Predicted + Offset;

        // Look slightly ahead for smoother facing
        LookTarget = Predicted;
    }
    // =========================
    // Idle orbit
    // =========================
    else
    {
        OrbitAngle += OrbitSpeed * DeltaTime;

        const float CosA = FMath::Cos(OrbitAngle);
        const float SinA = FMath::Sin(OrbitAngle);

        DesiredLocation = TargetLocation + FVector(
            CosA * OrbitRadius,
            SinA * OrbitRadius,
            FollowOffset.Z
        );

        // Look slightly ahead along orbit path (prevents jitter)
        const float LookAheadAngle = OrbitAngle + 0.2f;
        LookTarget = TargetLocation + FVector(
            FMath::Cos(LookAheadAngle) * OrbitRadius,
            FMath::Sin(LookAheadAngle) * OrbitRadius,
            FollowOffset.Z
        );
    }

    // =========================
    // Smooth velocity movement
    // =========================
    FVector ToTarget = DesiredLocation - DroneLocation;
    FVector DesiredVelocity = ToTarget.GetClampedToMaxSize(MaxSpeed);

    // Accelerate toward desired velocity
    CurrentVelocity = FMath::VInterpTo(
        CurrentVelocity,
        DesiredVelocity,
        DeltaTime,
        Acceleration / MaxSpeed
    );

    // Apply damping (removes jitter)
    CurrentVelocity *= (1.0f - FMath::Clamp(Damping * DeltaTime, 0.f, 1.f));

    FVector NewLocation = DroneLocation + CurrentVelocity * DeltaTime;
    ControlledPawn->SetActorLocation(NewLocation);

    // =========================
    // Yaw-only rotation (no tilt)
    // =========================
    FVector FlatDirection = LookTarget - DroneLocation;
    FlatDirection.Z = 0.f;

    if (!FlatDirection.IsNearlyZero())
    {
        FlatDirection.Normalize();

        FRotator DesiredRot = FlatDirection.Rotation();
        FRotator CurrentRot = ControlledPawn->GetActorRotation();

        // Only yaw
        DesiredRot.Pitch = 0.f;
        DesiredRot.Roll = 0.f;

        float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, DesiredRot.Yaw));

        // Deadzone to prevent jitter
        if (YawDelta > RotationDeadzoneDeg)
        {
            FRotator NewRot = FMath::RInterpTo(
                CurrentRot,
                DesiredRot,
                DeltaTime,
                RotationInterpSpeed
            );

            NewRot.Pitch = 0.f;
            NewRot.Roll = 0.f;

            ControlledPawn->SetActorRotation(NewRot);
        }
    }
}

void ADroneAIController::SetTargetPawn(APawn* NewTarget)
{
    TargetPawn = NewTarget;
}