#include "LidarMeshComponent.h"
#include "NavigationSystem.h"

ULidarMeshComponent::ULidarMeshComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bUseAsyncCooking = true;

    // Collision setup
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetCollisionObjectType(ECC_WorldStatic);
    SetCollisionResponseToAllChannels(ECR_Block);
    SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // Ignore vehicles
    bUseComplexAsSimpleCollision = true;

    // Navigation
    SetCanEverAffectNavigation(true);
}

void ULidarMeshComponent::RebuildCollisionAndNav()
{
    // Force physics/collision rebuild
    RecreatePhysicsState();

    // Force navigation rebuild
    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        NavSys->Build();
    }
}