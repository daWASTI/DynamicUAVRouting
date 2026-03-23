#include "HISMDestructible.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AHISMDestructible::AHISMDestructible()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create HISM component
	HISMComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMComponent"));
	SetRootComponent(HISMComponent);

	// Load default cube mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BrickMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BrickMeshAsset.Succeeded())
	{
		BrickMesh = BrickMeshAsset.Object;
		HISMComponent->SetStaticMesh(BrickMesh);
	}
}

void AHISMDestructible::BeginPlay()
{
	Super::BeginPlay();

	InitWall();
	InitStaticMeshPool();
}

void AHISMDestructible::InitWall()
{
	if (!BrickMesh) return;

	HISMComponent->ClearInstances();

	FVector BrickExtents = BrickMesh->GetBoundingBox().GetExtent() * BrickSize;

	for (int32 X = 0; X < BricksX; X++)
	{
		for (int32 Y = 0; Y < BricksY; Y++)
		{
			FVector Location = FVector(
				X * BrickExtents.X * 2,
				Y * BrickExtents.Y * 2,
				0.f
			);

			FTransform InstanceTransform(FRotator::ZeroRotator, Location, BrickSize);
			HISMComponent->AddInstance(InstanceTransform);
		}
	}
}

void AHISMDestructible::InitStaticMeshPool()
{
	if (!BrickMesh) return;

	const int32 TotalCount = BricksX * BricksY;

	StaticMeshPool.Empty();
	FreeIndices.Empty();
	UsedMask.Empty();

	StaticMeshPool.Reserve(TotalCount);
	FreeIndices.Reserve(TotalCount);
	UsedMask.Init(false, TotalCount);

	for (int32 i = 0; i < TotalCount; i++)
	{
		UStaticMeshComponent* SMComp = NewObject<UStaticMeshComponent>(this);

		SMComp->RegisterComponent();
		SMComp->SetStaticMesh(BrickMesh);
		SMComp->SetSimulatePhysics(false);

		if (!DebugMode)
		{
			SMComp->SetVisibility(false);
		}

		SMComp->SetWorldLocation(PoolOffset);
		SMComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

		StaticMeshPool.Add(SMComp);

		// Add to free list
		FreeIndices.Add(i);
	}
}

//
// --------- Pool Logic ---------
//

int32 AHISMDestructible::GetFreeIndex()
{
	if (FreeIndices.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 Index = FreeIndices.Pop();
	UsedMask[Index] = true;

	return Index;
}

void AHISMDestructible::ReleaseIndex(int32 Index)
{
	if (!UsedMask.IsValidIndex(Index))
	{
		return;
	}

	UsedMask[Index] = false;
	FreeIndices.Add(Index);

	// Reset component state
	if (StaticMeshPool.IsValidIndex(Index))
	{
		UStaticMeshComponent* SMComp = StaticMeshPool[Index];
		if (SMComp)
		{
			SMComp->SetSimulatePhysics(false);
			SMComp->SetWorldLocation(PoolOffset);

			if (!DebugMode)
			{
				SMComp->SetVisibility(false);
			}
		}
	}
}

void AHISMDestructible::GetFreeIndices(int32 Count, TArray<int32>& OutIndices)
{
	OutIndices.Reset();
	OutIndices.Reserve(Count);

	int32 NumToGive = FMath::Min(Count, FreeIndices.Num());

	for (int32 i = 0; i < NumToGive; i++)
	{
		int32 Index = FreeIndices.Pop();
		UsedMask[Index] = true;
		OutIndices.Add(Index);
	}
}

void AHISMDestructible::ReleaseIndices(const TArray<int32>& Indices)
{
	for (int32 Index : Indices)
	{
		ReleaseIndex(Index);
	}
}

void AHISMDestructible::ActivateBrick(int32 InstanceIndex)
{
	if (!HISMComponent) return;
	if (!HISMComponent->IsValidInstance(InstanceIndex)) return;

	int32 PoolIndex = GetFreeIndex();
	if (PoolIndex == INDEX_NONE) return;

	UStaticMeshComponent* SMComp = StaticMeshPool[PoolIndex];
	if (!SMComp) return;

	// Get HISM transform
	FTransform HISMTransform;
	HISMComponent->GetInstanceTransform(InstanceIndex, HISMTransform, true);

	// Get SM transform
	FTransform SMTransform = SMComp->GetComponentTransform();

	// --- Swap location & rotation ONLY ---
	FVector HISM_Location = HISMTransform.GetLocation();
	FRotator HISM_Rotation = HISMTransform.Rotator();

	FVector SM_Location = SMTransform.GetLocation();
	FRotator SM_Rotation = SMTransform.Rotator();

	// Apply to SM (copy scale from HISM!)
	SMComp->SetWorldLocationAndRotation(HISM_Location, HISM_Rotation);
	SMComp->SetWorldScale3D(HISMTransform.GetScale3D());

	// Apply to HISM instance (keep its scale!)
	HISMTransform.SetLocation(SM_Location);
	HISMTransform.SetRotation(SM_Rotation.Quaternion());

	HISMComponent->UpdateInstanceTransform(InstanceIndex, HISMTransform, true, true, true);

	// Activate physics
	SMComp->SetVisibility(true);
	SMComp->SetSimulatePhysics(true);
}

void AHISMDestructible::ActivateBricks(const TArray<int32>& InstanceIndices)
{
	if (!HISMComponent) return;
	if (InstanceIndices.Num() == 0) return;

	TArray<int32> PoolIndices;
	GetFreeIndices(InstanceIndices.Num(), PoolIndices);

	int32 Count = FMath::Min(InstanceIndices.Num(), PoolIndices.Num());

	for (int32 i = 0; i < Count; i++)
	{
		int32 InstanceIndex = InstanceIndices[i];
		int32 PoolIndex = PoolIndices[i];

		if (!HISMComponent->IsValidInstance(InstanceIndex)) continue;
		if (!StaticMeshPool.IsValidIndex(PoolIndex)) continue;

		UStaticMeshComponent* SMComp = StaticMeshPool[PoolIndex];
		if (!SMComp) continue;

		// Get transforms
		FTransform HISMTransform;
		HISMComponent->GetInstanceTransform(InstanceIndex, HISMTransform, true);

		FTransform SMTransform = SMComp->GetComponentTransform();

		// Swap location + rotation
		FVector HISM_Location = HISMTransform.GetLocation();
		FRotator HISM_Rotation = HISMTransform.Rotator();

		FVector SM_Location = SMTransform.GetLocation();
		FRotator SM_Rotation = SMTransform.Rotator();

		// Apply to SM
		SMComp->SetWorldLocationAndRotation(HISM_Location, HISM_Rotation);
		SMComp->SetWorldScale3D(HISMTransform.GetScale3D());

		// Apply to HISM
		HISMTransform.SetLocation(SM_Location);
		HISMTransform.SetRotation(SM_Rotation.Quaternion());

		HISMComponent->UpdateInstanceTransform(InstanceIndex, HISMTransform, false, false, true);

		// Activate
		SMComp->SetVisibility(true);
		SMComp->SetSimulatePhysics(true);
	}

	// Only mark render state dirty once (big optimization)
	HISMComponent->MarkRenderStateDirty();
}