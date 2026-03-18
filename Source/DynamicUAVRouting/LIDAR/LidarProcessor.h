#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"
#include "LidarProcessor.generated.h"

UCLASS()
class DYNAMICUAVROUTING_API ALidarProcessor : public AActor
{
    GENERATED_BODY()

public:
    ALidarProcessor();

protected:
    virtual void BeginPlay() override;

public:
    /** Niagara system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    UNiagaraSystem* NiagaraSystemAsset;

    /** Procedural mesh material */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    UMaterialInterface* ProcMeshMaterial;

    /** Niagara component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    UNiagaraComponent* NiagaraComp;

    /** Procedural mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh")
    UProceduralMeshComponent* ProcMeshComp;

    /** Grid resolution */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar")
    float StepSize = 100.f;

    /** Plane dimensions */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float PlaneWidth = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float PlaneLength = 5000.f;

    /** Smoothing */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    float SmoothStrength = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lidar|Mesh", meta = (ExposeOnSpawn = "true"))
    int32 SmoothRadius = 1;

    /** Add new points */
    UFUNCTION(BlueprintCallable)
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Clear all points */
    UFUNCTION(BlueprintCallable)
    void ClearPoints();

protected:
    /** Mesh data */
    TArray<FVector> Vertices;
    TArray<FVector> RawVerts;
    TArray<int32> Triangles;

    /** Grid spacing */
    float StepX, StepY;

    /** Init procedural mesh */
    void InitializeProcMeshGrid();

    /** Update mesh section */
    void UpdateProcMeshSection();

    /** Smooth only around updated vertices */
    void SmoothVertices(const TArray<int32>& UpdatedVertices);
};