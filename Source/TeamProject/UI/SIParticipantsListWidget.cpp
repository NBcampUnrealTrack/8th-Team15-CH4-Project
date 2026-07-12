// SIParticipantsListWidget.cpp

#include "UI/SIParticipantsListWidget.h"
#include "GameState/SIGameState.h"
#include "PlayerState/SIPlayerState.h"

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

	for (APlayerState* PS : GS->PlayerArray)
	{
		BindPlayerScoreDelegate(PS);
	}

	RefreshParticipantsFromGameState();
}

void USIParticipantsListWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPlayerJoined.RemoveDynamic(this, &USIParticipantsListWidget::HandlePlayerJoined);
		CachedGameState->OnPlayerLeft.RemoveDynamic(this, &USIParticipantsListWidget::HandlePlayerLeft);

		for (APlayerState* PS : CachedGameState->PlayerArray)
		{
			UnbindPlayerScoreDelegate(PS);
		}
	}

	Super::NativeDestruct();
}

void USIParticipantsListWidget::HandlePlayerJoined(APlayerState* JoinedPlayer)
{
	if (JoinedPlayer && !Participants.Contains(JoinedPlayer))
	{
		Participants.Add(JoinedPlayer);
		BindPlayerScoreDelegate(JoinedPlayer);
		OnParticipantsRefreshed();
	}
}

void USIParticipantsListWidget::HandlePlayerLeft(APlayerState* LeftPlayer)
{
	if (LeftPlayer)
	{
		UnbindPlayerScoreDelegate(LeftPlayer);
		Participants.Remove(LeftPlayer);
		OnParticipantsRefreshed();
	}
}

void USIParticipantsListWidget::HandleAnyScoreChanged(int32 NewScore)
{
	OnParticipantScoreChanged();
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

	OnParticipantsRefreshed();
}

void USIParticipantsListWidget::BindPlayerScoreDelegate(APlayerState* PS)
{
	if (ASIPlayerState* SIPS = Cast<ASIPlayerState>(PS))
	{
		SIPS->OnScoreUpdated.AddDynamic(this, &USIParticipantsListWidget::HandleAnyScoreChanged);
	}
}

void USIParticipantsListWidget::UnbindPlayerScoreDelegate(APlayerState* PS)
{
	if (ASIPlayerState* SIPS = Cast<ASIPlayerState>(PS))
	{
		SIPS->OnScoreUpdated.RemoveDynamic(this, &USIParticipantsListWidget::HandleAnyScoreChanged);
	}
}
