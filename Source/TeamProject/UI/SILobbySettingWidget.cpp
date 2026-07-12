// SILobbySettingWidget.cpp

#include "UI/SILobbySettingWidget.h"
#include "PlayerState/SIPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void USILobbySettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveHostStatus();
}

void USILobbySettingWidget::RequestStartGame()
{
	if (!bIsLocalPlayerHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySettingWidget] 방장이 아닌 플레이어의 게임 시작 요청 무시"));
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	ASIPlayerState* PS = PC->GetPlayerState<ASIPlayerState>();
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySettingWidget] SIPlayerState 미확보"));
		return;
	}

	PS->Server_RequestStartGame();
}

void USILobbySettingWidget::ResolveHostStatus()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	ASIPlayerState* PS = PC->GetPlayerState<ASIPlayerState>();
	if (!PS)
	{
		// PlayerState 아직 replicate 안 됐을 수 있음 - 짧게 재시도
		if (UWorld* World = GetWorld())
		{
			FTimerHandle RetryHandle;
			World->GetTimerManager().SetTimer(RetryHandle, this, &USILobbySettingWidget::ResolveHostStatus, 0.2f, false);
		}
		return;
	}

	bIsLocalPlayerHost = PS->bIsHost;
	OnHostStatusResolved(bIsLocalPlayerHost);
}
