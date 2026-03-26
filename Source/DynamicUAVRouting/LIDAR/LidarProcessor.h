#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LidarMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "LidarProcessor.generated.h"

DECLARE_DELEGATE_TwoParams(FOnLidarSmoothingComplete, const TArray<FVector>&, const TArray<int32>&);

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

    /** Niagara point cloud system asset, assigned externally (e.g. in Blueprint spawn params). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar", meta = (ExposeOnSpawn = true))
    UNiagaraSystem* NiagaraSystemAsset = nullptr;

    /** Spawned Niagara point cloud visualization component. */
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<FLinearColor> GradientColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar|Gradient")
    TArray<float> GradientHeights;

    /** Internal mesh storage */
    UPROPERTY(Transient)
    TArray<FVector> Vertices;

    UPROPERTY(Transient)
    TArray<FVector> RawVerts;

    UPROPERTY(Transient)
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
    void HandleSmoothingComplete(const TArray<FVector>& SmoothedVertices, const TArray<int32>& UpdatedVertices);
    void StartBufferedSmoothingTask();
    void UpdateNiagaraPointCloud(const TSet<int32>& PointIDs);
    FLinearColor GetColorForHeight(float Z) const;

    TArray<FVector> BufferedPointUpdates;
    bool bSmoothingTaskRunning = false;
    TMap<int32, FVector> NiagaraPointPositions;
    TMap<int32, FLinearColor> NiagaraPointColors;
};
