#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LidarMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "LidarProcessor.generated.h"

DECLARE_DELEGATE_TwoParams(FOnLidarSmoothingComplete, const TArray<FVector>&, const TArray<int32>&);

/** Consumes LIDAR hit frames, updates the heightfield mesh, and drives point-cloud visualization. */
UCLASS()
class DYNAMICUAVROUTING_API ALidarProcessor : public AActor
{
    GENERATED_BODY()

public:
    ALidarProcessor();

protected:
    /** Spawns runtime visualization dependencies and initializes the procedural grid. */
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

    /** Builds the local heightfield grid used by the procedural mesh surface. */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void InitializeProcMeshGrid();

    /** Buffers a raw LIDAR hit frame for visualization and deferred mesh processing. */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void AddPoints(const TArray<FVector>& NewPoints);

    /** Pushes the latest vertex state into the procedural mesh component. */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void UpdateProcMeshSection();

    /** Resets the mesh and visualization state back to an empty grid. */
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    void ClearPoints();

private:
    /** Applies the result of an asynchronous smoothing pass back onto the live mesh state. */
    void HandleSmoothingComplete(const TArray<FVector>& SmoothedVertices, const TArray<int32>& UpdatedVertices);
    /** Starts the next asynchronous smoothing job from the buffered raw point queue. */
    void StartBufferedSmoothingTask();
    /** Uploads the latest raw hit frame to Niagara as a point-cloud visualization buffer. */
    void UpdateNiagaraPointCloud(const TArray<FVector>& RawWorldPoints);
    /** Maps a height value into the configured point-cloud color gradient. */
    FLinearColor GetColorForHeight(float Z) const;

    TArray<FVector> BufferedPointUpdates;
    bool bSmoothingTaskRunning = false;
    bool bHasDeferredNiagaraReinit = false;
};
