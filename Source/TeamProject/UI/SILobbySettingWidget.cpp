// SILobbySettingWidget.cpp

#include "UI/SILobbySettingWidget.h"
#include "PlayerState/SIPlayerState.h"

#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/Button.h"
#include "Components/EditableText.h"

void USILobbySettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	HandleHostStatusChanged(bIsLocalPlayerHost);
	ResolveHostStatus();
	
	if (Button_GameStart)
	{
		Button_GameStart->OnClicked.AddDynamic(this, &USILobbySettingWidget::RequestStartGame);
	}
}

void USILobbySettingWidget::NativeDestruct()
{
	if (Button_GameStart)
	{
		Button_GameStart->OnClicked.RemoveDynamic(this, &USILobbySettingWidget::RequestStartGame);
	}
	
	if (CachedPlayerState.IsValid())
	{
		CachedPlayerState->OnHostStatusChanged.RemoveDynamic(this, &USILobbySettingWidget::HandleHostStatusChanged);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostResolveRetryTimer);
	}

	Super::NativeDestruct();
}

void USILobbySettingWidget::RequestStartGame()
{
	if (!bIsLocalPlayerHost)
	{
		return;
	}
	
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	PS->Server_RequestStartGame();
}

void USILobbySettingWidget::HandleHostStatusChanged(bool bNewIsHost)
{
	bIsLocalPlayerHost = bNewIsHost;
	
	if (IsValid(Button_GameStart))
	{
		if (bIsLocalPlayerHost)
		{
			Button_GameStart->SetVisibility(ESlateVisibility::Visible);
			Button_GameStart->SetIsEnabled(true);
		}
		else
		{
			Button_GameStart->SetVisibility(ESlateVisibility::Collapsed);
			Button_GameStart->SetIsEnabled(false);
		}
	}
}

void USILobbySettingWidget::ResolveHostStatus()
{
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();
	if (!IsValid(PS))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				HostResolveRetryTimer, this, &USILobbySettingWidget::ResolveHostStatus, 0.1f, false);
		}
		return;
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostResolveRetryTimer);
	}

	CachedPlayerState = PS;

	PS->OnHostStatusChanged.AddDynamic(this, &USILobbySettingWidget::HandleHostStatusChanged);
	HandleHostStatusChanged(PS->bIsHost);
}
