#include "SIGameMode.h"
#include "GameState/SIGameState.h"
#include "PlayerController/SIPlayerController.h" 
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

ASIGameMode::ASIGameMode()
{
	/*GameStateClass = ASIGameState::StaticClass();
	PlayerControllerClass = ASIPlayerController::StaticClass();*/

	CurrentPresenterIndex = -1;
}

void ASIGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		PlayerOrderList.Add(NewPlayer);
	}
}

void ASIGameMode::StartTurnMatch()
{
	ASIGameState* SIGameState = GetGameState<ASIGameState>();
	if (SIGameState)
	{
		SIGameState->CurrentGamePhase = ESIGamePhase::BuildPhase;
	}

	CurrentPresenterIndex = 0;
	NextTurn();
}

void ASIGameMode::NextTurn()
{
	if (!PlayerOrderList.IsValidIndex(CurrentPresenterIndex))
	{
		EndTurnMatch();
		return;
	}

	ASIGameState* SIGameState = GetGameState<ASIGameState>();
	if (SIGameState)
	{
		SIGameState->CurrentPresenter = PlayerOrderList[CurrentPresenterIndex]->PlayerState;
		SIGameState->CurrentGamePhase = ESIGamePhase::BuildPhase;
	}

	CurrentCorrectAnswer = TEXT("사과");
	CorrectCountThisTurn = 0;

	ASIPlayerController* PresenterPC = Cast<ASIPlayerController>(PlayerOrderList[CurrentPresenterIndex]);
	if (PresenterPC)
	{
		PresenterPC->Client_ReceiveSecretWord(CurrentCorrectAnswer);
	}

	CurrentPresenterIndex++;
}

void ASIGameMode::OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer)
{
	ASIGameState* SIGameState = GetGameState<ASIGameState>();

	if (SIGameState && SIGameState->CurrentGamePhase != ESIGamePhase::GuessPhase)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Not Guess Phase!"));
		return;
	}

	if (SubmittedAnswer.Equals(CurrentCorrectAnswer, ESearchCase::IgnoreCase))
	{
		APlayerState* SubmitterState = Submitter->PlayerState;

		if (SubmitterState)
		{
			int32 ScoreToEarn = FMath::Max(5 - CorrectCountThisTurn, 1);
			SubmitterState->SetScore(SubmitterState->GetScore() + ScoreToEarn);

			if (SIGameState->CurrentPresenter)
			{
				SIGameState->CurrentPresenter->SetScore(SIGameState->CurrentPresenter->GetScore() + 1);
			}

			CorrectCountThisTurn++;
		}
	}
}

void ASIGameMode::EndTurnMatch()
{
	ASIGameState* SIGameState = GetGameState<ASIGameState>();
	if (SIGameState)
	{
		SIGameState->CurrentGamePhase = ESIGamePhase::ResultPhase;
	}

	FTimerHandle ReturnTimerHandle;
	GetWorldTimerManager().SetTimer(ReturnTimerHandle, [this]()
		{
			ProcessServerTravel(TEXT("/Game/Maps/LobbyMap?listen"));
		}, 5.f, false);
}