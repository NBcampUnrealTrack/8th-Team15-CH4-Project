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

	// 프리뷰와 실제 배치에 사용할 도형 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	TObjectPtr<UStaticMesh> Mesh;

	// 프리뷰 상태에서 사용할 반투명 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	// 실제 배치된 도형에 사용할 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	TObjectPtr<UMaterialInterface> PlacedMaterial;

	// 도형 선택 시 처음 적용되는 기본 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
	FVector DefaultScale = FVector::OneVector;
};
