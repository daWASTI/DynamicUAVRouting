#include "TerrainGeneratorComponent.h"
#include "Async/AsyncWork.h"
#include "Async/Async.h"
#include "Kismet/KismetMathLibrary.h"

// -------------------- Worker Task --------------------
class FGenerateTerrainTask : public FNonAbandonableTask
{
public:
    FGenerateTerrainTask(int32 InSizeX, int32 InSizeY, float InNoiseFreq, float InMaxHeight,
        int32 InSeed, bool bInAddRiver, float InRiverDepth)
        : SizeX(InSizeX), SizeY(InSizeY), NoiseFreq(InNoiseFreq), MaxHeight(InMaxHeight),
        Seed(InSeed), bAddRiver(bInAddRiver), RiverDepth(InRiverDepth)
    {
    }

    void DoWork()
    {
        Vertices.SetNum(SizeX * SizeY);
        Normals.SetNum(SizeX * SizeY);
        UVs.SetNum(SizeX * SizeY);

        FRandomStream RNG(Seed);

        for (int32 Y = 0; Y < SizeY; ++Y)
        {
            for (int32 X = 0; X < SizeX; ++X)
            {
                float BaseNoise = FMath::PerlinNoise2D(FVector2D(X * NoiseFreq + Seed, Y * NoiseFreq + Seed));
                float DetailNoise = FMath::PerlinNoise2D(FVector2D(X * NoiseFreq * 4.f + Seed, Y * NoiseFreq * 4.f + Seed)) * 0.3f;
                float Height = (BaseNoise + DetailNoise + 1.f) * 0.5f * MaxHeight;

                Vertices[Y * SizeX + X] = FVector(X * 100.f, Y * 100.f, Height);
                UVs[Y * SizeX + X] = FVector2D((float)X / SizeX, (float)Y / SizeY);
                Normals[Y * SizeX + X] = FVector::ZeroVector;
            }
        }

        // Triangles
        for (int32 Y = 0; Y < SizeY - 1; ++Y)
        {
            for (int32 X = 0; X < SizeX - 1; ++X)
            {
                int32 BL = Y * SizeX + X;
                int32 BR = BL + 1;
                int32 TL = BL + SizeX;
                int32 TR = TL + 1;

                Triangles.Add(TL); Triangles.Add(BR); Triangles.Add(BL);
                Triangles.Add(TL); Triangles.Add(TR); Triangles.Add(BR);
            }
        }

        // River
        if (bAddRiver)
        {
            int32 CenterY = SizeY / 2;
            int32 RiverWidth = SizeY / 10;
            for (int32 Y = 0; Y < SizeY; ++Y)
            {
                for (int32 X = 0; X < SizeX; ++X)
                {
                    int32 Index = Y * SizeX + X;
                    int32 DistanceToRiver = FMath::Abs(Y - CenterY);
                    if (DistanceToRiver < RiverWidth)
                    {
                        float Factor = 1.f - (float)DistanceToRiver / (float)RiverWidth;
                        Vertices[Index].Z -= Factor * RiverDepth;
                    }
                }
            }
        }

        // Smooth normals
        for (int32 i = 0; i < Triangles.Num(); i += 3)
        {
            int32 I0 = Triangles[i];
            int32 I1 = Triangles[i + 1];
            int32 I2 = Triangles[i + 2];

            FVector Edge1 = Vertices[I1] - Vertices[I0];
            FVector Edge2 = Vertices[I2] - Vertices[I0];
            FVector FaceNormal = FVector::CrossProduct(Edge2, Edge1).GetSafeNormal();

            Normals[I0] += FaceNormal;
            Normals[I1] += FaceNormal;
            Normals[I2] += FaceNormal;
        }

        for (int32 i = 0; i < Normals.Num(); ++i)
            Normals[i].Normalize();
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FGenerateTerrainTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    // Results
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

private:
    int32 SizeX, SizeY;
    float NoiseFreq, MaxHeight, RiverDepth;
    int32 Seed;
    bool bAddRiver;
};

// -------------------- Component --------------------
UTerrainGeneratorComponent::UTerrainGeneratorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTerrainGeneratorComponent::GenerateTerrainAsync()
{
    int32 LocalSizeX = SizeX;
    int32 LocalSizeY = SizeY;
    float LocalNoiseFreq = NoiseFrequency;
    float LocalMaxHeight = MaxHeight;
    int32 LocalSeed = RandomSeed;
    bool bLocalAddRiver = bAddRiver;
    float LocalRiverDepth = RiverDepth;

    // Run worker async
    (new FAutoDeleteAsyncTask<FGenerateTerrainTask>(
        LocalSizeX, LocalSizeY, LocalNoiseFreq, LocalMaxHeight,
        LocalSeed, bLocalAddRiver, LocalRiverDepth))
        ->StartBackgroundTask();

    // Dispatch the mesh update to main thread **after worker completes**
    Async(EAsyncExecution::TaskGraphMainThread, [this, LocalSizeX, LocalSizeY, LocalNoiseFreq, LocalMaxHeight, LocalSeed, bLocalAddRiver, LocalRiverDepth]()
        {
            // This lambda now generates the terrain with the captured seed
            // Broadcast arrays to Blueprint
            FGenerateTerrainTask Worker(LocalSizeX, LocalSizeY, LocalNoiseFreq, LocalMaxHeight,
                LocalSeed, bLocalAddRiver, LocalRiverDepth);
            Worker.DoWork();
            OnTerrainGenerated.Broadcast(Worker.Vertices, Worker.Triangles, Worker.Normals, Worker.UVs);
        });
}