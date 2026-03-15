#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TerrainGeneratorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTerrainGenerated,
    const TArray<FVector>&, Vertices,
    const TArray<int32>&, Triangles,
    const TArray<FVector>&, Normals,
    const TArray<FVector2D>&, UVs);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DYNAMICUAVROUTING_API UTerrainGeneratorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTerrainGeneratorComponent();

    /** Generates terrain asynchronously */
    UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
    void GenerateTerrainAsync();

    /** Fired when terrain data is ready */
    UPROPERTY(BlueprintAssignable, Category = "Procedural Terrain")
    FOnTerrainGenerated OnTerrainGenerated;

    // Terrain parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    int32 SizeX = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    int32 SizeY = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    float MaxHeight = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    float NoiseFrequency = 0.005f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    int32 RandomSeed = 1337;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    bool bAddRiver = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Terrain")
    float RiverDepth = 150.f;
};