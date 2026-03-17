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
 * Optimized: smooths only around updated points.
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

    /** Procedural mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh")
    UProceduralMeshComponent* ProcMeshComp;

    /** Cumulative smoothed points for Niagara */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    TArray<FVector> AggregatedPoints;

    /** Step size in world units */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float StepSize = 100.f;

    /** Plane dimensions in world units - set at spawn */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float PlaneWidth = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float PlaneLength = 5000.f;

    /** Smoothing parameters */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float SmoothStrength = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    int32 SmoothRadius = 1;

    /** Add new points */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Clear all points */
    UFUNCTION(BlueprintCallable, Category = "Lidar")
    void ClearPoints();

protected:
    /** All procedural mesh vertices */
    TArray<FVector> Vertices;

    /** Raw vertex Z positions for smoothing */
    TArray<FVector> RawVerts;

    /** Triangles for procedural mesh */
    TArray<int32> Triangles;

    /** Map grid coordinate to vertex index */
    TMap<FIntPoint, int32> GridVertexMap;

    /** Computed step for X/Y to match PlaneWidth/PlaneLength */
    float StepX, StepY;

    /** Initialize procedural mesh plane */
    void InitializeProcMeshGrid();

    /** Update mesh section */
    void UpdateProcMeshSection();

    /** Apply smoothing locally around updated vertices */
    void SmoothVertices(const TSet<int32>& UpdatedVertices);
};