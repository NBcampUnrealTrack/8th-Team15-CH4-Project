// SIExitWidget.cpp


#include "UI/SIExitWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void USIExitWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &USIExitWidget::OnQuitClicked);
	}

	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &USIExitWidget::OnContinueClicked);
	}
}

void USIExitWidget::OnQuitClicked()
{
	// 로컬에서 게임 완전히 종료.
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void USIExitWidget::OnContinueClicked()
{
	HandleCancelled();
}