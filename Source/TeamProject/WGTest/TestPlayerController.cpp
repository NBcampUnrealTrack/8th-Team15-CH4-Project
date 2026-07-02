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
	EditTraceDistance = 3000.0f;
	MaxEditDistance = 3000.0f;
	PreviewYaw = 0.0f;
	bIsRotatingCounterClockwise = false;
	bIsRotatingClockwise = false;
	bIsEditingExistingShape = false;
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
	UpdateHoveredShape();
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
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATestPlayerController::CancelPreview);
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
		SetHoveredShape(nullptr);
		CurrentPreviewShapeId = ShapeId;
		bIsEditingExistingShape = false;
		EditingOriginalShapeId = NAME_None;
		EditingOriginalTransform = FTransform::Identity;
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
	if (PreviewActor && !CurrentPreviewShapeId.IsNone())
	{
		Server_RequestSpawnShape(CurrentPreviewShapeId, PreviewActor->GetActorTransform());
		ClearPreview();
		return;
	}

	Server_RequestEditShape(HoveredShape);
}

void ATestPlayerController::CancelPreview()
{
	if (!PreviewActor || CurrentPreviewShapeId.IsNone())
	{
		return;
	}

	if (bIsEditingExistingShape && !EditingOriginalShapeId.IsNone())
	{
		Server_RequestSpawnShape(EditingOriginalShapeId, EditingOriginalTransform);
	}

	ClearPreview();
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

void ATestPlayerController::UpdateHoveredShape()
{
	if (PreviewActor && !CurrentPreviewShapeId.IsNone())
	{
		SetHoveredShape(nullptr);
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * EditTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UpdateHoveredShape), false, GetPawn());

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

void ATestPlayerController::SetHoveredShape(APlacedShapeActor* NewHoveredShape)
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

void ATestPlayerController::StartEditPreview(FName ShapeId, const FTransform& PreviewTransform)
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
		PreviewYaw = PreviewTransform.GetRotation().Rotator().Yaw;

		FVector ViewLocation;
		FRotator ViewRotation;
		GetPlayerViewPoint(ViewLocation, ViewRotation);

		const float ProjectedDistance = FVector::DotProduct(PreviewTransform.GetLocation() - ViewLocation, ViewRotation.Vector());
		PreviewDistance = FMath::Clamp(ProjectedDistance, MinPreviewDistance, MaxPreviewDistance);

		PreviewActor->SetActorTransform(PreviewTransform);
	}
}

void ATestPlayerController::ClearPreview()
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	CurrentPreviewShapeId = NAME_None;
	PreviewYaw = 0.0f;
	bIsRotatingCounterClockwise = false;
	bIsRotatingClockwise = false;
	bIsEditingExistingShape = false;
	EditingOriginalShapeId = NAME_None;
	EditingOriginalTransform = FTransform::Identity;
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

void ATestPlayerController::Server_RequestEditShape_Implementation(APlacedShapeActor* TargetShape)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

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

void ATestPlayerController::Client_StartEditShape_Implementation(FName ShapeId, FTransform PreviewTransform)
{
	StartEditPreview(ShapeId, PreviewTransform);
}
