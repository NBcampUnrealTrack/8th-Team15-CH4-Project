// SIParticipantsListWidget.cpp

#include "UI/SIParticipantsListWidget.h"
#include "GameState/SIGameState.h"
#include "PlayerState/SIPlayerState.h"

#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"

void USIParticipantsListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ASIGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASIGameState>() : nullptr;
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ParticipantsListWidget] GameState 미확보 - 델리게이트 바인딩 실패"));
		return;
	}

	CachedGameState = GS;

	GS->OnPlayerJoined.AddDynamic(this, &USIParticipantsListWidget::HandlePlayerJoined);
	GS->OnPlayerLeft.AddDynamic(this, &USIParticipantsListWidget::HandlePlayerLeft);
	
	RefreshParticipantsFromGameState();
}

void USIParticipantsListWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPlayerJoined.RemoveDynamic(this, &USIParticipantsListWidget::HandlePlayerJoined);
		CachedGameState->OnPlayerLeft.RemoveDynamic(this, &USIParticipantsListWidget::HandlePlayerLeft);
	}

	Super::NativeDestruct();
}

void USIParticipantsListWidget::HandlePlayerJoined(APlayerState* JoinedPlayer)
{
	RefreshParticipantsFromGameState();
}

void USIParticipantsListWidget::HandlePlayerLeft(APlayerState* LeftPlayer)
{
	RefreshParticipantsFromGameState();
}

void USIParticipantsListWidget::DrawParticipants()
{
	if (!IsValid(VerticalBox_ParticipantsList))
	{
		return;
	}
	
	const int slotcount = VerticalBox_ParticipantsList->GetChildrenCount();
	
	for (int i = 0; i < slotcount; ++i)
	{
		UTextBlock* RawText = Cast<UTextBlock>(VerticalBox_ParticipantsList->GetChildAt(i));
		if (!RawText)
		{
			continue;
		}
		
		if (i < Participants.Num())
		{
			const FString PlayerName = Participants[i]->GetPlayerName();
			
			FString Line = FString::Printf(TEXT("%s"), *PlayerName);
			
			ASIPlayerState* PS  = Cast<ASIPlayerState>(Participants[i]);
			
			if (PS && PS->bIsHost)
			{
				Line += TEXT(" (방장)");
			}
			
			RawText->SetText(FText::FromString(Line));
			RawText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			RawText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USIParticipantsListWidget::RefreshParticipantsFromGameState()
{
	Participants.Reset();
	if (!CachedGameState.IsValid())
	{
		return;
	}

	for (APlayerState* PS : CachedGameState->PlayerArray)
	{
		if (PS)
		{
			Participants.Add(PS);
		}
	}
	DrawParticipants();
}
