// SIHUDWidget.cpp

#include "UI/SIHUDWidget.h"
#include "PlayerController/SIPlayerController.h"
#include "GameState/SIGameState.h"
#include "PlayerState/SIPlayerState.h"
#include "UI/SIChatLineWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableText.h"

void USIHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	TObjectPtr<ASIGameState> GS = GetWorld()->GetGameState<ASIGameState>();
	if (!IsValid(GS))
	{
		return;
	}
	
	CachedGameState = GS;
	
	GS->OnPhaseChanged.AddDynamic(this, &USIHUDWidget::HandlePhaseChanged);
	GS->OnTimeUpdated.AddDynamic(this, &USIHUDWidget::HandleTimeUpdated);
	GS->OnChatMessage.AddDynamic(this, &USIHUDWidget::HandleChatMessage);
	
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}
	
	CachedPlayerState = PS;
	PS->OnScoreUpdated.AddDynamic(this, &USIHUDWidget::HandleScoreUpdated);
	
	// 초기 상태 즉시 반영 (이미 진행 중인 게임에 접속하는 케이스)
	HandlePhaseChanged(GS->CurrentGamePhase);
	HandleTimeUpdated(GS->RemainingTime);
	HandleScoreUpdated(PS->CurrentScore);
	
	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->OnTextCommitted.AddDynamic(this, &USIHUDWidget::HandleChatCommitted);
	}
}

void USIHUDWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPhaseChanged.RemoveDynamic(this, &USIHUDWidget::HandlePhaseChanged);
		CachedGameState->OnTimeUpdated.RemoveDynamic(this, &USIHUDWidget::HandleTimeUpdated);
		CachedGameState->OnChatMessage.RemoveDynamic(this, &USIHUDWidget::HandleChatMessage);
	}
	
	if (CachedPlayerState.IsValid())
	{
		CachedPlayerState->OnScoreUpdated.RemoveDynamic(this, &USIHUDWidget::HandleScoreUpdated);
	}
	
	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->OnTextCommitted.RemoveDynamic(this, &USIHUDWidget::HandleChatCommitted);
	}
	
	Super::NativeDestruct();
}

void USIHUDWidget::SetSecretWord(const FString& NewSecretWord)
{
	CurrentSecretWord = NewSecretWord;
	
	Text_Timer->SetText(FText::Format(INVTEXT("제시어 - {0}"), FText::FromString(NewSecretWord)));
}

void USIHUDWidget::HandlePhaseChanged(ESIGamePhase NewPhase)
{
	CurrentPhase = NewPhase;
}

void USIHUDWidget::HandleTimeUpdated(int32 NewTime)
{
	RemainingTime = NewTime;
	
	if (!IsValid(Text_Timer))
	{
		return;
	}
	
	Text_Timer->SetText(FText::Format(INVTEXT("남은 시간 - {0}"),NewTime));
}

void USIHUDWidget::HandleChatMessage(const FChatMessagePayload& Payload)
{
	if (!IsValid(ScrollBox_ChatLog) || !ChatLineWidgetClass)
	{
		return;
	}
	
	FString SenderName = IsValid(Payload.Sender) ? Payload.Sender->GetPlayerName() : TEXT("???");
    
	USIChatLineWidget* Line = CreateWidget<USIChatLineWidget>(this, ChatLineWidgetClass);
	Line->SetMessage(SenderName, Payload.Message);
    
	ScrollBox_ChatLog->AddChild(Line);
	ScrollBox_ChatLog->ScrollToEnd();
}

void USIHUDWidget::HandleScoreUpdated(int32 NewScore)
{
	if (!IsValid(Text_Score))
	{
		return;
	}
	
	Text_Score->SetText(FText::Format(INVTEXT("스코어 - {0}"), NewScore));
}

void USIHUDWidget::HandleChatCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter) return;
    
	FString Message = Text.ToString().TrimStartAndEnd();
	if (Message.IsEmpty()) return;
    
	if (ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>())
	{
		PC->Server_SendChat(Message);
	}
    
	EditableText_ChatInput->SetText(FText::GetEmpty());
}
