// SIHUDWidget.cpp

#include "UI/SIHUDWidget.h"
#include "PlayerController/SIPlayerController.h"
#include "GameInstance/SIGameInstance.h"
#include "GameState/SIGameState.h"
#include "PlayerState/SIPlayerState.h"
#include "UI/SIChatLineWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableText.h"
#include "Components/VerticalBox.h"

void USIHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 정답/오답 패널은 초기엔 무조건 숨김.
	// GS/PS 확보 실패로 아래에서 early-return 되더라도 이 초기화는 반드시 실행되도록 맨 위로 올림.
	if (IsValid(VerticalBox_CorrectAnswer))
	{
		VerticalBox_CorrectAnswer->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(VerticalBox_InCorrectAnswer))
	{
		VerticalBox_InCorrectAnswer->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 위젯 존재만 필요한 입력 바인딩: GS/PS 확보 여부와 무관하게 항상 실행.
	// (이전엔 PS early-return 뒤에 있어서 PS 복제가 늦으면 채팅 Enter가 영영 안 붙었음)
	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->OnTextCommitted.AddDynamic(this, &USIHUDWidget::HandleChatCommitted);
	}

	if (IsValid(EditableText_AnswerInput))
	{
		EditableText_AnswerInput->OnTextCommitted.AddDynamic(this, &USIHUDWidget::HandleAnswerCommitted);
	}

	// 과거 대화를 먼저 그리고(복원) → 실시간 구독을 붙인다(BindGameData) 순서를 지킨다
	RestoreChatHistoryTo(ScrollBox_ChatLog, ChatLineWidgetClass);

	// Enter 포커스가 이 위젯의 채팅창으로 오도록 등록
	if (ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>())
	{
		PC->RegisterChatWidget(this);
	}

	// GameState/PlayerState 의존 배선은 준비될 때까지 재시도
	BindGameData();
}

void USIHUDWidget::BindGameData()
{
	UWorld* World = GetWorld();
	ASIGameState* GS = World ? World->GetGameState<ASIGameState>() : nullptr;
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();

	if (!IsValid(GS) || !IsValid(PS))
	{
		// 아직 복제 안 됨 → 잠시 후 재시도 (위젯이 GS/PS보다 먼저 생성되는 케이스 대비)
		if (World)
		{
			World->GetTimerManager().SetTimer(
				DataBindRetryTimer, this, &USIHUDWidget::BindGameData, 0.1f, false);
		}
		return;
	}

	World->GetTimerManager().ClearTimer(DataBindRetryTimer);

	CachedGameState = GS;
	GS->OnPhaseChanged.AddDynamic(this, &USIHUDWidget::HandlePhaseChanged);
	GS->OnTimeUpdated.AddDynamic(this, &USIHUDWidget::HandleTimeUpdated);
	GS->OnChatMessage.AddDynamic(this, &USIHUDWidget::HandleChatMessage);
	GS->OnAnswerResult.AddDynamic(this, &USIHUDWidget::HandleAnswerResult);
	GS->OnWorkspaceOwnerChanged.AddDynamic(this, &USIHUDWidget::HandleWorkspaceOwnerChanged);

	CachedPlayerState = PS;
	PS->OnScoreUpdated.AddDynamic(this, &USIHUDWidget::HandleScoreUpdated);

	// 초기 상태 즉시 반영 (이미 진행 중인 게임에 접속하는 케이스)
	HandlePhaseChanged(GS->CurrentGamePhase);
	HandleTimeUpdated(GS->RemainingTime);
	HandleWorkspaceOwnerChanged(GS->CurrentWorkspaceOwner);
	HandleScoreUpdated(PS->CurrentScore);
}

void USIHUDWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPhaseChanged.RemoveDynamic(this, &USIHUDWidget::HandlePhaseChanged);
		CachedGameState->OnTimeUpdated.RemoveDynamic(this, &USIHUDWidget::HandleTimeUpdated);
		CachedGameState->OnChatMessage.RemoveDynamic(this, &USIHUDWidget::HandleChatMessage);
		CachedGameState->OnAnswerResult.RemoveDynamic(this, &USIHUDWidget::HandleAnswerResult);
		CachedGameState->OnWorkspaceOwnerChanged.RemoveDynamic(this, &USIHUDWidget::HandleWorkspaceOwnerChanged);
	}
	
	if (CachedPlayerState.IsValid())
	{
		CachedPlayerState->OnScoreUpdated.RemoveDynamic(this, &USIHUDWidget::HandleScoreUpdated);
	}
	
	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->OnTextCommitted.RemoveDynamic(this, &USIHUDWidget::HandleChatCommitted);
	}
	
	if (IsValid(EditableText_AnswerInput))
	{
		EditableText_AnswerInput->OnTextCommitted.RemoveDynamic(this, &USIHUDWidget::HandleAnswerCommitted);
	}
	
	if (ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>())
	{
		PC->UnregisterChatWidget(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResultHideTimerHandle);
		World->GetTimerManager().ClearTimer(DataBindRetryTimer);
	}

	Super::NativeDestruct();
}

void USIHUDWidget::ShowResult(bool bCorrect)
{
	constexpr float AnswerResultDisplayDuration = 1.5f;

	if (!IsValid(VerticalBox_CorrectAnswer) || !IsValid(VerticalBox_InCorrectAnswer))
	{
		return;
	}
	
	if (bCorrect)
	{
		VerticalBox_CorrectAnswer->SetVisibility(ESlateVisibility::Visible);
		
		GetWorld()->GetTimerManager().SetTimer(
			ResultHideTimerHandle, this, &USIHUDWidget::HideResult, AnswerResultDisplayDuration, false);
	}
	else
	{
		VerticalBox_InCorrectAnswer->SetVisibility(ESlateVisibility::Visible);
		
		GetWorld()->GetTimerManager().SetTimer(
			ResultHideTimerHandle, this, &USIHUDWidget::HideResult, AnswerResultDisplayDuration, false);
	}
}

void USIHUDWidget::HideResult()
{
	if (IsValid(VerticalBox_CorrectAnswer))
	{
		VerticalBox_CorrectAnswer->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	if (IsValid(VerticalBox_InCorrectAnswer))
	{
		VerticalBox_InCorrectAnswer->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USIHUDWidget::SetSecretWord(const FString& NewSecretWord)
{
	CurrentSecretWord = NewSecretWord;

	if (IsValid(Text_Keyword))
	{
		Text_Keyword->SetText(FText::Format(INVTEXT("제시어 - {0}"), FText::FromString(NewSecretWord)));
	}

	RefreshTurnRoleVisibility();
}

void USIHUDWidget::RefreshTurnRoleVisibility()
{
	if (CurrentPhase != ESIGamePhase::TurnPhase)
	{
		return;
	}

	const bool bIsBuilder = CachedPlayerState.IsValid()
		&& CurrentWorkspaceOwner == CachedPlayerState.Get();

	if (IsValid(EditableText_AnswerInput))
	{
		EditableText_AnswerInput->SetVisibility(
			bIsBuilder ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}

	if (IsValid(Text_Keyword))
	{
		Text_Keyword->SetVisibility(
			bIsBuilder ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	RefreshWorkspaceOwnerText();
}

void USIHUDWidget::HandlePhaseChanged(ESIGamePhase NewPhase)
{
	CurrentPhase = NewPhase;
	if (NewPhase == ESIGamePhase::TurnPhase)
	{
		RefreshTurnRoleVisibility();
		return;
	}

	// 정답 입력창: BuildPhase엔 숨김, GuessPhase엔 표시
	if (IsValid(EditableText_AnswerInput))
	{
		if (NewPhase == ESIGamePhase::BuildPhase)
		{
			EditableText_AnswerInput->SetVisibility(ESlateVisibility::Hidden);
		}
		else if (NewPhase == ESIGamePhase::GuessPhase)
		{
			EditableText_AnswerInput->SetVisibility(ESlateVisibility::Visible);
		}
	}

	RefreshWorkspaceOwnerText();

	// 제시어: BuildPhase엔 표시(그리는 사람이 봐야 함), GuessPhase엔 숨김(맞추는 사람에게 정답 노출 방지)
	if (IsValid(Text_Keyword))
	{
		if (NewPhase == ESIGamePhase::BuildPhase)
		{
			Text_Keyword->SetVisibility(ESlateVisibility::Visible);
		}
		else if (NewPhase == ESIGamePhase::GuessPhase)
		{
			Text_Keyword->SetVisibility(ESlateVisibility::Hidden);
		}
	}
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

void USIHUDWidget::HandleWorkspaceOwnerChanged(APlayerState* NewWorkspaceOwner)
{
	CurrentWorkspaceOwner = NewWorkspaceOwner;
	RefreshTurnRoleVisibility();
	if (CurrentPhase != ESIGamePhase::TurnPhase)
	{
		RefreshWorkspaceOwnerText();
	}
}

void USIHUDWidget::RefreshWorkspaceOwnerText()
{
	if (!IsValid(Text_WorkspaceOwner))
	{
		return;
	}

	if ((CurrentPhase != ESIGamePhase::TurnPhase && CurrentPhase != ESIGamePhase::GuessPhase)
		|| !IsValid(CurrentWorkspaceOwner))
	{
		Text_WorkspaceOwner->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// 자기 차례인 사람은 정답을 이미 알고 있다 → 문구를 갈라 헷갈리지 않게 한다.
	const bool bIsMine = (CachedPlayerState.IsValid() && CurrentWorkspaceOwner == CachedPlayerState.Get());
	const FText Message = bIsMine
		? INVTEXT("현재 작품을 제작하고 있습니다!")
		: FText::Format(INVTEXT("{0}님의 작품을 맞춰주세요!"), FText::FromString(CurrentWorkspaceOwner->GetPlayerName()));

	Text_WorkspaceOwner->SetText(Message);
	Text_WorkspaceOwner->SetVisibility(ESlateVisibility::Visible);
}

void USIHUDWidget::FocusChatInput()
{
	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->SetKeyboardFocus();
	}
}

void USIHUDWidget::HandleChatMessage(const FChatMessagePayload& Payload)
{
	// 저장은 ASIPlayerController가 한다 — 여기서 또 저장하면 같은 줄이 두 번 쌓인다
	AddChatLineTo(ScrollBox_ChatLog, ChatLineWidgetClass,
		ResolveChatSenderName(Payload), Payload.Message);
}

void USIHUDWidget::HandleAnswerResult(const FAnswerResultPayload& Payload)
{
	if (CachedPlayerState.IsValid() && Payload.Submitter == CachedPlayerState.Get())
	{
		ShowResult(Payload.bIsCorrect);
	}
}

void USIHUDWidget::HandleScoreUpdated(int32 NewScore)
{
	if (!IsValid(Text_Score))
	{
		return;
	}
	
	Text_Score->SetText(FText::Format(INVTEXT("스코어 - {0}"), NewScore));
}

void USIHUDWidget::HandleChatCommitted(const FText& Chat, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter) return;
    
	ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>();
	FString Message = Chat.ToString().TrimStartAndEnd();
    
	if (!Message.IsEmpty() && IsValid(PC))
	{
		PC->Server_SendChat(Message);
	}
    
	EditableText_ChatInput->SetText(FText::GetEmpty());

	// 채팅 입력 종료 → 게임 입력 모드로 복귀 (빈 메시지로 Enter 쳐도 닫힘)
	if (IsValid(PC))
	{
		PC->EndChatFocus();
	}
}

void USIHUDWidget::HandleAnswerCommitted(const FText& Answer, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter) return;
    
	FString Message = Answer.ToString().TrimStartAndEnd();
	if (Message.IsEmpty()) return;
    
	if (ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>())
	{
		const int32 Round = CachedGameState.IsValid()
			? CachedGameState->CurrentRound : INDEX_NONE;
		PC->Server_SubmitAnswer(Message, Round);
	}
	
	EditableText_AnswerInput->SetText(FText::GetEmpty());
}
