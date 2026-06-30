// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ShapeDefinitionRow.generated.h"

class UMaterialInterface;
class UStaticMesh;

USTRUCT(BlueprintType)
struct TEAMPROJECT_API FShapeDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	TObjectPtr<UMaterialInterface> PlacedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	FVector DefaultScale = FVector::OneVector;
};
