#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DronePawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFloatingPawnMovement;
class UStaticMeshComponent;

UCLASS()
class DYNAMICUAVROUTING_API ADronePawn : public APawn
{
    GENERATED_BODY()

public:
    ADronePawn();

protected:

    virtual void BeginPlay() override;

public:

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* DroneMesh;

    UPROPERTY(VisibleAnywhere)
    USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere)
    UCameraComponent* Camera;

    UPROPERTY(VisibleAnywhere)
    UFloatingPawnMovement* MovementComponent;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void MoveUp(float Value);

    void Turn(float Value);
    void LookUp(float Value);
};