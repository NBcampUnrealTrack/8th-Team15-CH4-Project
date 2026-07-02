// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TestCharacter.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class UPawnMovementComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class TEAMPROJECT_API ATestCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATestCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float ServerMoveSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveUpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PawnClientRestart() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UPawnMovementComponent* GetMovementComponent() const override;

private:
	void Move(const FInputActionValue& Value);
	void MoveUp(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UFUNCTION(Server, Unreliable)
	void ServerMove(FVector2D MovementVector, float ControlYaw);

	UFUNCTION(Server, Unreliable)
	void ServerMoveUp(float AxisValue);

	void ApplyMove(FVector2D MovementVector, float ControlYaw);
	void ApplyMoveUp(float AxisValue);
	void ApplyServerMove(FVector2D MovementVector, float ControlYaw, float DeltaTime);
	void ApplyServerMoveUp(float AxisValue, float DeltaTime);
	void AddMappingContext();

	FVector2D ServerMovementVector;
	float ServerMovementControlYaw;
	float ServerMoveUpAxis;
	float LastServerMoveInputTime;
	float LastServerMoveUpInputTime;
	float ServerInputTimeout;

};
