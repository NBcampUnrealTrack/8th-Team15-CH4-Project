// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/PlacedShapeActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Object/ShapeDefinitionRow.h"

APlacedShapeActor::APlacedShapeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	bIsBeingEdited = false;
	ReplicatedScale = FVector::OneVector;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	EditTraceComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("EditTraceComponent"));
	EditTraceComponent->SetupAttachment(MeshComponent);
	EditTraceComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EditTraceComponent->SetCollisionObjectType(ECC_WorldDynamic);
	EditTraceComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	EditTraceComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	EditTraceComponent->SetBoxExtent(FVector(50.0f));
	EditTraceComponent->SetHiddenInGame(true);
}

void APlacedShapeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlacedShapeActor, ReplicatedShapeId);
	DOREPLIFETIME(APlacedShapeActor, bIsBeingEdited);
	DOREPLIFETIME(APlacedShapeActor, ReplicatedMesh);
	DOREPLIFETIME(APlacedShapeActor, ReplicatedMaterial);
	DOREPLIFETIME(APlacedShapeActor, ReplicatedScale);
}

bool APlacedShapeActor::SetPlacedShape(UDataTable* ShapeDefinitionTable, FName ShapeId)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone())
	{
		return false;
	}

	const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(ShapeId, TEXT("SetPlacedShape"));
	if (!ShapeDefinition || !ShapeDefinition->Mesh)
	{
		return false;
	}

	ReplicatedShapeId = ShapeId;
	ReplicatedMesh = ShapeDefinition->Mesh;
	ReplicatedMaterial = ShapeDefinition->PlacedMaterial;
	ReplicatedScale = ShapeDefinition->DefaultScale;

	ApplyShapeVisuals();
	return true;
}

FName APlacedShapeActor::GetShapeId() const
{
	return ReplicatedShapeId;
}

bool APlacedShapeActor::IsBeingEdited() const
{
	return bIsBeingEdited;
}

void APlacedShapeActor::SetBeingEdited(bool bInIsBeingEdited)
{
	bIsBeingEdited = bInIsBeingEdited;
}

void APlacedShapeActor::SetHovered(bool bHovered)
{
	MeshComponent->SetRenderCustomDepth(bHovered);
}

void APlacedShapeActor::OnRep_ShapeVisuals()
{
	ApplyShapeVisuals();
}

void APlacedShapeActor::ApplyShapeVisuals()
{
	if (ReplicatedMesh)
	{
		MeshComponent->SetStaticMesh(ReplicatedMesh);
		UpdateEditTraceBounds();
	}

	if (ReplicatedMaterial)
	{
		MeshComponent->SetMaterial(0, ReplicatedMaterial);
	}

	SetActorScale3D(ReplicatedScale);
}

void APlacedShapeActor::UpdateEditTraceBounds()
{
	if (!ReplicatedMesh || !EditTraceComponent)
	{
		return;
	}

	const FBoxSphereBounds MeshBounds = ReplicatedMesh->GetBounds();
	const FVector BoxExtent = MeshBounds.BoxExtent.ComponentMax(FVector(1.0f));

	EditTraceComponent->SetRelativeLocation(MeshBounds.Origin);
	EditTraceComponent->SetBoxExtent(BoxExtent);
}
