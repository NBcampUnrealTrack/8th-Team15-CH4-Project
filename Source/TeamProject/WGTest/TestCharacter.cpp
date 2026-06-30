// Fill out your copyright notice in the Description page of Project Settings.


#include "WGTest/TestCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

// Sets default values
ATestCharacter::ATestCharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.0f);
	SetMinNetUpdateFrequency(30.0f);
	ServerMoveSpeed = 1200.0f;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(CapsuleComponent);
	SpringArmComponent->TargetArmLength = 400.0f;
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = CapsuleComponent;
}

// Called when the game starts or when spawned
void ATestCharacter::BeginPlay()
{
	Super::BeginPlay();

	AddMappingContext();
}

// Called every frame
void ATestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATestCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	AddMappingContext();
}

// Called to bind functionality to input
void ATestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestCharacter::Move);
		}

		if (MoveUpAction)
		{
			EnhancedInputComponent->BindAction(MoveUpAction, ETriggerEvent::Triggered, this, &ATestCharacter::MoveUp);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestCharacter::Look);
		}
	}
}

UPawnMovementComponent* ATestCharacter::GetMovementComponent() const
{
	return MovementComponent;
}

void ATestCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const float ControlYaw = Controller->GetControlRotation().Yaw;

	if (IsLocallyControlled())
	{
		ApplyMove(MovementVector, ControlYaw);
	}

	if (!HasAuthority())
	{
		ServerMove(MovementVector, ControlYaw);
	}
}

void ATestCharacter::MoveUp(const FInputActionValue& Value)
{
	const float AxisValue = Value.GetMagnitude();

	if (IsLocallyControlled())
	{
		ApplyMoveUp(AxisValue);
	}

	if (!HasAuthority())
	{
		ServerMoveUp(AxisValue);
	}
}

void ATestCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void ATestCharacter::ServerMove_Implementation(FVector2D MovementVector, float ControlYaw)
{
	ApplyServerMove(MovementVector, ControlYaw);
}

void ATestCharacter::ServerMoveUp_Implementation(float AxisValue)
{
	ApplyServerMoveUp(AxisValue);
}

void ATestCharacter::ApplyMove(FVector2D MovementVector, float ControlYaw)
{
	MovementVector.X = FMath::Clamp(MovementVector.X, -1.0f, 1.0f);
	MovementVector.Y = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);

	const FRotator YawRotation(0.0f, ControlYaw, 0.0f);

	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void ATestCharacter::ApplyMoveUp(float AxisValue)
{
	AddMovementInput(FVector::UpVector, FMath::Clamp(AxisValue, -1.0f, 1.0f));
}

void ATestCharacter::ApplyServerMove(FVector2D MovementVector, float ControlYaw)
{
	MovementVector.X = FMath::Clamp(MovementVector.X, -1.0f, 1.0f);
	MovementVector.Y = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);

	if (MovementVector.IsNearlyZero())
	{
		return;
	}

	if (MovementVector.SizeSquared() > 1.0f)
	{
		MovementVector.Normalize();
	}

	const FRotator YawRotation(0.0f, ControlYaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector MoveDirection = ((Forward * MovementVector.Y) + (Right * MovementVector.X)).GetSafeNormal();
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;

	AddActorWorldOffset(MoveDirection * ServerMoveSpeed * DeltaSeconds, true);
	ForceNetUpdate();
}

void ATestCharacter::ApplyServerMoveUp(float AxisValue)
{
	AxisValue = FMath::Clamp(AxisValue, -1.0f, 1.0f);
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;

	AddActorWorldOffset(FVector::UpVector * AxisValue * ServerMoveSpeed * DeltaSeconds, true);
	ForceNetUpdate();
}

void ATestCharacter::AddMappingContext()
{
	if (!MappingContext || !IsLocallyControlled())
	{
		return;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}
