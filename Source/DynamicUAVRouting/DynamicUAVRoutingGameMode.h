#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DynamicUAVRoutingGameMode.generated.h"

class APawn;
class ADronePawn;
class ADroneAIController;


UCLASS()
class ADynamicUAVRoutingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADynamicUAVRoutingGameMode();

protected:
	virtual void BeginPlay() override;

	// Classes to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<APawn> VehiclePawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<APawn> DronePawnClass;

	// Spawn offsets
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	FVector VehicleSpawnLocation = FVector(0.f, 0.f, 100.f);

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	FVector DroneSpawnOffset = FVector(0.f, 0.f, 300.f);
};