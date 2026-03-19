#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "LidarMeshComponent.generated.h"

/**
 * Custom procedural mesh component for LIDAR meshes.
 * Automatically supports collision + navigation updates.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DYNAMICUAVROUTING_API ULidarMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:
    ULidarMeshComponent(const FObjectInitializer& ObjectInitializer);

    /** Rebuild physics state & force nav update */
    UFUNCTION(BlueprintCallable, Category = "Lidar|Mesh")
    void RebuildCollisionAndNav();
};