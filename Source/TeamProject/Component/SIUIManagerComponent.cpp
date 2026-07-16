// SIUIManagerComponent.cpp


#include "Component/SIUIManagerComponent.h"

#include "UI/SIUserWidget.h"
#include "GameFramework/PlayerController.h"

USIUIManagerComponent::USIUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USIUIManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USIUIManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USIUIManagerComponent::OnWidgetConfirmed()
{
	if (WidgetStack.IsEmpty())
	{
		return;
	}

	OnUIConfirmed.Broadcast(WidgetStack.Last().UIType);
}

void USIUIManagerComponent::OpenWidget(EUIType Type)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	TSubclassOf<USIUserWidget>* FoundClass = WidgetClasses.Find(Type);

	if (!FoundClass || !*FoundClass)
	{
		return;
	}

	if (WidgetStack.ContainsByPredicate(
		[Type](const FUIStackEntry& Entry)
			{
			return Entry.UIType == Type;
			}
		))
	{
		return;
	}

	TObjectPtr<USIUserWidget> NewWidget = CreateWidget<USIUserWidget>(PC, *FoundClass);
	
	if (!IsValid(NewWidget))
	{
		return;
	}

	NewWidget->AddToViewport();
	NewWidget->OnConfirmed.AddDynamic(this, &USIUIManagerComponent::OnWidgetConfirmed);
	NewWidget->OnCancelled.AddDynamic(this, &USIUIManagerComponent::OnWidgetCancelled);

	if (Type == EUIType::CreateRoom)
	{
		NewWidget->OnConfirmed.AddDynamic(this, &USIUIManagerComponent::HandleCreateRoomRequested);
	}

	FUIStackEntry UIStackEntry;

	UIStackEntry.UIType = Type;
	UIStackEntry.Widget = NewWidget;

	WidgetStack.Add(UIStackEntry);
}

void USIUIManagerComponent::CloseWidget()
{
	if (WidgetStack.IsEmpty())
	{
		return;
	}

	TObjectPtr<USIUserWidget> TargetWidget = WidgetStack.Last().Widget;

	if (!IsValid(TargetWidget))
	{
		return;
	}

	TargetWidget->OnConfirmed.RemoveDynamic(this, &USIUIManagerComponent::OnWidgetConfirmed);
	TargetWidget->OnCancelled.RemoveDynamic(this, &USIUIManagerComponent::OnWidgetCancelled);

	if (WidgetStack.Last().UIType == EUIType::CreateRoom)
	{
		TargetWidget->OnConfirmed.RemoveDynamic(this, &USIUIManagerComponent::HandleCreateRoomRequested);
	}

	TargetWidget->RemoveFromParent();

	WidgetStack.Pop();
}

void USIUIManagerComponent::OnWidgetCancelled()
{
	CloseWidget();
}

void USIUIManagerComponent::HandleCreateRoomRequested()
{
	OnCreateRoomRequested.Broadcast();
}

