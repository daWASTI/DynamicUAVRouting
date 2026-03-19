#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LidarMeshComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
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
    /** LIDAR mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    ULidarMeshComponent* LidarMeshComp;

    /** Niagara point cloud visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UNiagaraSystem* NiagaraSystemAsset;

    UPROPERTY(VisibleAnywhere)
    UNiagaraComponent* NiagaraComp;

    /** Procedural mesh parameters */
    UPROPERTY(EditAnywhere)
    UMaterialInterface* ProcMeshMaterial;

    UPROPERTY(EditAnywhere)
    float PlaneWidth = 1000.f;

    UPROPERTY(EditAnywhere)
    float PlaneLength = 1000.f;

    UPROPERTY(EditAnywhere)
    float StepSize = 50.f;

    UPROPERTY(EditAnywhere)
    int32 SmoothRadius = 1;

    UPROPERTY(EditAnywhere)
    float SmoothStrength = 0.5f;

    /** Internal mesh storage */
    TArray<FVector> Vertices;
    TArray<FVector> RawVerts;
    TArray<int32> Triangles;

    /** Initialize procedural mesh grid */
    void InitializeProcMeshGrid();

    /** Add LIDAR points (updates mesh + collision + nav) */
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Smooth mesh vertices */
    void SmoothVertices(const TArray<int32>& UpdatedVertices);

    /** Update procedural mesh section */
    void UpdateProcMeshSection();

    /** Clear mesh */
    void ClearPoints();
};