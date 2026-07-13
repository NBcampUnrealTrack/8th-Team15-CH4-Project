// SIHUDWidget.cpp

#include "UI/SIHUDWidget.h"
#include "GameState/SIGameState.h"

void USIHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ASIGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASIGameState>() : nullptr;
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUDWidget] GameState 미확보 - 델리게이트 바인딩 실패"));
		return;
	}
	
	CachedGameState = GS;
	
	GS->OnPhaseChanged.AddDynamic(this, &USIHUDWidget::HandlePhaseChanged);
	GS->OnTimeUpdated.AddDynamic(this, &USIHUDWidget::HandleTimeUpdated);
	//GS->OnPresenterChanged.AddDynamic(this, &USIHUDWidget::);
	GS->OnChatMessage.AddDynamic(this, &USIHUDWidget::HandleChatMessage);
	
	// 초기 상태 즉시 반영 (이미 진행 중인 게임에 접속하는 케이스)
	HandlePhaseChanged(GS->CurrentGamePhase);
	HandleTimeUpdated(GS->RemainingTime);
}

void USIHUDWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPhaseChanged.RemoveDynamic(this, &USIHUDWidget::HandlePhaseChanged);
		CachedGameState->OnTimeUpdated.RemoveDynamic(this, &USIHUDWidget::HandleTimeUpdated);
		//CachedGameState->OnPresenterChanged.RemoveDynamic(this, &USIHUDWidget::);
		CachedGameState->OnChatMessage.RemoveDynamic(this, &USIHUDWidget::HandleChatMessage);
	}

	Super::NativeDestruct();
}

void USIHUDWidget::SetSecretWord(const FString& NewSecretWord)
{
	CurrentSecretWord = NewSecretWord;
	OnHUDSecretWordReceived(NewSecretWord);
}

void USIHUDWidget::HandlePhaseChanged(ESIGamePhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnHUDPhaseChanged(NewPhase);
}

void USIHUDWidget::HandleTimeUpdated(int32 NewTime)
{
	RemainingTime = NewTime;
	OnHUDTimeUpdated(NewTime);
}

void USIHUDWidget::HandleChatMessage(const FChatMessagePayload& Payload)
{
	OnHUDChatMessage(Payload);
}
