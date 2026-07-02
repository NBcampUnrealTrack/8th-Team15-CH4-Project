// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SICharacter.h"

#include <string>

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/SIIPlayerCharacternputConfig.h"
#include "Object/PlacedShapeActor.h"
#include "Object/PlacementPreviewActor.h"
#include "Object/ShapeDefinitionRow.h"
#include "UI/DetailPanelWidget.h"

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
	PreviewRotation = FQuat::Identity;
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
	
	// 인스턴스가 없을 때, StaticClass만 존재한다면
	if (!TransformWidgetInstance && TransformWidgetClass)
	{
		// StaticClass를 통해 Instance화
		TransformWidgetInstance = CreateWidget<UDetailPanelWidget>(PlayerController, TransformWidgetClass);
	}
		
	// 인스턴스가 존재한다면
	if (TransformWidgetInstance)
	{
		// 뷰포트에 노출
		TransformWidgetInstance->AddToViewport();
	}
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
		
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Preview, ETriggerEvent::Started, this, &ThisClass::StartBoxPreview);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->IncreasePreviewDistance, ETriggerEvent::Triggered, this, &ThisClass::IncreasePreviewDistance);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->DecreasePreviewDistance, ETriggerEvent::Triggered, this, &ThisClass::DecreasePreviewDistance);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Confirm, ETriggerEvent::Started, this, &ThisClass::ConfirmPlacement);
		EnhancedInputComponent->BindAction(PlayerCharacterInputConfig->Cancel, ETriggerEvent::Started, this, &ThisClass::CancelPreview);
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
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
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
				GetCharacterMovement()->SetMovementMode(MOVE_Flying);
				// 캐릭터 Z값 이동속도 0으로 수정하여 기존 Z축 속도로 인한 예외 상황 방지
				GetCharacterMovement()->Velocity.Z = 0.0f; 
				GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Red, TEXT("EMovementMode::MOVE_Flying"));
			}
			else
			{
				// Falling 상태로 전환
				GetCharacterMovement()->SetMovementMode(MOVE_Falling);
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
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController) == false)
	{
		return;
	}
	
	bIsUIOnlyMode = !bIsUIOnlyMode;
	
	if (bIsUIOnlyMode)
	{
		GetCharacterMovement()->StopMovementImmediately();
 
		// 1. 마우스 위치를 먼저 중앙으로 '예약' 세팅
		int32 ViewportSizeX, ViewportSizeY;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		PlayerController->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
 
		// 2. 입력 모드 설정 (이 시점에서 위젯 포커스 준비)
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TransformWidgetInstance->TakeWidget());
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

void ASICharacter::ToggleTransformUI()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController) == false)
	{
		return;
	}
	
	// UIVisible 전환
	bIsUIVisible = !bIsUIVisible;
	
	// UI가 화면에 보여야한다면
	if (bIsUIVisible)
	{
		// 인스턴스가 없을 때, StaticClass만 존재한다면
		if (!TransformWidgetInstance && TransformWidgetClass)
		{
			// StaticClass를 통해 Instance화
			TransformWidgetInstance = CreateWidget<UDetailPanelWidget>(PlayerController, TransformWidgetClass);
		}
		
		// 인스턴스가 존재한다면
		if (TransformWidgetInstance)
		{
			// 뷰포트에 노출
			TransformWidgetInstance->AddToViewport();
		}
		
		// UIOnly InputMode로 전환을 위한 변수
		FInputModeUIOnly  InputMode;
		// 입력 우선 TakeWidget로 Focus
		InputMode.SetWidgetToFocus(TransformWidgetInstance->TakeWidget());
		// 마우스를 게임 화면 내에 감금
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		// UIOnly InputMode로 전환
		PlayerController->SetInputMode(InputMode);
		
		// MouseCursor 노출
		PlayerController->bShowMouseCursor = true;
		// 기존 인풋컨테이너의 값들을 삭제
		PlayerController->FlushPressedKeys();
	}
	else
	{
		// 인스턴스가 존재한다면
		if (TransformWidgetInstance)
		{
			// 인스턴스 삭제
			TransformWidgetInstance->RemoveFromParent();
		}
		
		// GameOnly InputMode로 전환
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		
		// MouseCursor 비노출
		PlayerController->bShowMouseCursor = false;
	}
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

#pragma endregion

#pragma region Obeject

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
		PreviewRotation = FQuat::Identity;
		bIsEditingExistingShape = false;
		EditingOriginalShapeId = NAME_None;
		EditingOriginalTransform = FTransform::Identity;
		UpdatePreviewTransform();
	}
}

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
	const FVector PreviewLocation = bHit ? HitResult.ImpactPoint : TraceEnd;

	PreviewActor->SetActorLocation(PreviewLocation);
	PreviewActor->SetActorRotation(PreviewRotation);
}

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
		PreviewRotation = PreviewTransform.GetRotation();

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
	}
}

void ASICharacter::ClearPreview()
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	CurrentPreviewShapeId = NAME_None;
	PreviewRotation = FQuat::Identity;
	bIsEditingExistingShape = false;
	EditingOriginalShapeId = NAME_None;
	EditingOriginalTransform = FTransform::Identity;
}

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
		PlacedShape->SetPlacedShape(ShapeDefinitionTable, ShapeId);
	}
}

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

void ASICharacter::Client_StartEditShape_Implementation(FName ShapeId, FTransform PreviewTransform)
{
	StartEditPreview(ShapeId, PreviewTransform);
}

#pragma endregion
