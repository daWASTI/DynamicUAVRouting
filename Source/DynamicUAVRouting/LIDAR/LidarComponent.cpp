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
        if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility))
        {
            Points.Add(Hit.Location);

            if (bDrawDebug)
            {
                // Live hits: gray, temporary
                DrawDebugPoint(GetWorld(), Hit.Location, 5.f, FColor(128, 128, 128), false, 1.f);
            }
        }
    }

    return Points;
}

void ULidarComponent::LIDARAggregate(const TArray<FVector>& Points)
{
    if (Points.Num() == 0)
        return;

    for (const FVector& P : Points)
    {
        // Store exact hit location in cumulative array
        AggregatedPoints.Add(P);

        // Draw permanent debug at exact hit location with gradient color
        if (bDrawDebug)
        {
            FLinearColor ColorLinear = GetColorForHeight(P.Z, GradientColors, GradientHeights);
            FColor Color = ColorLinear.ToFColor(true);
            DrawDebugPoint(GetWorld(), P, 6.f, Color, true); // scattered permanent point
        }

        // Update grid-based heightmap for terrain
        FIntPoint GridKey(
            FMath::FloorToInt(P.X / GridSize),
            FMath::FloorToInt(P.Y / GridSize)
        );

        float* ExistingHeight = HeightMap.Find(GridKey);
        if (ExistingHeight)
        {
            *ExistingHeight = FMath::Max(*ExistingHeight, P.Z);
        }
        else
        {
            HeightMap.Add(GridKey, P.Z);
        }
    }
}