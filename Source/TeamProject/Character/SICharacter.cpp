// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SICharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/SIIPlayerCharacternputConfig.h"
#include "UI/SIUserWidget.h"

#pragma region ACharacter Override

ASICharacter::ASICharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
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
		TransformWidgetInstance = CreateWidget<USIUserWidget>(PlayerController, TransformWidgetClass);
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
			TransformWidgetInstance = CreateWidget<USIUserWidget>(PlayerController, TransformWidgetClass);
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

void ASICharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Red, TEXT("ASICharacter::Landed"));
	// Effect, Sound Code
}

#pragma endregion
