#pragma once
#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

/**
 * FAsyncTask-compatible Lidar smoothing task
 * Runs on a background thread to smooth point cloud Z-values
 */
class FLidarSmoothingTask : public FNonAbandonableTask
{
public:
    // --- Inputs ---
    TArray<FVector> BaseVertices;
    TArray<FVector> RawInputPoints;
    TArray<int32> UpdatedVertices;
    int32 NumX;
    int32 NumY;
    int32 SmoothRadius;
    float SmoothStrength;

    // --- Outputs ---
    TArray<FVector> OutputVertices;

    // Constructor
    FLidarSmoothingTask(const TArray<FVector>& InBaseVertices,
        const TArray<FVector>& InRawInputPoints,
        const TArray<int32>& InUpdatedVertices,
        int32 InNumX, int32 InNumY,
        int32 InRadius = 1, float InStrength = 0.5f)
        : BaseVertices(InBaseVertices)
        , RawInputPoints(InRawInputPoints)
        , UpdatedVertices(InUpdatedVertices)
        , NumX(InNumX)
        , NumY(InNumY)
        , SmoothRadius(InRadius)
        , SmoothStrength(InStrength)
    {
        OutputVertices = BaseVertices;
    }

    // Main work function, runs on worker thread
    void DoWork()
    {
        if (BaseVertices.Num() == 0 || RawInputPoints.Num() == 0 || UpdatedVertices.Num() == 0) return;

        TSet<int32> AffectedVertices;
        AffectedVertices.Reserve(UpdatedVertices.Num() * (SmoothRadius * 4 + 1));

        for (int32 Idx : UpdatedVertices)
        {
            const int32 x = Idx % NumX;
            const int32 y = Idx / NumX;

            const int32 MinX = FMath::Max(0, x - SmoothRadius);
            const int32 MaxX = FMath::Min(NumX - 1, x + SmoothRadius);
            const int32 MinY = FMath::Max(0, y - SmoothRadius);
            const int32 MaxY = FMath::Min(NumY - 1, y + SmoothRadius);

            for (int32 iy = MinY; iy <= MaxY; iy++)
            {
                for (int32 ix = MinX; ix <= MaxX; ix++)
                {
                    AffectedVertices.Add(iy * NumX + ix);
                }
            }
        }

        for (int32 I : AffectedVertices)
        {
            const int32 x = I % NumX;
            const int32 y = I / NumX;

            float SumZ = 0.f;
            float WeightSum = 0.f;

            const int32 MinX = FMath::Max(0, x - SmoothRadius);
            const int32 MaxX = FMath::Min(NumX - 1, x + SmoothRadius);
            const int32 MinY = FMath::Max(0, y - SmoothRadius);
            const int32 MaxY = FMath::Min(NumY - 1, y + SmoothRadius);

            for (int32 ny = MinY; ny <= MaxY; ny++)
            {
                for (int32 nx = MinX; nx <= MaxX; nx++)
                {
                    const int32 NeighborIdx = ny * NumX + nx;
                    if (!RawInputPoints.IsValidIndex(NeighborIdx) || !OutputVertices.IsValidIndex(I)) continue;

                    const float Dist = FVector2D(nx - x, ny - y).Size();
                    const float Weight = 1.f / (1.f + Dist);
                    SumZ += RawInputPoints[NeighborIdx].Z * Weight;
                    WeightSum += Weight;
                }
            }

            if (WeightSum > 0.f)
            {
                OutputVertices[I].Z = FMath::Lerp(OutputVertices[I].Z, SumZ / WeightSum, SmoothStrength);
            }
        }
    }

    // Required by FAsyncTask
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FLidarSmoothingTask, STATGROUP_ThreadPoolAsyncTasks);
    }
};
