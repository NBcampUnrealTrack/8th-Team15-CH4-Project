// Fill out your copyright notice in the Description page of Project Settings.


#include "WGTest/PlacementPreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "WGTest/ShapeDefinitionRow.h"

APlacementPreviewActor::APlacementPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool APlacementPreviewActor::SetPreviewShape(UDataTable* ShapeDefinitionTable, FName ShapeId)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone())
	{
		return false;
	}

	const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(ShapeId, TEXT("SetPreviewShape"));
	if (!ShapeDefinition || !ShapeDefinition->Mesh)
	{
		return false;
	}

	MeshComponent->SetStaticMesh(ShapeDefinition->Mesh);
	if (ShapeDefinition->PreviewMaterial)
	{
		MeshComponent->SetMaterial(0, ShapeDefinition->PreviewMaterial);
	}

	SetActorScale3D(ShapeDefinition->DefaultScale);
	return true;
}
