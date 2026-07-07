// SIUIManagerComponent.cpp


#include "Component/SIUIManagerComponent.h"
#include "Blueprint/UserWidget.h"

USIUIManagerComponent::USIUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void USIUIManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void USIUIManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

