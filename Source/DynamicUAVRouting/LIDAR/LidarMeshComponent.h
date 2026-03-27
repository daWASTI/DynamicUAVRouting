#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "LidarMeshComponent.generated.h"

/** Procedural mesh component used as the runtime terrain surface for processed LIDAR data. */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DYNAMICUAVROUTING_API ULidarMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:
    ULidarMeshComponent(const FObjectInitializer& ObjectInitializer);

    /** Rebuilds mesh collision and requests a navigation refresh for the updated surface. */
    UFUNCTION(BlueprintCallable, Category = "Lidar|Mesh")
    void RebuildCollisionAndNav();
};
