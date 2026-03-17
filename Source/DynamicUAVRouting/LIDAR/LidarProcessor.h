#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "LidarProcessor.generated.h"

/**
 * LidarProcessor
 * GPU point cloud using Niagara + ProceduralMeshComponent for full mesh visualization.
 */
UCLASS()
class DYNAMICUAVROUTING_API ALidarProcessor : public AActor
{
    GENERATED_BODY()

public:
    ALidarProcessor();

protected:
    virtual void BeginPlay() override;

public:
    /** Niagara system asset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    UNiagaraSystem* NiagaraSystemAsset;

    /** Spawned Niagara component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    UNiagaraComponent* NiagaraComp;

    /** Procedural mesh for aggregated points */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh")
    UProceduralMeshComponent* ProcMeshComp;

    /** Cumulative points on CPU */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    TArray<FVector> AggregatedPoints;

    /** Step size in world units for subdivision */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float StepSize = 100.f;

    /** Plane dimensions in world units - set at spawn */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float PlaneWidth = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float PlaneLength = 5000.f;

    /** Add new LIDAR points (sends only new points to Niagara) */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Clear all points */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void ClearPoints();

protected:
    /** Preallocated vertex array for procedural mesh */
    TArray<FVector> Vertices;

    /** Triangles for procedural mesh */
    TArray<int32> Triangles;

    /** Map grid cell to vertex index for fast snapping */
    TMap<FIntPoint, int32> GridVertexMap;

    /** World-space step between vertices (computed dynamically) */
    float StepX;
    float StepY;

    /** Initialize the procedural mesh plane with subdivisions */
    void InitializeProcMeshGrid();
};