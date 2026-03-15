#include "DynamicUAVRoutingGameMode.h"
#include "DynamicUAVRoutingPlayerController.h"
#include "Drone/DronePawn.h"
#include "Drone/DroneAIController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ADynamicUAVRoutingGameMode::ADynamicUAVRoutingGameMode()
{
	// Keep your existing PlayerController
	PlayerControllerClass = ADynamicUAVRoutingPlayerController::StaticClass();
}

void ADynamicUAVRoutingGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;
	if (!VehiclePawnClass || !DronePawnClass) return;

	// --- Spawn Vehicle ---
	FActorSpawnParameters VehicleParams;
	VehicleParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* VehiclePawn = World->SpawnActor<APawn>(VehiclePawnClass, VehicleSpawnLocation, FRotator::ZeroRotator, VehicleParams);
	if (!VehiclePawn) return;

	// Possess the vehicle with the player controller
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC && VehiclePawn)
	{
		PC->Possess(VehiclePawn);
	}

	// --- Spawn Drone ---
	FVector DroneLocation = VehicleSpawnLocation + DroneSpawnOffset;
	FActorSpawnParameters DroneParams;
	DroneParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ADronePawn* DronePawn = World->SpawnActor<ADronePawn>(DronePawnClass, DroneLocation, FRotator::ZeroRotator, DroneParams);
	if (!DronePawn) return;

	// Set AI target
	ADroneAIController* DroneAI = Cast<ADroneAIController>(DronePawn->GetController());
	if (DroneAI)
	{
		DroneAI->SetTargetPawn(VehiclePawn);
	}
}