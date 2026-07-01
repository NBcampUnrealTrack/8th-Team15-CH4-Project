// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SICharacter.generated.h"

class UInputMappingContext;
class USIIPlayerCharacternputConfig;
struct FInputActionValue;
class UStaticMeshComponent;
class UCameraComponent;

UCLASS()
class TEAMPROJECT_API ASICharacter : public ACharacter
{
	GENERATED_BODY()

#pragma region ACharacter Override
	
public:
	// Sets default values for this character's properties
	ASICharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma endregion 
	
#pragma region Character Component
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UCameraComponent> CameraComp;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UStaticMeshComponent> ArmOnlyComp;
	
#pragma endregion
	
#pragma region Input
	
private:
	void Move(const FInputActionValue& InValue);
	void Look(const FInputActionValue& InValue);
	virtual void Jump() override;
	void Fly(const FInputActionValue& Invalue);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	TObjectPtr<USIIPlayerCharacternputConfig> PlayerCharacterInputConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	TObjectPtr<UInputMappingContext> PlayerCharacterInputMappingContext;
	
#pragma endregion 
	
};
