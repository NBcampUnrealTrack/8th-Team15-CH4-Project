// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlacementPreviewActor.generated.h"

class UDataTable;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class TEAMPROJECT_API APlacementPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	APlacementPreviewActor();

	// DataTable의 도형 정의를 사용해 프리뷰 메시와 머티리얼을 설정한다.
	bool SetPreviewShape(UDataTable* ShapeDefinitionTable, FName ShapeId);
	void SetPreviewColor(const FLinearColor& Color);
	// 표면 히트 지점과 법선을 기준으로, 프리뷰가 벽/바닥을 뚫지 않도록 위치를 보정한다.
	void SetSurfaceAlignedTransform(const FVector& SurfacePoint, const FVector& SurfaceNormal, const FRotator& PreviewRotation, const FVector& PreviewScale);

protected:
	// 로컬 플레이어에게만 보이는 프리뷰 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance;

	FLinearColor CurrentPreviewColor = FLinearColor::White;

	// 현재 회전/스케일이 반영된 메시 bounds가 표면 법선 방향으로 차지하는 반쪽 길이를 계산한다.
	float CalculateSurfaceOffset(const FVector& SurfaceNormal, const FRotator& PreviewRotation, const FVector& PreviewScale) const;
};
