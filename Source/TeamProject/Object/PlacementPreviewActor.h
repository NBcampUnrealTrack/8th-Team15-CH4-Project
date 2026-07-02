// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlacementPreviewActor.generated.h"

class UDataTable;
class UStaticMeshComponent;

UCLASS()
class TEAMPROJECT_API APlacementPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	APlacementPreviewActor();

	bool SetPreviewShape(UDataTable* ShapeDefinitionTable, FName ShapeId);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
