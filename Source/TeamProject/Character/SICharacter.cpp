// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SICharacter.h"

#include <string>

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/SIIPlayerCharacternputConfig.h"
#include "Object/PlacedShapeActor.h"
#include "Object/PlacementPreviewActor.h"
#include "Object/ShapeDefinitionRow.h"
#include "PlayerController/SIPlayerController.h"
#include "UI/DetailPanelWidget.h"
#include "UI/SIUserWidget.h"
#include "Component/SIUIManagerComponent.h"

namespace
{
	constexpr float PreviewRotationRangeDegrees = 180.0f;
	constexpr float PreviewSliderMinValue = 0.0f;
	constexpr float PreviewSliderMaxValue = 2.0f;
	constexpr float PreviewSliderCenterValue = 1.0f;
	constexpr float MinPreviewScale = 0.65f;
	constexpr float MaxPreviewScale = 1.35f;

	float RotationSliderValueToDegrees(float Value)
	{
		const float ClampedValue = FMath::Clamp(Value, PreviewSliderMinValue, PreviewSliderMaxValue);
		return (ClampedValue - PreviewSliderCenterValue) * PreviewRotationRangeDegrees;
	}

	float ScaleSliderValueToScale(float Value)
	{
		return FMath::GetMappedRangeValueClamped(FVector2D(PreviewSliderMinValue, PreviewSliderMaxValue), FVector2D(MinPreviewScale, MaxPreviewScale), Value);
	}
}

#pragma region ACharacter Override

ASICharacter::ASICharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// 1인칭 카메라
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(GetCapsuleComponent());
	CameraComp->bUsePawnControlRotation = true;

	// 1인칭 카메라 전용 팔(손)
	ArmOnlyComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arm"));
	ArmOnlyComp->SetupAttachment(CameraComp);
	ArmOnlyComp->SetOnlyOwnerSee(true);
	ArmOnlyComp->bCastDynamicShadow = false;
	
	// ??? 까먹음 추후 수정함
	GetMesh()->SetOwnerNoSee(true);
	
	// Fly 상태 Movement 제어값
	GetCharacterMovement()->BrakingDecelerationFlying = 2000.0f;
	GetCharacterMovement()->MaxFlySpeed = 800.0f;
	
	// Object Settings
	PreviewDistance = 600.0f;
	MinPreviewDistance = 150.0f;
	MaxPreviewDistance = 2000.0f;
	PreviewDistanceStep = 100.0f;
	EditTraceDistance = 3000.0f;
	MaxEditDistance = 3000.0f;
	PreviewRotation = FRotator::ZeroRotator;
	PreviewScale = FVector::OneVector;
	bIsEditingExistingShape = false;
	PreviewActorClass = APlacementPreviewActor::StaticClass();
	PlacedShapeActorClass = APlacedShapeActor::StaticClass();
}

// Called when the game starts or when spawned
void ASICharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(PlayerCharacterInputMappingContext) == false)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController) == false)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (IsValid(Subsystem) == false)
	{
		return;
	}
	
	// PlayerCharacterInputMappingContext Mapping
	Subsystem->AddMappingContext(PlayerCharacterInputMappingContext, 1);

	// 초기 가시성 설정
	UpdateMeshVisibility();
	
	BindDetailPanelDelegates();
}

void ASICharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	// 초기 가시성 설정
	UpdateMeshVisibility();
}

void ASICharacter::PostNetInit()
{
	Super::PostNetInit();
	
	// 초기 가시성 설정
	UpdateMeshVisibility();
}

// Called every frame
void ASICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePreviewTransform();
	UpdateHoveredShape();
}

// Called to bind functionality to input
void ASICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (IsValid(EnhancedInputComponent) == true)
	{
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Move, ETriggerEvent::Triggered, this, &ThisClass::Move);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Look, ETriggerEvent::Triggered, this, &ThisClass::Look);
	
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Jump, ETriggerEvent::Started, this, &ThisClass::HandleJumpNFly);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->ToggleTransformUI, ETriggerEvent::Triggered, this, &ThisClass::ToggleUIOnlyMode);
		
		// EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Preview, ETriggerEvent::Started, this, &ThisClass::StartBoxPreview);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->IncreasePreviewDistance, ETriggerEvent::Triggered, this, &ThisClass::IncreasePreviewDistance);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->DecreasePreviewDistance, ETriggerEvent::Triggered, this, &ThisClass::DecreasePreviewDistance);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Confirm, ETriggerEvent::Started, this, &ThisClass::ConfirmPlacement);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Cancel, ETriggerEvent::Started, this, &ThisClass::CancelPreview);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->ResetPreviewTransform, ETriggerEvent::Started, this, &ThisClass::ResetPreviewTransform);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Participants, ETriggerEvent::Started, this, &ThisClass::OpenParticipants);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Participants, ETriggerEvent::Completed, this, &ThisClass::CloseParticipants);
	}
}

void ASICharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ASICharacter::UpdateMeshVisibility()
{
	// 핵심: 이 캐릭터가 현재 로컬 플레이어에 의해 조종되는지 확인
	bool bIsLocallyControlled = IsLocallyControlled();
 
	if (ArmOnlyComp)
	{
		// 1. 소유자만 보게 설정 (기본 엔진 기능)
		ArmOnlyComp->SetOnlyOwnerSee(true);
        
		// 2. 리슨 서버 프록시 문제를 해결하기 위한 수동 제거 (eliminate)
		// 로컬 컨트롤 중이 아니면 게임에서 아예 숨깁니다.
		ArmOnlyComp->SetHiddenInGame(!bIsLocallyControlled);
	}
 
	if (GetMesh())
	{
		// 3인칭 몸체 매쉬는 내 화면(1인칭)에서 숨기고 남들에게만 보이게 설정
		GetMesh()->SetOwnerNoSee(true);
	}

}

#pragma endregion 

#pragma region Input

void ASICharacter::Move(const FInputActionValue& InValue)
{
	// UI 조작 중이라면 입력을 무시합니다.
	if (bIsUIOnlyMode)
	{
		return;
	}
	
	const FVector MovementVector = InValue.Get<FVector>();
	UE_LOG(LogTemp, Warning, TEXT("Input: %s"), *MovementVector.ToString());
	
	if (IsValid(Controller) == false)
	{
		return;
	}

	// PlayerController가 바라보고 있는 방향(Rotator)의 좌우(Yaw)
	const FRotator ControlRotation = GetController()->GetControlRotation();
	const FRotator ControlRotationYaw(0.f, ControlRotation.Yaw, 0.f);

	// PlayerController의 Yaw값을 이용하여 Forward, Right Vector값을 산출
	const FVector ForwardVector = FRotationMatrix(ControlRotationYaw).GetUnitAxis(EAxis::X);
	const FVector RightVector = FRotationMatrix(ControlRotationYaw).GetUnitAxis(EAxis::Y);

	// 입력값에 따른 Forward, Right 움직임
	AddMovementInput(ForwardVector, MovementVector.X);
	AddMovementInput(RightVector, MovementVector.Y);
	
	// MOVE_Flying 상태가 아니라면 반환(방어코드)
	if (GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Flying)
	{
		return;
	}
	
	// 인풋값에 따른 Z축 방향 이동
	if (FMath::Abs(MovementVector.Z) > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(FVector::UpVector, MovementVector.Z);
	}
	
	// 하강 중인 경우
	if (MovementVector.Z < KINDA_SMALL_NUMBER)
	{
		FFindFloorResult FloorResult;
		
		GetCharacterMovement()->FindFloor(
			GetCapsuleComponent()->GetComponentLocation(), 
			FloorResult, 
			false);
 
		// 걸을 수 있는 바닥과의 거리가 10 이하라면
		if (FloorResult.IsWalkableFloor() && FloorResult.FloorDist < 10.f) 
		{
			// Falling 상태로 변경
			ServerRPCSetMovementMode(MOVE_Falling);
			// 중력의 영향으로 덜컥 거리는 현상 방지
			GetCharacterMovement()->Velocity.Z = 0.f;
			GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Red, TEXT("GetCharacterMovement()->SetMovementMode(MOVE_Falling)"));
			// 1틱 뒤에 자동 호출되지만 자연스러움을 위해 직접 호출
			Landed(FloorResult.HitResult);
		}
	}
}

void ASICharacter::Look(const FInputActionValue& InValue)
{
	// UI 조작 중이라면 입력을 무시합니다.
	if (bIsUIOnlyMode)
	{
		return;
	}
	
	FVector2D LookVector = InValue.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void ASICharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Red, TEXT("ASICharacter::Landed"));
	// Effect, Sound Code
}

void ASICharacter::HandleJumpNFly()
{
	// UI 조작 중이라면 입력을 무시합니다.
	if (bIsUIOnlyMode)
	{
		return;
	}
	
	if (IsValid(GetWorld()) == false)
	{
		return;
	}
	// fly 상태 전환을 위한 현재 시간값
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	// 캐릭터가 낙하 중이라면
	if (GetCharacterMovement()->IsFalling())
	{
		// 현재 시간 - 이전 마지막 점프 당시의 시간 < 이중점프 판단기준 시간
		if ((CurrentTime - LastJumpTime) < DoubleJumpThreshold)
		{
			// Fly 상태가 아니라면
			if (GetCharacterMovement()->MovementMode != MOVE_Flying)
			{
				// Fly 상태로 전환
				ServerRPCSetMovementMode(MOVE_Flying);
				// 캐릭터 Z값 이동속도 0으로 수정하여 기존 Z축 속도로 인한 예외 상황 방지
				GetCharacterMovement()->Velocity.Z = 0.0f; 
				GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Red, TEXT("EMovementMode::MOVE_Flying"));
			}
			else
			{
				// Falling 상태로 전환
				ServerRPCSetMovementMode(MOVE_Falling);
				GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Red, TEXT("EMovementMode::MOVE_Falling"));
			}

		}
	}
	else
	{
		// Jump 함수 실행
		Jump();
		GEngine->AddOnScreenDebugMessage(3, 5.0f, FColor::Red, TEXT("Jump()"));
	}
	
	// 마지막 점프 시간 갱신
	LastJumpTime = CurrentTime;
}

void ASICharacter::ToggleUIOnlyMode()
{
	ASIPlayerController* PlayerController = Cast<ASIPlayerController>(GetController());
	if (IsValid(PlayerController) == false)
	{
		return;
	}
	
	bIsUIOnlyMode = !bIsUIOnlyMode;
	
	if (bIsUIOnlyMode)
	{
		GetCharacterMovement()->StopMovementImmediately();
		BindDetailPanelDelegates();
 
		// 1. 마우스 위치를 먼저 중앙으로 '예약' 세팅
		int32 ViewportSizeX, ViewportSizeY;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		PlayerController->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
 
		// 2. 입력 모드 설정 (이 시점에서 위젯 포커스 준비)
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(PlayerController->GetDetailPanelWidget()->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		
		// 3. 마지막에 커서를 노출 (이미 중앙으로 옮겨진 상태에서 등장)
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		// 게임 전용 모드로 복구
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}
	
	// 모드 전환 시 박혀있던 키 입력들 초기화
	PlayerController->FlushPressedKeys();	
}

void ASICharacter::OpenParticipants()
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC)
	{
		return;
	}

	USIUIManagerComponent* UIManagerComponent = PC->GetComponentByClass<USIUIManagerComponent>();

	if (!UIManagerComponent)
	{
		return;
	}
	UIManagerComponent->OpenWidget(EUIType::Participants);
}

void ASICharacter::CloseParticipants()
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC)
	{
		return;
	}

	USIUIManagerComponent* UIManagerComponent = PC->GetComponentByClass<USIUIManagerComponent>();

	if (!UIManagerComponent)
	{
		return;
	}
	UIManagerComponent->CloseWidget();
}

void ASICharacter::StartBoxPreview()
{
	StartShapePreview(TEXT("Box"));
}

void ASICharacter::StartSpherePreview()
{
	StartShapePreview(TEXT("Sphere"));
}

void ASICharacter::StartCylinderPreview()
{
	StartShapePreview(TEXT("Cylinder"));
}	

void ASICharacter::StartConePreview()
{
	StartShapePreview(TEXT("Cone"));
}

void ASICharacter::ConfirmPlacement()
{
	if (PreviewActor && !CurrentPreviewShapeId.IsNone())
	{
		// 설치는 서버에서 확정
		Server_RequestSpawnShape(CurrentPreviewShapeId, PreviewActor->GetActorTransform());
		if (bIsEditingExistingShape)
		{
			ClearPreview();
			return;
		}

		bIsEditingExistingShape = false;
		EditingOriginalShapeId = NAME_None;
		EditingOriginalTransform = FTransform::Identity;
		return;
	}

	// 기존 도형 편집 요청
	Server_RequestEditShape(HoveredShape);
}

void ASICharacter::CancelPreview()
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	if (bIsEditingExistingShape && !EditingOriginalShapeId.IsNone())
	{
		// 편집 취소 시 원래 도형 복구
		Server_RequestSpawnShape(EditingOriginalShapeId, EditingOriginalTransform);
	}

	ClearPreview();
}

void ASICharacter::IncreasePreviewDistance()
{
	PreviewDistance = FMath::Clamp(PreviewDistance + PreviewDistanceStep, MinPreviewDistance, MaxPreviewDistance);
	UpdatePreviewTransform();
}

void ASICharacter::DecreasePreviewDistance()
{
	PreviewDistance = FMath::Clamp(PreviewDistance - PreviewDistanceStep, MinPreviewDistance, MaxPreviewDistance);
	UpdatePreviewTransform();
}


//선택 상태가 아닐시 도형의 로테이트, 스케일 값 초기화 
void ASICharacter::ResetPreviewTransform()
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	//선택 상태일 시 도형 삭제
	if (bIsEditingExistingShape)
	{
		ClearPreview();
		return;
	}

	PreviewRotation = FRotator::ZeroRotator;
	PreviewScale = FVector::OneVector;

	if (ShapeDefinitionTable)
	{
		const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(CurrentPreviewShapeId, TEXT("ResetPreviewTransform"));
		if (ShapeDefinition)
		{
			PreviewScale = ShapeDefinition->DefaultScale;
		}
	}

	UpdatePreviewTransform();
	SyncDetailPanelToPreview();
}

void ASICharacter::ServerRPCSetMovementMode_Implementation(EMovementMode NewMovementMode)
{
	// NewMovementMode 상태로 전환
	GetCharacterMovement()->SetMovementMode(NewMovementMode);
}

#pragma endregion

#pragma region Obeject

// 선택한 도형의 배치 프리뷰를 시작한다.
void ASICharacter::StartShapePreview(FName ShapeId)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone() || !PreviewActorClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!PreviewActor)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		PreviewActor = World->SpawnActor<APlacementPreviewActor>(PreviewActorClass, FTransform::Identity, SpawnParameters);
	}

	if (PreviewActor && PreviewActor->SetPreviewShape(ShapeDefinitionTable, ShapeId))
	{
		SetHoveredShape(nullptr);
		CurrentPreviewShapeId = ShapeId;
		PreviewRotation = FRotator::ZeroRotator;
		PreviewScale = PreviewActor->GetActorScale3D();
		bIsEditingExistingShape = false;
		EditingOriginalShapeId = NAME_None;
		EditingOriginalTransform = FTransform::Identity;
		UpdatePreviewTransform();
		SyncDetailPanelToPreview();
	}
}

// 디테일 패널 UI의 회전/스케일 변경 이벤트를 프리뷰 조작 함수에 연결한다.
void ASICharacter::BindDetailPanelDelegates()
{
	ASIPlayerController* PlayerController = Cast<ASIPlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	UDetailPanelWidget* DetailPanel = PlayerController->GetDetailPanelWidget();
	if (!DetailPanel)
	{
		return;
	}

	DetailPanel->OnRotationChanged.AddUniqueDynamic(this, &ASICharacter::HandlePreviewRotationChanged);
	DetailPanel->OnScaleChanged.AddUniqueDynamic(this, &ASICharacter::HandlePreviewScaleChanged);

	if (PreviewActor && !CurrentPreviewShapeId.IsNone())
	{
		SyncDetailPanelToPreview();
	}
	else
	{
		DetailPanel->SetTransformControlsEnabled(false);
	}
}

// 현재 프리뷰의 회전/스케일 값을 디테일 패널 UI에 반영한다.
void ASICharacter::SyncDetailPanelToPreview()
{
	ASIPlayerController* PlayerController = Cast<ASIPlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	UDetailPanelWidget* DetailPanel = PlayerController->GetDetailPanelWidget();
	if (!DetailPanel)
	{
		return;
	}

	DetailPanel->SetTransformControlsEnabled(true);
	DetailPanel->SetRotationValues(PreviewRotation);
	DetailPanel->SetScaleValues(PreviewScale);
}

// 플레이어 시선 기준으로 프리뷰 위치와 변형을 갱신한다.
void ASICharacter::UpdatePreviewTransform()
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	AController* PlayerController = GetController();
	if (IsValid(PlayerController) == false)
	{
		return;
	}
	
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * PreviewDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UpdatePlacementPreview), false, this);

	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (bHit)
	{
		// 표면에 맞은 경우, 프리뷰 액터가 표면 법선 방향으로 도형 bounds만큼 밀어내도록 히트 정보를 넘긴다.
		PreviewActor->SetSurfaceAlignedTransform(HitResult.ImpactPoint, HitResult.ImpactNormal, PreviewRotation, PreviewScale);
	}
	else
	{
		PreviewActor->SetActorLocation(TraceEnd);
		PreviewActor->SetActorRotation(PreviewRotation);
		PreviewActor->SetActorScale3D(PreviewScale);
	}
}

// 시선에 들어온 편집 가능한 도형을 찾아 호버 상태를 갱신한다.
void ASICharacter::UpdateHoveredShape()
{
	if (PreviewActor && !CurrentPreviewShapeId.IsNone())
	{
		SetHoveredShape(nullptr);
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	
	AController* PlayerController = GetController();
	if (IsValid(PlayerController) == false)
	{
		return;
	}
	
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * EditTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UpdateHoveredShape), false, this);

	APlacedShapeActor* NewHoveredShape = nullptr;
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		NewHoveredShape = Cast<APlacedShapeActor>(HitResult.GetActor());
		if (NewHoveredShape && NewHoveredShape->IsBeingEdited())
		{
			NewHoveredShape = nullptr;
		}
	}

	SetHoveredShape(NewHoveredShape);
}

// 이전 호버 도형을 해제하고 새 호버 도형을 설정한다.
void ASICharacter::SetHoveredShape(APlacedShapeActor* NewHoveredShape)
{
	if (HoveredShape == NewHoveredShape)
	{
		return;
	}

	if (HoveredShape)
	{
		HoveredShape->SetHovered(false);
	}

	HoveredShape = NewHoveredShape;

	if (HoveredShape)
	{
		HoveredShape->SetHovered(true);
	}
}

// 기존에 배치된 도형을 프리뷰 상태로 전환해 편집을 시작한다.
void ASICharacter::StartEditPreview(FName ShapeId, const FTransform& PreviewTransform)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone() || !PreviewActorClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!PreviewActor)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		PreviewActor = World->SpawnActor<APlacementPreviewActor>(PreviewActorClass, PreviewTransform, SpawnParameters);
	}

	if (PreviewActor && PreviewActor->SetPreviewShape(ShapeDefinitionTable, ShapeId))
	{
		SetHoveredShape(nullptr);
		CurrentPreviewShapeId = ShapeId;
		bIsEditingExistingShape = true;
		EditingOriginalShapeId = ShapeId;
		EditingOriginalTransform = PreviewTransform;
		PreviewRotation = PreviewTransform.GetRotation().Rotator();
		PreviewScale = PreviewTransform.GetScale3D();

		FVector ViewLocation;
		FRotator ViewRotation;
		AController* PlayerController = GetController();
		if (IsValid(PlayerController) == false)
		{
			return;
		}
	
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

		const float ProjectedDistance = FVector::DotProduct(PreviewTransform.GetLocation() - ViewLocation, ViewRotation.Vector());
		PreviewDistance = FMath::Clamp(ProjectedDistance, MinPreviewDistance, MaxPreviewDistance);

		PreviewActor->SetActorTransform(PreviewTransform);
		PreviewRotation = PreviewActor->GetActorRotation();
		PreviewScale = PreviewActor->GetActorScale3D();
		SyncDetailPanelToPreview();
	}
}

// 현재 프리뷰를 제거하고 관련 상태와 UI를 초기화한다.
void ASICharacter::ClearPreview()
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	CurrentPreviewShapeId = NAME_None;
	PreviewRotation = FRotator::ZeroRotator;
	PreviewScale = FVector::OneVector;
	bIsEditingExistingShape = false;
	EditingOriginalShapeId = NAME_None;
	EditingOriginalTransform = FTransform::Identity;

	ASIPlayerController* PlayerController = Cast<ASIPlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	UDetailPanelWidget* DetailPanel = PlayerController->GetDetailPanelWidget();
	if (DetailPanel)
	{
		DetailPanel->SetTransformControlsEnabled(false);
	}
}

// 디테일 패널에서 변경된 회전 값을 프리뷰에 적용한다.
void ASICharacter::HandlePreviewRotationChanged(EAxis::Type Axis, float Value)
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	const float Degrees = RotationSliderValueToDegrees(Value);
	FRotator PreviewRotator = PreviewRotation;
	switch (Axis)
	{
	case EAxis::X:
		PreviewRotator.Roll = Degrees;
		break;
	case EAxis::Y:
		PreviewRotator.Pitch = Degrees;
		break;
	case EAxis::Z:
		PreviewRotator.Yaw = Degrees;
		break;
	default:
		return;
	}

	PreviewRotation = PreviewRotator;
	UpdatePreviewTransform();
}

// 디테일 패널에서 변경된 스케일 값을 프리뷰에 적용한다.
void ASICharacter::HandlePreviewScaleChanged(EAxis::Type Axis, float Value)
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	const float SafeValue = ScaleSliderValueToScale(Value);
	switch (Axis)
	{
	case EAxis::X:
		PreviewScale.X = SafeValue;
		break;
	case EAxis::Y:
		PreviewScale.Y = SafeValue;
		break;
	case EAxis::Z:
		PreviewScale.Z = SafeValue;
		break;
	default:
		return;
	}

	UpdatePreviewTransform();
}

// 서버에서 실제 배치 도형을 생성한다.
void ASICharacter::Server_RequestSpawnShape_Implementation(FName ShapeId, FTransform SpawnTransform)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone() || !PlacedShapeActorClass)
	{
		return;
	}

	const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(ShapeId, TEXT("Server_RequestSpawnShape"));
	if (!ShapeDefinition || !ShapeDefinition->Mesh)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;

	APlacedShapeActor* PlacedShape = World->SpawnActor<APlacedShapeActor>(PlacedShapeActorClass, SpawnTransform, SpawnParameters);
	if (PlacedShape)
	{
		PlacedShape->SetPlacedShape(ShapeDefinitionTable, ShapeId, SpawnTransform.GetScale3D());
	}
}

// 서버에서 편집 대상 도형을 확인하고 편집 프리뷰 시작을 클라이언트에 요청한다.
void ASICharacter::Server_RequestEditShape_Implementation(APlacedShapeActor* TargetShape)
{
	APawn* ControlledPawn = this;
	if (!ControlledPawn)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	AController* PlayerController = GetController();
	if (IsValid(PlayerController) == false)
	{
		return;
	}
	
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * EditTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ServerRequestEditShape), false, ControlledPawn);

	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		TargetShape = Cast<APlacedShapeActor>(HitResult.GetActor());
	}

	if (!TargetShape || TargetShape->IsBeingEdited())
	{
		return;
	}

	const FName ShapeId = TargetShape->GetShapeId();
	if (ShapeId.IsNone())
	{
		return;
	}

	const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), TargetShape->GetActorLocation());
	if (DistanceSquared > FMath::Square(MaxEditDistance))
	{
		return;
	}

	const FTransform PreviewTransform = TargetShape->GetActorTransform();
	TargetShape->SetBeingEdited(true);
	TargetShape->Destroy();

	Client_StartEditShape(ShapeId, PreviewTransform);
}

// 클라이언트에서 편집 프리뷰를 시작한다.
void ASICharacter::Client_StartEditShape_Implementation(FName ShapeId, FTransform PreviewTransform)
{
	StartEditPreview(ShapeId, PreviewTransform);
}

#pragma endregion
