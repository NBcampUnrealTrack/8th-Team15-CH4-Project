// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SICharacter.generated.h"

class APlacedShapeActor;
class APlacementPreviewActor;
class USIColorPalette;
class USIDrawingToolWidget;
class USIPreviewTransformGizmoComponent;
class UInputMappingContext;
class USIIPlayerCharacternputConfig;
class UStaticMeshComponent;
class UCameraComponent;
class UWidgetComponent;

struct FInputActionValue;
enum class ESIGamePhase : uint8;

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
	
	// 서버에서 소유권이 설정될 때 호출
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void PostNetInit() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma endregion 
	
protected:
	// 가시성을 갱신하는 공통 함수
	void UpdateMeshVisibility();
	
#pragma region Character Component
	
protected:
	// 1인칭 카메라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UCameraComponent> CameraComp;
	
	// 1인칭 카메라 전용 팔(손)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UStaticMeshComponent> ArmOnlyComp;

	// 머리 위 닉네임. 1인칭이라 본인에게는 보이지 않게 한다(SetOwnerNoSee).
	// 표시할 위젯 클래스는 BP_PlayerCharacter의 이 컴포넌트 디테일에서 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterComponent")
	TObjectPtr<UWidgetComponent> NameplateComp;

#pragma endregion

#pragma region Nameplate

protected:
	/** 닉네임 표시를 현재 PlayerState 기준으로 갱신한다.
		이름은 PlayerState와 별개로 복제되어 늦게 도착할 수 있으므로,
		비어 있으면 짧은 간격으로 재시도한다(이 코드베이스의 0.1초 재시도 패턴). */
	void RefreshNameplate();

	/** 이름표를 보는 사람의 카메라 쪽으로 돌린다(빌보딩).
		World 공간 위젯은 스스로 카메라를 향하지 않으므로 매 틱 직접 돌려줘야 한다. */
	void UpdateNameplateFacing();

	/** 클라이언트에서 PlayerState 포인터가 도착하는 시점 */
	virtual void OnRep_PlayerState() override;

private:
	FTimerHandle NameplateRetryTimer;

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
	
	UFUNCTION(Server, Reliable)
	void ServerRPCSetMovementMode(EMovementMode NewMovementMode);
	
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
	// 오브젝트 편집 상태는 UI 모드와 별도로 유지한다.
	enum class EObjectEditState : uint8
	{
		None,
		Preview,
		Selected,
		Rebase,
		Copy
	};
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UDataTable> ShapeDefinitionTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Color")
	TObjectPtr<USIColorPalette> ColorPalette;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<APlacementPreviewActor> PreviewActorClass;
	
	UPROPERTY()
	TObjectPtr<APlacementPreviewActor> PreviewActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<APlacedShapeActor> PlacedShapeActorClass;

	// 캐릭터 한 명이 빌드 페이즈에 설치할 수 있는 최대 도형 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Limits", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxPlacedShapeCount;

	UPROPERTY()
	TObjectPtr<APlacedShapeActor> HoveredShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<USIPreviewTransformGizmoComponent> PreviewTransformGizmoComponent;
	
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
	uint8 CurrentPreviewColorIndex = 0;
	FLinearColor CurrentPreviewColor = FLinearColor::White;
	bool bIsEditingExistingShape;
	FName EditingOriginalShapeId;
	FTransform EditingOriginalTransform;
	uint8 EditingOriginalColorIndex = 0;
	FLinearColor EditingOriginalColor = FLinearColor::White;
	EObjectEditState ObjectEditState = EObjectEditState::None;
	bool bApplyingGizmoTransform = false;
	bool bIsGizmoDragging = false;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartShapePreview(FName ShapeId);

	UFUNCTION(BlueprintCallable, Category = "Placement|Color")
	void SetSelectedPaletteColor(int32 ColorIndex, FLinearColor Color);

	UFUNCTION(BlueprintPure, Category = "Placement|Limits")
	int32 GetPlacedShapeCount() const { return PlacedShapeCount; }

	UFUNCTION(BlueprintPure, Category = "Placement|Limits")
	int32 GetMaxPlacedShapeCount() const { return MaxPlacedShapeCount; }

	UFUNCTION(BlueprintPure, Category = "Placement|Limits")
	int32 GetRemainingPlacedShapeCount() const;

	void HandleShapeEditingPhaseChanged(ESIGamePhase NewPhase);
	void RestoreActiveShapeEditForPhaseChange();
	
private:
	// Detail Panel Transform 편집은 기즈모로 이전되어 임시 비활성화한다.
	// void BindDetailPanelDelegates();
	// void SyncDetailPanelToPreview();
	void UpdatePreviewTransform();
	void UpdateHoveredShape();
	void SetHoveredShape(APlacedShapeActor* NewHoveredShape);
	void StartEditPreview(FName ShapeId, const FTransform& PreviewTransform, uint8 ColorIndex, const FLinearColor& Color);
	void ClearPreview();
	void UpdateShapePanelAvailability();
	void HandlePrimaryActionStarted();
	void HandlePrimaryActionCompleted();
	void ToggleRebaseMode();
	void ToggleCopyMode();
	void StartPreviewGizmo(bool bEnableLocation);
	void SetObjectGizmoInputMode(bool bEnable);
	void HandlePreviewGizmoTransformChanged(const FTransform& NewTransform);
	void UpdateSelectedCameraFocus();
	void BeginSelectedCameraFocus();
	void EndSelectedCameraFocus();
	bool IsSelectionCameraLocked() const;
	bool IsShapeEditingAllowed() const;
	bool IsServerInBuildPhase() const;
	bool CanSpawnPlacedShapeOnServer(bool bReplacingExistingShape) const;
	bool SpawnPlacedShapeOnServer(FName ShapeId, const FTransform& SpawnTransform, uint8 ColorIndex, bool bReplacingExistingShape);
	void SetPlacedShapeCount(int32 NewCount);
	void UpdatePlacementCountUI();
	void ClearServerShapeEditState();

	UPROPERTY(ReplicatedUsing = OnRep_PlacedShapeCount, VisibleInstanceOnly, BlueprintReadOnly, Category = "Placement|Limits", meta = (AllowPrivateAccess = "true"))
	int32 PlacedShapeCount = 0;

	UFUNCTION()
	void OnRep_PlacedShapeCount();

	// 서버가 보관하는 재배치 원본 정보다.
	bool bHasServerActiveShapeEdit = false;
	FName ServerEditingOriginalShapeId;
	FTransform ServerEditingOriginalTransform = FTransform::Identity;
	uint8 ServerEditingOriginalColorIndex = 0;

	// UFUNCTION()
	// void HandlePreviewRotationChanged(EAxis::Type Axis, float Value);

	// UFUNCTION()
	// void HandlePreviewScaleChanged(EAxis::Type Axis, float Value);
	
	UFUNCTION(Server, Reliable)
	void Server_RequestSpawnShape(FName ShapeId, FTransform SpawnTransform, uint8 ColorIndex);

	UFUNCTION(Server, Reliable)
	void Server_RequestEditShape(APlacedShapeActor* TargetShape);

	UFUNCTION(Server, Reliable)
	void Server_RequestDeleteEditedShape();

	UFUNCTION(Client, Reliable)
	void Client_StartEditShape(FName ShapeId, FTransform PreviewTransform, uint8 ColorIndex, FLinearColor Color);
	
#pragma endregion
	
};
