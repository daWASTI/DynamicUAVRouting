#include "LidarComponent.h"
#include "LidarProcessor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ULidarComponent::ULidarComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    GradientHeights = { 0.f, 500.f, 1000.f, 2000.f };
    GradientColors = { FLinearColor::Blue, FLinearColor::Green, FLinearColor::Yellow, FLinearColor::Red };
}

void ULidarComponent::BeginPlay()
{
    Super::BeginPlay();
}

void ULidarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ProcessPendingTraces();
}

FLinearColor ULidarComponent::GetColorForHeight(float Z) const
{
    const int32 NumStops = FMath::Min(GradientColors.Num(), GradientHeights.Num());
    if (NumStops == 0) return FLinearColor::White;

    if (Z <= GradientHeights[0]) return GradientColors[0];
    if (Z >= GradientHeights[NumStops - 1]) return GradientColors[NumStops - 1];

    for (int32 i = 0; i < NumStops - 1; i++)
    {
        if (Z >= GradientHeights[i] && Z <= GradientHeights[i + 1])
        {
            const float Alpha = (Z - GradientHeights[i]) / (GradientHeights[i + 1] - GradientHeights[i]);
            return FLinearColor::LerpUsingHSV(GradientColors[i], GradientColors[i + 1], Alpha);
        }
    }

    return GradientColors[NumStops - 1];
}

void ULidarComponent::FireLidarScan()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    FPendingTrace PendingTrace;
    PendingTrace.Handles.Reserve(NumRays);
    PendingTrace.Hits.Reserve(NumRays);

    const FVector Origin = Owner->GetActorLocation();
    const FRotator Rot = Owner->GetActorRotation();

    for (int32 i = 0; i < NumRays; ++i)
    {
        const float Theta = FMath::FRandRange(0.f, PI / 2.f);
        const float Phi = FMath::FRandRange(0.f, 2.f * PI);

        FVector Direction(
            FMath::Sin(Theta) * FMath::Cos(Phi),
            FMath::Sin(Theta) * FMath::Sin(Phi),
            -FMath::Cos(Theta));
        Direction = Rot.RotateVector(Direction);

        FCollisionQueryParams Params;
        Params.bReturnPhysicalMaterial = false;

        PendingTrace.Handles.Add(World->AsyncLineTraceByChannel(
            EAsyncTraceType::Single,
            Origin,
            Origin + Direction * MaxDistance,
            LidarTraceChannel,
            Params,
            FCollisionResponseParams::DefaultResponseParam));
    }

    PendingTraces.Add(MoveTemp(PendingTrace));
}

void ULidarComponent::ProcessPendingTraces()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (int32 TraceIndex = PendingTraces.Num() - 1; TraceIndex >= 0; --TraceIndex)
    {
        FPendingTrace& Trace = PendingTraces[TraceIndex];

        for (int32 HandleIndex = Trace.Handles.Num() - 1; HandleIndex >= 0; --HandleIndex)
        {
            FTraceDatum TraceData;
            if (!World->QueryTraceData(Trace.Handles[HandleIndex], TraceData))
            {
                continue;
            }

            for (const FHitResult& Hit : TraceData.OutHits)
            {
                if (!Hit.bBlockingHit)
                {
                    continue;
                }

                Trace.Hits.Add(Hit.ImpactPoint);

                if (bDrawDebug)
                {
                    const FColor Color = GetColorForHeight(Hit.ImpactPoint.Z).ToFColor(true);
                    DrawDebugPoint(World, Hit.ImpactPoint, 5.f, Color, false, DebugPointLifetime);
                }
            }

            Trace.Handles.RemoveAtSwap(HandleIndex);
        }

        if (Trace.Handles.Num() == 0)
        {
            OnTraceBatchComplete(Trace.Hits);
            PendingTraces.RemoveAt(TraceIndex);
        }
    }
}

void ULidarComponent::OnTraceBatchComplete(const TArray<FVector>& FramePoints)
{
    if (FramePoints.Num() == 0)
    {
        return;
    }

    if (LidarProcessor)
    {
        LidarProcessor->AddPoints(FramePoints);
    }
}
