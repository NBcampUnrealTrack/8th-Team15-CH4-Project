#include "Object/Component/SIPreviewTransformGizmoComponent.h"

#include "BaseGizmos/CombinedTransformGizmo.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "BaseGizmos/GizmoBoxComponent.h"
#include "BaseGizmos/GizmoRectangleComponent.h"
#include "BaseGizmos/GizmoViewContext.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "ContextObjectStore.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InteractiveGizmoManager.h"
#include "InputRouter.h"
#include "InteractiveToolManager.h"
#include "InteractiveToolsContext.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "SceneView.h"
#include "ToolContextInterfaces.h"

namespace
{
	constexpr float ScaleHandleSizeMultiplier = 1.5f;
	constexpr float ScaleHandleHitDistance = 10.0f;

	void EnlargeScaleHandle(UPrimitiveComponent* PrimitiveComponent)
	{
		UGizmoBaseComponent* BaseComponent = Cast<UGizmoBaseComponent>(PrimitiveComponent);
		if (!BaseComponent)
		{
			return;
		}

		BaseComponent->PixelHitDistanceThreshold = ScaleHandleHitDistance;

		if (UGizmoRectangleComponent* RectangleComponent = Cast<UGizmoRectangleComponent>(BaseComponent))
		{
			RectangleComponent->LengthX *= ScaleHandleSizeMultiplier;
			RectangleComponent->LengthY *= ScaleHandleSizeMultiplier;
			RectangleComponent->Thickness *= ScaleHandleSizeMultiplier;
		}
		else if (UGizmoBoxComponent* BoxComponent = Cast<UGizmoBoxComponent>(BaseComponent))
		{
			BoxComponent->Dimensions *= ScaleHandleSizeMultiplier;
			BoxComponent->LineThickness *= ScaleHandleSizeMultiplier;
		}

		BaseComponent->NotifyExternalPropertyUpdates();
	}

	void EnlargeScaleHandles(ACombinedTransformGizmoActor* GizmoActor)
	{
		if (!GizmoActor)
		{
			return;
		}

		EnlargeScaleHandle(GizmoActor->UniformScale);
		EnlargeScaleHandle(GizmoActor->AxisScaleX);
		EnlargeScaleHandle(GizmoActor->AxisScaleY);
		EnlargeScaleHandle(GizmoActor->AxisScaleZ);
		EnlargeScaleHandle(GizmoActor->PlaneScaleYZ);
		EnlargeScaleHandle(GizmoActor->PlaneScaleXZ);
		EnlargeScaleHandle(GizmoActor->PlaneScaleXY);
	}
}

class FSITransformGizmoQueries : public IToolsContextQueriesAPI
{
public:
	FSITransformGizmoQueries(UInteractiveToolsContext* InToolsContext, USIPreviewTransformGizmoComponent* InComponent)
		: ToolsContext(InToolsContext)
		, Component(InComponent)
	{
	}

	void SetActiveViewport(FViewport* InViewport)
	{
		ActiveViewport = InViewport;
	}

	virtual UWorld* GetCurrentEditingWorld() const override
	{
		return ToolsContext ? ToolsContext->GetWorld() : nullptr;
	}

	virtual void GetCurrentSelectionState(FToolBuilderState& StateOut) const override
	{
		if (!ToolsContext)
		{
			return;
		}

		StateOut.ToolManager = ToolsContext->ToolManager;
		StateOut.TargetManager = ToolsContext->TargetManager;
		StateOut.GizmoManager = ToolsContext->GizmoManager;
		StateOut.World = ToolsContext->GetWorld();
	}

	virtual void GetCurrentViewState(FViewCameraState& StateOut) const override
	{
		const ACharacter* OwnerCharacter = Component ? Cast<ACharacter>(Component->GetOwner()) : nullptr;
		APlayerController* PlayerController = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
		if (!PlayerController)
		{
			return;
		}

		FVector ViewLocation;
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

		StateOut.Position = ViewLocation;
		StateOut.Orientation = ViewRotation.Quaternion();
		StateOut.HorizontalFOVDegrees = PlayerController->PlayerCameraManager
			? PlayerController->PlayerCameraManager->GetFOVAngle()
			: 90.0f;
		StateOut.AspectRatio = ActiveViewport && ActiveViewport->GetSizeXY().Y > 0
			? static_cast<float>(ActiveViewport->GetSizeXY().X) / static_cast<float>(ActiveViewport->GetSizeXY().Y)
			: 1.0f;
		StateOut.OrthoWorldCoordinateWidth = 1.0f;
		StateOut.bIsOrthographic = false;
		StateOut.bIsVR = false;
	}

	virtual EToolContextCoordinateSystem GetCurrentCoordinateSystem() const override
	{
		return EToolContextCoordinateSystem::World;
	}

	virtual EToolContextTransformGizmoMode GetCurrentTransformGizmoMode() const override
	{
		return EToolContextTransformGizmoMode::Combined;
	}

	virtual FToolContextSnappingConfiguration GetCurrentSnappingSettings() const override
	{
		return FToolContextSnappingConfiguration();
	}

	virtual UMaterialInterface* GetStandardMaterial(EStandardToolContextMaterials MaterialType) const override
	{
		return UMaterial::GetDefaultMaterial(MD_Surface);
	}

	virtual FViewport* GetHoveredViewport() const override
	{
		return ActiveViewport;
	}

	virtual FViewport* GetFocusedViewport() const override
	{
		return ActiveViewport;
	}

private:
	UInteractiveToolsContext* ToolsContext = nullptr;
	USIPreviewTransformGizmoComponent* Component = nullptr;
	FViewport* ActiveViewport = nullptr;
};

class FSITransformGizmoTransactions : public IToolsContextTransactionsAPI
{
public:
	virtual void DisplayMessage(const FText& Message, EToolMessageLevel Level) override {}
	virtual void PostInvalidation() override {}
	virtual void BeginUndoTransaction(const FText& Description) override {}
	virtual void EndUndoTransaction() override {}
	virtual void AppendChange(UObject* TargetObject, TUniquePtr<FToolCommandChange> Change, const FText& Description) override {}
	virtual bool RequestSelectionChange(const FSelectedObjectsChangeList& SelectionChange) override { return false; }
};

USIPreviewTransformGizmoComponent::USIPreviewTransformGizmoComponent()
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USIPreviewTransformGizmoComponent::UninitializeComponent()
{
	StopEditing();

	if (ToolsContext)
	{
		UE::TransformGizmoUtil::DeregisterTransformGizmoContextObject(ToolsContext);
		ToolsContext->Shutdown();
		ToolsContext = nullptr;
	}

	ContextQueriesAPI.Reset();
	ContextTransactionsAPI.Reset();
	Super::UninitializeComponent();
}

void USIPreviewTransformGizmoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsEditing() || !ToolsContext || !UpdateViewAndInputState())
	{
		return;
	}

	if (IsInteracting())
	{
		ToolsContext->InputRouter->PostInputEvent(CurrentMouseState);
	}
	else
	{
		ToolsContext->InputRouter->PostHoverInputEvent(CurrentMouseState);
	}

	ToolsContext->ToolManager->Tick(DeltaTime);
	ToolsContext->GizmoManager->Tick(DeltaTime);
}

bool USIPreviewTransformGizmoComponent::StartEditing(AActor* TargetActor, bool bEnableLocation)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!TargetActor || !TargetActor->GetRootComponent() || !OwnerCharacter || !OwnerCharacter->IsLocallyControlled()
		|| !EnsureToolsContext())
	{
		return false;
	}

	StopEditing();
	if (!UpdateViewAndInputState())
	{
		return false;
	}

	ActiveTargetActor = TargetActor;
	TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->AddComponent(TargetActor->GetRootComponent(), false);
	TransformProxy->OnTransformChanged.AddUObject(this, &USIPreviewTransformGizmoComponent::HandleTransformChanged);

	ETransformGizmoSubElements GizmoElements =
		ETransformGizmoSubElements::RotateAllAxes |
		ETransformGizmoSubElements::ScaleAllAxes |
		ETransformGizmoSubElements::ScaleAllPlanes |
		ETransformGizmoSubElements::ScaleUniform;

	if (bEnableLocation)
	{
		GizmoElements |= ETransformGizmoSubElements::TranslateAllAxes;
		GizmoElements |= ETransformGizmoSubElements::TranslateAllPlanes;
	}

	TransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(
		ToolsContext->GizmoManager,
		GizmoElements,
		this,
		bEnableLocation ? TEXT("SelectedShapeGizmo") : TEXT("PreviewShapeGizmo"));

	if (!TransformGizmo)
	{
		StopEditing();
		return false;
	}

	TransformGizmo->SetDisallowNegativeScaling(true);
	TransformGizmo->SetActiveTarget(TransformProxy);
	TransformGizmo->SetVisibility(true);
	if (ACombinedTransformGizmoActor* GizmoActor = TransformGizmo->GetGizmoActor())
	{
		EnlargeScaleHandles(GizmoActor);
		GizmoActor->SetActorHiddenInGame(false);
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(GizmoActor);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			PrimitiveComponent->SetHiddenInGame(false);
			PrimitiveComponent->SetVisibility(true);
			PrimitiveComponent->MarkRenderStateDirty();
		}
	}
	ToolsContext->GizmoManager->Tick(0.0f);
	return true;
}

void USIPreviewTransformGizmoComponent::StopEditing()
{
	if (TransformProxy)
	{
		TransformProxy->OnTransformChanged.RemoveAll(this);
	}

	if (ToolsContext && ToolsContext->GizmoManager)
	{
		ToolsContext->GizmoManager->DestroyAllGizmosByOwner(this);
	}

	TransformGizmo = nullptr;
	TransformProxy = nullptr;
	ActiveTargetActor = nullptr;
	CurrentMouseState = FInputDeviceState();
	PreviousMousePosition = FVector2D::ZeroVector;
}

void USIPreviewTransformGizmoComponent::SynchronizeTargetTransform()
{
	if (IsEditing() && !IsInteracting() && IsValid(ActiveTargetActor))
	{
		TransformGizmo->ReinitializeGizmoTransform(ActiveTargetActor->GetActorTransform(), true);
	}
}

bool USIPreviewTransformGizmoComponent::IsEditing() const
{
	return IsValid(ActiveTargetActor) && TransformGizmo != nullptr;
}

bool USIPreviewTransformGizmoComponent::IsInteracting() const
{
	return ToolsContext && ToolsContext->InputRouter && ToolsContext->InputRouter->HasActiveMouseCapture();
}

bool USIPreviewTransformGizmoComponent::TryBeginMouseInteraction()
{
	if (!IsEditing() || !UpdateViewAndInputState())
	{
		return false;
	}

	CurrentMouseState.Mouse.Left.SetStates(true, false, false);
	ToolsContext->InputRouter->PostInputEvent(CurrentMouseState);

	const bool bCaptured = IsInteracting();
	CurrentMouseState.Mouse.Left.SetStates(false, bCaptured, false);
	return bCaptured;
}

void USIPreviewTransformGizmoComponent::EndMouseInteraction()
{
	if (!IsEditing() || !IsInteracting() || !UpdateViewAndInputState())
	{
		return;
	}

	CurrentMouseState.Mouse.Left.SetStates(false, false, true);
	ToolsContext->InputRouter->PostInputEvent(CurrentMouseState);
	CurrentMouseState.Mouse.Left.SetStates(false, false, false);
}

bool USIPreviewTransformGizmoComponent::EnsureToolsContext()
{
	if (ToolsContext)
	{
		return true;
	}

	ToolsContext = NewObject<UInteractiveToolsContext>(this);
	ContextQueriesAPI = MakeShared<FSITransformGizmoQueries>(ToolsContext, this);
	ContextTransactionsAPI = MakeShared<FSITransformGizmoTransactions>();
	ToolsContext->Initialize(ContextQueriesAPI.Get(), ContextTransactionsAPI.Get());

	if (!UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(ToolsContext))
	{
		ToolsContext->Shutdown();
		ToolsContext = nullptr;
		ContextQueriesAPI.Reset();
		ContextTransactionsAPI.Reset();
		return false;
	}

	return true;
}

bool USIPreviewTransformGizmoComponent::UpdateViewAndInputState()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!PlayerController || !ViewportClient || !ViewportClient->Viewport || !LocalPlayer)
	{
		return false;
	}

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		GetWorld()->Scene,
		*ViewportClient->GetEngineShowFlags())
		.SetRealtimeUpdate(true));

	FVector ViewLocation;
	FRotator ViewRotation;
	FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, ViewportClient->Viewport);
	if (!SceneView)
	{
		return false;
	}

	if (UGizmoViewContext* GizmoViewContext = ToolsContext->ContextObjectStore->FindContext<UGizmoViewContext>())
	{
		GizmoViewContext->ResetFromSceneView(*SceneView);
	}

	ContextQueriesAPI->SetActiveViewport(ViewportClient->Viewport);

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->GetMousePosition(MouseX, MouseY)
		|| !PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection))
	{
		return false;
	}

	CurrentMouseState.InputDevice = EInputDevices::Mouse;
	CurrentMouseState.Mouse.Position2D = FVector2D(MouseX, MouseY);
	CurrentMouseState.Mouse.Delta2D = CurrentMouseState.Mouse.Position2D - PreviousMousePosition;
	CurrentMouseState.Mouse.WorldRay = FRay(RayOrigin, RayDirection.GetSafeNormal(), true);
	PreviousMousePosition = CurrentMouseState.Mouse.Position2D;
	return true;
}

APlayerController* USIPreviewTransformGizmoComponent::GetOwningPlayerController() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	return OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
}

void USIPreviewTransformGizmoComponent::HandleTransformChanged(UTransformProxy* Proxy, FTransform Transform)
{
	if (IsValid(ActiveTargetActor))
	{
		OnTransformChanged.Broadcast(Transform);
	}
}
