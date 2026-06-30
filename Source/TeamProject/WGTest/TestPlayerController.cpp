// Fill out your copyright notice in the Description page of Project Settings.


#include "WGTest/TestPlayerController.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "WGTest/PlacedShapeActor.h"
#include "WGTest/PlacementPreviewActor.h"
#include "WGTest/ShapeDefinitionRow.h"

ATestPlayerController::ATestPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PreviewDistance = 600.0f;
	MinPreviewDistance = 150.0f;
	MaxPreviewDistance = 2000.0f;
	PreviewDistanceStep = 100.0f;
	PreviewRotationStep = 90.0f;
	PreviewYaw = 0.0f;
	bIsRotatingCounterClockwise = false;
	bIsRotatingClockwise = false;
	PreviewActorClass = APlacementPreviewActor::StaticClass();
	PlacedShapeActorClass = APlacedShapeActor::StaticClass();
}

void ATestPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ATestPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePreviewRotation(DeltaTime);
	UpdatePreviewTransform();
}

void ATestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ATestPlayerController::StartBoxPreview);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ATestPlayerController::StartSpherePreview);
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ATestPlayerController::IncreasePreviewDistance);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ATestPlayerController::DecreasePreviewDistance);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ATestPlayerController::StartRotatePreviewCounterClockwise);
	InputComponent->BindKey(EKeys::Q, IE_Released, this, &ATestPlayerController::StopRotatePreviewCounterClockwise);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ATestPlayerController::StartRotatePreviewClockwise);
	InputComponent->BindKey(EKeys::E, IE_Released, this, &ATestPlayerController::StopRotatePreviewClockwise);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATestPlayerController::ConfirmPlacement);
}

void ATestPlayerController::StartShapePreview(FName ShapeId)
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
		CurrentPreviewShapeId = ShapeId;
		UpdatePreviewTransform();
	}
}

void ATestPlayerController::StartBoxPreview()
{
	StartShapePreview(TEXT("Box"));
}

void ATestPlayerController::StartSpherePreview()
{
	StartShapePreview(TEXT("Sphere"));
}

void ATestPlayerController::ConfirmPlacement()
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	Server_RequestSpawnShape(CurrentPreviewShapeId, PreviewActor->GetActorTransform());
}

void ATestPlayerController::IncreasePreviewDistance()
{
	PreviewDistance = FMath::Clamp(PreviewDistance + PreviewDistanceStep, MinPreviewDistance, MaxPreviewDistance);
	UpdatePreviewTransform();
}

void ATestPlayerController::DecreasePreviewDistance()
{
	PreviewDistance = FMath::Clamp(PreviewDistance - PreviewDistanceStep, MinPreviewDistance, MaxPreviewDistance);
	UpdatePreviewTransform();
}

void ATestPlayerController::StartRotatePreviewCounterClockwise()
{
	bIsRotatingCounterClockwise = true;
}

void ATestPlayerController::StopRotatePreviewCounterClockwise()
{
	bIsRotatingCounterClockwise = false;
}

void ATestPlayerController::StartRotatePreviewClockwise()
{
	bIsRotatingClockwise = true;
}

void ATestPlayerController::StopRotatePreviewClockwise()
{
	bIsRotatingClockwise = false;
}

void ATestPlayerController::UpdatePreviewRotation(float DeltaTime)
{
	const float RotationDirection = (bIsRotatingClockwise ? 1.0f : 0.0f) - (bIsRotatingCounterClockwise ? 1.0f : 0.0f);
	if (FMath::IsNearlyZero(RotationDirection))
	{
		return;
	}

	PreviewYaw = FRotator::NormalizeAxis(PreviewYaw + (RotationDirection * PreviewRotationStep * DeltaTime));
}

void ATestPlayerController::UpdatePreviewTransform()
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * PreviewDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UpdatePlacementPreview), false, GetPawn());

	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const FVector PreviewLocation = bHit ? HitResult.ImpactPoint : TraceEnd;

	PreviewActor->SetActorLocation(PreviewLocation);
	PreviewActor->SetActorRotation(FRotator(0.0f, PreviewYaw, 0.0f));
}

void ATestPlayerController::Server_RequestSpawnShape_Implementation(FName ShapeId, FTransform SpawnTransform)
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
	SpawnParameters.Instigator = GetPawn();

	APlacedShapeActor* PlacedShape = World->SpawnActor<APlacedShapeActor>(PlacedShapeActorClass, SpawnTransform, SpawnParameters);
	if (PlacedShape)
	{
		PlacedShape->SetPlacedShape(ShapeDefinitionTable, ShapeId);
	}
}
