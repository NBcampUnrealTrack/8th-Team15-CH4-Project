// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SICharacter.generated.h"

class APlacedShapeActor;
class APlacementPreviewActor;
class UDetailPanelWidget;
class UInputMappingContext;
class USIIPlayerCharacternputConfig;
class UStaticMeshComponent;
class UCameraComponent;

struct FInputActionValue;

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
	// 1인칭 카메라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UCameraComponent> CameraComp;
	
	// 1인칭 카메라 전용 팔(손)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UStaticMeshComponent> ArmOnlyComp;
	
#pragma endregion
	
#pragma region Input
	
private:
	// WASDQE 이동 조작
	void Move(const FInputActionValue& InValue);
	// 카메라 상하좌우 조작
	void Look(const FInputActionValue& InValue);
	// 점프 or Fly 상태 전환
	void HandleJumpNFly();
	// UI Only Mode 전환
	void ToggleUIOnlyMode();
	
	// Preview 관련 인풋
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartBoxPreview();
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartSpherePreview();
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartCylinderPreview();
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartConePreview();
	
	void ConfirmPlacement();
	void CancelPreview();
	void IncreasePreviewDistance();
	void DecreasePreviewDistance();
	void ResetPreviewTransform();
	
protected:
	// 플레이어 인풋 IA 식별자 데이터 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	TObjectPtr<USIIPlayerCharacternputConfig> PlayerCharacterInputConfig;
	// 플레이어 IMC
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	TObjectPtr<UInputMappingContext> PlayerCharacterInputMappingContext;
	
private:
	// 플레이어 Fly 상태 변환을 위한 값
	float LastJumpTime = 0.0f;
	const float DoubleJumpThreshold = 0.3f;
	
public:
	// 플레이어 착지 시 자동 호출되는 Landed 함수
	virtual void Landed(const FHitResult& Hit) override;
	
#pragma endregion 
	
#pragma region UI
	
private:
	// UI Only Mode 변수
	bool bIsUIOnlyMode = false;
	// UI Visble 변수
	bool bIsUIVisible = false;
	
#pragma endregion 
	
#pragma region Object
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UDataTable> ShapeDefinitionTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<APlacementPreviewActor> PreviewActorClass;
	
	UPROPERTY()
	TObjectPtr<APlacementPreviewActor> PreviewActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<APlacedShapeActor> PlacedShapeActorClass;

	UPROPERTY()
	TObjectPtr<APlacedShapeActor> HoveredShape;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float PreviewDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float MinPreviewDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float MaxPreviewDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float PreviewDistanceStep;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float EditTraceDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float MaxEditDistance;
	
private:
	FName CurrentPreviewShapeId;
	FRotator PreviewRotation;
	FVector PreviewScale;
	bool bIsEditingExistingShape;
	FName EditingOriginalShapeId;
	FTransform EditingOriginalTransform;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartShapePreview(FName ShapeId);
	
private:
	void BindDetailPanelDelegates();
	void SyncDetailPanelToPreview();
	void UpdatePreviewTransform();
	void UpdateHoveredShape();
	void SetHoveredShape(APlacedShapeActor* NewHoveredShape);
	void StartEditPreview(FName ShapeId, const FTransform& PreviewTransform);
	void ClearPreview();

	UFUNCTION()
	void HandlePreviewRotationChanged(EAxis::Type Axis, float Value);

	UFUNCTION()
	void HandlePreviewScaleChanged(EAxis::Type Axis, float Value);
	
	UFUNCTION(Server, Reliable)
	void Server_RequestSpawnShape(FName ShapeId, FTransform SpawnTransform);

	UFUNCTION(Server, Reliable)
	void Server_RequestEditShape(APlacedShapeActor* TargetShape);

	UFUNCTION(Client, Reliable)
	void Client_StartEditShape(FName ShapeId, FTransform PreviewTransform);
	
#pragma endregion
	
};
