// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SIPlayerController.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Object/PlacedShapeActor.h"
#include "Object/PlacementPreviewActor.h"
#include "Object/ShapeDefinitionRow.h"

ASIPlayerController::ASIPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

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

void ASIPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ASIPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsLocalController())
	{
		return;
	}

	UpdatePreviewTransform();
	UpdateHoveredShape();
}

void ASIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 도형 프리뷰 선택
	// 프리뷰 거리 조절
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ASIPlayerController::IncreasePreviewDistance);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ASIPlayerController::DecreasePreviewDistance);

	// 설치 / 프리뷰 취소
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASIPlayerController::ConfirmPlacement);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASIPlayerController::CancelPreview);
}

void ASIPlayerController::StartShapePreview(FName ShapeId)
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

void ASIPlayerController::StartBoxPreview()
{
	StartShapePreview(TEXT("Box"));
}

void ASIPlayerController::StartSpherePreview()
{
	StartShapePreview(TEXT("Sphere"));
}

void ASIPlayerController::StartCylinderPreview()
{
	StartShapePreview(TEXT("Cylinder"));
}

void ASIPlayerController::StartConePreview()
{
	StartShapePreview(TEXT("Cone"));
}

void ASIPlayerController::ConfirmPlacement()
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

void ASIPlayerController::CancelPreview()
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

void ASIPlayerController::IncreasePreviewDistance()
{
	PreviewDistance = FMath::Clamp(PreviewDistance + PreviewDistanceStep, MinPreviewDistance, MaxPreviewDistance);
	UpdatePreviewTransform();
}

void ASIPlayerController::DecreasePreviewDistance()
{
	PreviewDistance = FMath::Clamp(PreviewDistance - PreviewDistanceStep, MinPreviewDistance, MaxPreviewDistance);
	UpdatePreviewTransform();
}

void ASIPlayerController::UpdatePreviewTransform()
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
	PreviewActor->SetActorRotation(PreviewRotation);
}

void ASIPlayerController::UpdateHoveredShape()
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

void ASIPlayerController::SetHoveredShape(APlacedShapeActor* NewHoveredShape)
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

void ASIPlayerController::StartEditPreview(FName ShapeId, const FTransform& PreviewTransform)
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
		GetPlayerViewPoint(ViewLocation, ViewRotation);

		const float ProjectedDistance = FVector::DotProduct(PreviewTransform.GetLocation() - ViewLocation, ViewRotation.Vector());
		PreviewDistance = FMath::Clamp(ProjectedDistance, MinPreviewDistance, MaxPreviewDistance);

		PreviewActor->SetActorTransform(PreviewTransform);
	}
}

void ASIPlayerController::ClearPreview()
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

void ASIPlayerController::Server_RequestSpawnShape_Implementation(FName ShapeId, FTransform SpawnTransform)
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

void ASIPlayerController::Server_RequestEditShape_Implementation(APlacedShapeActor* TargetShape)
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

void ASIPlayerController::Client_StartEditShape_Implementation(FName ShapeId, FTransform PreviewTransform)
{
	StartEditPreview(ShapeId, PreviewTransform);
}
