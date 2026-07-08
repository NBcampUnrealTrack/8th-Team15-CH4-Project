// SIUIManagerComponent.cpp


#include "Component/SIUIManagerComponent.h"

#include "Blueprint/UserWidget.h"
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

void USIUIManagerComponent::OpenWidget(EUIType Type)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget>* FoundClass = WidgetClasses.Find(Type);

	if (!FoundClass || !*FoundClass)
	{
		return;
	}

	TObjectPtr<UUserWidget> NewWidget = CreateWidget<UUserWidget>(PC, *FoundClass);
	
	if (!IsValid(NewWidget))
	{
		return;
	}

	NewWidget->AddToViewport();

	WidgetStack.Add(NewWidget);
}

void USIUIManagerComponent::CloseWidget()
{
	if (WidgetStack.IsEmpty())
	{
		return;
	}

	TObjectPtr<UUserWidget> TargetWidget = WidgetStack.Last();

	if (!IsValid(TargetWidget))
	{
		return;
	}

	TargetWidget->RemoveFromParent();

	WidgetStack.Pop();
}

