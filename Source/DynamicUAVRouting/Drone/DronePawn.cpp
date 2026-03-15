#include "DronePawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/StaticMeshComponent.h"

ADronePawn::ADronePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
    RootComponent = DroneMesh;

    DroneMesh->SetSimulatePhysics(false);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 300.f;
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
    MovementComponent->MaxSpeed = 1200.f;
}

void ADronePawn::BeginPlay()
{
    Super::BeginPlay();
}

void ADronePawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ADronePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &ADronePawn::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ADronePawn::MoveRight);
    PlayerInputComponent->BindAxis("MoveUp", this, &ADronePawn::MoveUp);

    PlayerInputComponent->BindAxis("Turn", this, &ADronePawn::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &ADronePawn::LookUp);
}

void ADronePawn::MoveForward(float Value)
{
    AddMovementInput(GetActorForwardVector(), Value);
}

void ADronePawn::MoveRight(float Value)
{
    AddMovementInput(GetActorRightVector(), Value);
}

void ADronePawn::MoveUp(float Value)
{
    AddMovementInput(GetActorUpVector(), Value);
}

void ADronePawn::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void ADronePawn::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}