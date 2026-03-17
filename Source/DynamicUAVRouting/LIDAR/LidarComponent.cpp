#include "LidarComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

// Interpolate color gradient based on absolute world Z
static FLinearColor GetColorForHeight(float Z, const TArray<FLinearColor>& Colors, const TArray<float>& Heights)
{
    int32 NumStops = FMath::Min(Colors.Num(), Heights.Num());
    if (NumStops == 0)
        return FLinearColor::White;

    if (Z <= Heights[0])
        return Colors[0];
    if (Z >= Heights[NumStops - 1])
        return Colors[NumStops - 1];

    for (int32 i = 0; i < NumStops - 1; i++)
    {
        if (Z >= Heights[i] && Z <= Heights[i + 1])
        {
            float Alpha = (Z - Heights[i]) / (Heights[i + 1] - Heights[i]);
            return FLinearColor::LerpUsingHSV(Colors[i], Colors[i + 1], Alpha);
        }
    }

    return Colors[NumStops - 1];
}

ULidarComponent::ULidarComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Default gradient
    GradientHeights = { 0.f, 500.f, 1000.f, 2000.f };
    GradientColors = {
        FLinearColor(0.f, 0.f, 1.f),   // Blue
        FLinearColor(0.f, 1.f, 0.f),   // Green
        FLinearColor(1.f, 1.f, 0.f),   // Yellow
        FLinearColor(1.f, 0.f, 0.f)    // Red
    };
}

void ULidarComponent::BeginPlay()
{
    Super::BeginPlay();
}

TArray<FVector> ULidarComponent::LIDARSnapshot()
{
    TArray<FVector> Points;
    if (!GetOwner())
        return Points;

    FVector Origin = GetOwner()->GetActorLocation();
    FRotator Rotation = GetOwner()->GetActorRotation();

    for (int32 i = 0; i < NumRays; i++)
    {
        float Theta = FMath::FRandRange(0.f, PI / 2.f);
        float Phi = FMath::FRandRange(0.f, 2.f * PI);

        FVector Direction(
            FMath::Sin(Theta) * FMath::Cos(Phi),
            FMath::Sin(Theta) * FMath::Sin(Phi),
            -FMath::Cos(Theta)
        );

        Direction = Rotation.RotateVector(Direction);
        FVector End = Origin + Direction * MaxDistance;

        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, LidarTraceChannel))
        {
            Points.Add(Hit.Location);

            if (bDrawDebug)
            {
                FLinearColor LinearColor = GetColorForHeight(Hit.ImpactPoint.Z, GradientColors, GradientHeights);
                FColor Color = LinearColor.ToFColor(true);
                DrawDebugPoint(GetWorld(), Hit.Location, 5.f, Color, false, DebugPointLifetime);
            }
        }
    }

    return Points;
}