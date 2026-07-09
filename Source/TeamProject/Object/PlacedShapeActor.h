// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlacedShapeActor.generated.h"

class UDataTable;
class UMaterialInterface;
class UBoxComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class TEAMPROJECT_API APlacedShapeActor : public AActor
{
	GENERATED_BODY()

public:
	APlacedShapeActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// DataTable 기본 스케일로 배치 도형 정보를 설정한다.
	bool SetPlacedShape(UDataTable* ShapeDefinitionTable, FName ShapeId);
	// 지정한 스케일을 유지한 채 배치 도형 정보를 설정한다.
	bool SetPlacedShape(UDataTable* ShapeDefinitionTable, FName ShapeId, const FVector& PlacedScale);
	// 현재 배치된 도형의 ShapeId를 반환한다.
	FName GetShapeId() const;
	// 다른 플레이어가 편집 중인지 확인한다.
	bool IsBeingEdited() const;
	// 편집 중 상태를 설정한다.
	void SetBeingEdited(bool bInIsBeingEdited);
	// 시선이 닿은 도형을 외곽선으로 표시하거나 해제한다.
	void SetHovered(bool bHovered);

protected:
	// 실제 배치 도형을 보여주는 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 편집 대상 선택을 위한 트레이스 충돌 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> EditTraceComponent;

private:
	// 클라이언트에 복제되는 도형 식별자
	UPROPERTY(Replicated)
	FName ReplicatedShapeId;

	// 편집 중 여부를 클라이언트에 복제한다.
	UPROPERTY(Replicated)
	bool bIsBeingEdited;

	// 클라이언트에 복제되는 실제 메시
	UPROPERTY(ReplicatedUsing = OnRep_ShapeVisuals)
	TObjectPtr<UStaticMesh> ReplicatedMesh;

	// 클라이언트에 복제되는 배치용 머티리얼
	UPROPERTY(ReplicatedUsing = OnRep_ShapeVisuals)
	TObjectPtr<UMaterialInterface> ReplicatedMaterial;

	// 클라이언트에 복제되는 배치 스케일
	UPROPERTY(ReplicatedUsing = OnRep_ShapeVisuals)
	FVector ReplicatedScale;

	UFUNCTION()
	void OnRep_ShapeVisuals();

	// 복제된 메시, 머티리얼, 스케일을 실제 컴포넌트에 적용한다.
	void ApplyShapeVisuals();
	// 메시 크기에 맞춰 편집 트레이스 박스를 갱신한다.
	void UpdateEditTraceBounds();
};
