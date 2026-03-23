#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HISMDestructible.generated.h"

UCLASS()
class DYNAMICUAVROUTING_API AHISMDestructible : public AActor
{
	GENERATED_BODY()

public:
	AHISMDestructible();

protected:
	virtual void BeginPlay() override;

public:
	// Dimensions of the wall in bricks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	int32 BricksX = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	int32 BricksY = 5;

	// HISM component for instanced bricks
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destruction")
	UHierarchicalInstancedStaticMeshComponent* HISMComponent;

	// Pool of Static Mesh Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pool")
	TArray<UStaticMeshComponent*> StaticMeshPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	FVector PoolOffset = FVector(0.f, 0.f, -1000.f);

	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ExposeOnSpawn = true))
	bool DebugMode = false;

	// Functions
	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void InitWall();

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void InitStaticMeshPool();

	// --- Pool API ---

	// Single
	int32 GetFreeIndex();
	void ReleaseIndex(int32 Index);

	// Batch
	void GetFreeIndices(int32 Count, TArray<int32>& OutIndices);
	void ReleaseIndices(const TArray<int32>& Indices);

	// Mesh scaling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	FVector BrickSize = FVector(1.f, 1.f, 1.f);

	// Mesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destruction")
	UStaticMesh* BrickMesh;

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void ActivateBrick(int32 InstanceIndex);

	UFUNCTION(BlueprintCallable, Category = "Destruction")
	void ActivateBricks(const TArray<int32>& InstanceIndices);

protected:
	// Free list (core allocator)
	TArray<int32> FreeIndices;

	// Optional debug / validation
	TBitArray<> UsedMask;
};