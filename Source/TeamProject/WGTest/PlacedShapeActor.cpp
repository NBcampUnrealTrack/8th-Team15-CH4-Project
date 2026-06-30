// Fill out your copyright notice in the Description page of Project Settings.


#include "WGTest/PlacedShapeActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "WGTest/ShapeDefinitionRow.h"

APlacedShapeActor::APlacedShapeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	ReplicatedScale = FVector::OneVector;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void APlacedShapeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

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

	ReplicatedMesh = ShapeDefinition->Mesh;
	ReplicatedMaterial = ShapeDefinition->PlacedMaterial;
	ReplicatedScale = ShapeDefinition->DefaultScale;

	ApplyShapeVisuals();
	return true;
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
	}

	if (ReplicatedMaterial)
	{
		MeshComponent->SetMaterial(0, ReplicatedMaterial);
	}

	SetActorScale3D(ReplicatedScale);
}
