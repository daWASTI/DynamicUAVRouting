#include "UGVAIController.h"
#include "DynamicUAVRoutingOffroadCar.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

AUGVAIController::AUGVAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUGVAIController::BeginPlay()
{
    Super::BeginPlay();

    ControlledUGV = Cast<ADynamicUAVRoutingOffroadCar>(GetPawn());
}

void AUGVAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (ControlledUGV)
    {
        DriveTowardsTarget(DeltaTime);
    }
}

void AUGVAIController::SetTargetLocation(FVector NewTarget)
{
    TargetLocation = NewTarget;
}

void AUGVAIController::DriveTowardsTarget(float DeltaTime)
{
    if (!ControlledUGV) return;

    FVector VehicleLoc = ControlledUGV->GetActorLocation();
    FVector Forward = ControlledUGV->GetActorForwardVector();

    FVector ToTarget = TargetLocation - VehicleLoc;
    ToTarget.Z = 0.f; // ignore vertical
    float Distance = ToTarget.Size();

    if (Distance < 100.f)
    {
        // Stop vehicle when near target
        ControlledUGV->GetChaosVehicleMovement()->SetThrottleInput(0.f);
        ControlledUGV->GetChaosVehicleMovement()->SetBrakeInput(MaxBrakeInput);
        ControlledUGV->GetChaosVehicleMovement()->SetSteeringInput(0.f);
        return;
    }

    ToTarget.Normalize();
    float Dot = FVector::DotProduct(Forward, ToTarget);
    float Det = FVector::CrossProduct(Forward, ToTarget).Z;

    float Steering = FMath::Clamp(Det, -MaxSteerInput, MaxSteerInput);
    float Throttle = FMath::Clamp(Dot, 0.f, MaxThrottleInput);

    ControlledUGV->GetChaosVehicleMovement()->SetSteeringInput(Steering);
    ControlledUGV->GetChaosVehicleMovement()->SetThrottleInput(Throttle);
    ControlledUGV->GetChaosVehicleMovement()->SetBrakeInput(0.f);
}