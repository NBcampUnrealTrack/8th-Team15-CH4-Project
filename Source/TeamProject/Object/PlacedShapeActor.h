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

	bool SetPlacedShape(UDataTable* ShapeDefinitionTable, FName ShapeId);
	FName GetShapeId() const;
	bool IsBeingEdited() const;
	void SetBeingEdited(bool bInIsBeingEdited);
	void SetHovered(bool bHovered);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> EditTraceComponent;

private:
	UPROPERTY(Replicated)
	FName ReplicatedShapeId;

	UPROPERTY(Replicated)
	bool bIsBeingEdited;

	UPROPERTY(ReplicatedUsing = OnRep_ShapeVisuals)
	TObjectPtr<UStaticMesh> ReplicatedMesh;

	UPROPERTY(ReplicatedUsing = OnRep_ShapeVisuals)
	TObjectPtr<UMaterialInterface> ReplicatedMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_ShapeVisuals)
	FVector ReplicatedScale;

	UFUNCTION()
	void OnRep_ShapeVisuals();

	void ApplyShapeVisuals();
	void UpdateEditTraceBounds();
};
