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

    FVector DroneLocation = ControlledPawn->GetActorLocation();
    FVector TargetLocation = TargetPawn->GetActorLocation() + FVector(0, 0, 300); // hover above vehicle

    FVector Direction = (TargetLocation - DroneLocation).GetSafeNormal();
    float Distance = FVector::Dist(DroneLocation, TargetLocation);
    float Speed = 600.f; // units/sec, tune as needed

    if (Distance > 10.f) // minimal threshold to prevent jitter
    {
        ControlledPawn->AddMovementInput(Direction, Speed * DeltaTime);
    }

    // Smoothly rotate drone to face the target
    FRotator LookAt = FRotationMatrix::MakeFromX(TargetLocation - DroneLocation).Rotator();
    ControlledPawn->SetActorRotation(LookAt);
}

void ADroneAIController::SetTargetPawn(APawn* NewTarget)
{
    TargetPawn = NewTarget;
}