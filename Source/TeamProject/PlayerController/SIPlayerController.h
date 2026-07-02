// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SIPlayerController.generated.h"

class APlacedShapeActor;
class APlacementPreviewActor;
class UDataTable;

/**
 * 
 */
UCLASS()
class TEAMPROJECT_API ASIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASIPlayerController();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartShapePreview(FName ShapeId);

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartBoxPreview();

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartSpherePreview();

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartCylinderPreview();

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartConePreview();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UDataTable> ShapeDefinitionTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<APlacementPreviewActor> PreviewActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<APlacedShapeActor> PlacedShapeActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float PreviewDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float MinPreviewDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float MaxPreviewDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float PreviewDistanceStep;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float EditTraceDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	float MaxEditDistance;

private:
	UPROPERTY()
	TObjectPtr<APlacementPreviewActor> PreviewActor;

	UPROPERTY()
	TObjectPtr<APlacedShapeActor> HoveredShape;

	FName CurrentPreviewShapeId;
	FQuat PreviewRotation;
	bool bIsEditingExistingShape;
	FName EditingOriginalShapeId;
	FTransform EditingOriginalTransform;

	void ConfirmPlacement();
	void CancelPreview();
	void IncreasePreviewDistance();
	void DecreasePreviewDistance();
	void UpdatePreviewTransform();
	void UpdateHoveredShape();
	void SetHoveredShape(APlacedShapeActor* NewHoveredShape);
	void StartEditPreview(FName ShapeId, const FTransform& PreviewTransform);
	void ClearPreview();

	UFUNCTION(Server, Reliable)
	void Server_RequestSpawnShape(FName ShapeId, FTransform SpawnTransform);

	UFUNCTION(Server, Reliable)
	void Server_RequestEditShape(APlacedShapeActor* TargetShape);

	UFUNCTION(Client, Reliable)
	void Client_StartEditShape(FName ShapeId, FTransform PreviewTransform);
	
};
