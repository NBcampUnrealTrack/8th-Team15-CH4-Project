// UExitWidget.cpp


#include "UI/ExitWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void UExitWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &UExitWidget::OnQuitClicked);
	}

	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &UExitWidget::OnContinueClicked);
	}
}

void UExitWidget::OnQuitClicked()
{
	// 로컬에서 게임 완전히 종료.
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UExitWidget::OnContinueClicked()
{
	RemoveFromParent();
}