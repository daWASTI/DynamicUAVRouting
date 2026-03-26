#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LidarMeshComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "LidarProcessor.generated.h"

DECLARE_DELEGATE_ThreeParams(FOnLidarSmoothingComplete, const TArray<FVector>&, const TArray<FVector>&, const TArray<int32>&);

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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    ULidarMeshComponent* LidarMeshComp;

    /** Niagara point cloud visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar", meta = (ExposeOnSpawn = true))
    UNiagaraSystem* NiagaraSystemAsset;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar")
    UNiagaraComponent* NiagaraComp;

    /** Procedural mesh parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ExposeOnSpawn = true))
    UMaterialInterface* ProcMeshMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ExposeOnSpawn = true))
    float PlaneWidth = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ExposeOnSpawn = true))
    float PlaneLength = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ExposeOnSpawn = true))
    float StepSize = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ExposeOnSpawn = true))
    int32 SmoothRadius = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ExposeOnSpawn = true))
    float SmoothStrength = 0.5f;

    /** Internal mesh storage */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TArray<FVector> Vertices;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TArray<FVector> RawVerts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TArray<int32> Triangles;

    /** Initialize procedural mesh grid */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void InitializeProcMeshGrid();

    /** Add LIDAR points (updates mesh + collision + nav) */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Update procedural mesh section */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void UpdateProcMeshSection();

    /** Clear mesh */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void ClearPoints();

private:
    void HandleSmoothingComplete(const TArray<FVector>& SmoothedVertices, const TArray<FVector>& SnappedPoints, const TArray<int32>& UpdatedVertices);
    void StartBufferedSmoothingTask();

    TArray<FVector> BufferedPointUpdates;
    bool bSmoothingTaskRunning = false;
};
