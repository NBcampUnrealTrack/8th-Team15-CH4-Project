#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputState.h"
#include "SIPreviewTransformGizmoComponent.generated.h"

class AActor;
class FSITransformGizmoQueries;
class FSITransformGizmoTransactions;
class UCombinedTransformGizmo;
class UInteractiveToolsContext;
class UTransformProxy;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewGizmoTransformChanged, const FTransform&);

UCLASS(ClassGroup = (Object), meta = (BlueprintSpawnableComponent))
class TEAMPROJECT_API USIPreviewTransformGizmoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPreviewTransformGizmoComponent();

	virtual void UninitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 프리뷰에 필요한 Transform 기즈모를 표시한다.
	bool StartEditing(AActor* TargetActor, bool bEnableLocation);
	void StopEditing();
	void SynchronizeTargetTransform();

	bool IsEditing() const;
	bool IsInteracting() const;
	bool TryBeginMouseInteraction();
	void EndMouseInteraction();

	FOnPreviewGizmoTransformChanged OnTransformChanged;

private:
	bool EnsureToolsContext();
	bool UpdateViewAndInputState();
	APlayerController* GetOwningPlayerController() const;
	void HandleTransformChanged(UTransformProxy* Proxy, FTransform Transform);

	UPROPERTY(Transient)
	TObjectPtr<UInteractiveToolsContext> ToolsContext;

	UPROPERTY(Transient)
	TObjectPtr<UTransformProxy> TransformProxy;

	UPROPERTY(Transient)
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveTargetActor;

	TSharedPtr<FSITransformGizmoQueries> ContextQueriesAPI;
	TSharedPtr<FSITransformGizmoTransactions> ContextTransactionsAPI;

	FInputDeviceState CurrentMouseState;
	FVector2D PreviousMousePosition = FVector2D::ZeroVector;
};
