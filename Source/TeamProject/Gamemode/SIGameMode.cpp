// #pragma once
//
// #include "SIGameMode.h"
// #include "GameState/SIGameState.h"
// #include "PlayerController/SIPlayerController.h"
// #include "GameFramework/PlayerState.h"
// #include "TimerManager.h"
// #include "Blueprint/UserWidget.h" 
//
// ASIGameMode::ASIGameMode()
// {
// 	GameStateClass = ASIGameState::StaticClass();
// 	PlayerControllerClass = ASIPlayerController::StaticClass();
//
// 	CurrentPresenterIndex = -1;
// }
//
// void ASIGameMode::PostLogin(APlayerController* NewPlayer)
// {
// 	Super::PostLogin(NewPlayer);
//
// 	if (NewPlayer)
// 	{
// 		PlayerOrderList.Add(NewPlayer);
// 	}
// }
//
// // 이름 변경: StartMatch -> StartTurnMatch
// void ASIGameMode::StartTurnMatch()
// {
// 	ASIGameState* SIGameState = GetGameState<ASIGameState>();
// 	if (SIGameState)
// 	{
// 		// 옛날 코드(CurrentMatchState) 대신 새 코드(CurrentGamePhase)를 사용합니다.
// 		SIGameState->CurrentGamePhase = ESIGamePhase::BuildPhase;
// 	}
//
// 	CurrentPresenterIndex = 0;
// 	NextTurn();
// }
//
// void ASIGameMode::NextTurn()
// {
// 	if (!PlayerOrderList.IsValidIndex(CurrentPresenterIndex))
// 	{
// 		EndTurnMatch(); // 이름 변경
// 		return;
// 	}
//
// 	ASIGameState* SIGameState = GetGameState<ASIGameState>();
// 	if (SIGameState)
// 	{
// 		SIGameState->CurrentPresenter = PlayerOrderList[CurrentPresenterIndex]->PlayerState;
// 	}
//
// 	CurrentCorrectAnswer = TEXT("사과");
// 	CorrectCountThisTurn = 0;
//
// 	// 출제자의 컨트롤러를 찾아 제시어 전송
// 	ASIPlayerController* PresenterPC = Cast<ASIPlayerController>(PlayerOrderList[CurrentPresenterIndex]);
// 	if (PresenterPC)
// 	{
// 		PresenterPC->Client_ReceiveSecretWord(CurrentCorrectAnswer);
// 	}
//
// 	CurrentPresenterIndex++;
// }
//
// void ASIGameMode::OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer)
// {
// 	if (SubmittedAnswer.Equals(CurrentCorrectAnswer, ESearchCase::IgnoreCase))
// 	{
// 		APlayerState* SubmitterState = Submitter->PlayerState;
// 		ASIGameState* SIGameState = GetGameState<ASIGameState>();
//
// 		if (SubmitterState && SIGameState)
// 		{
// 			int32 ScoreToEarn = FMath::Max(5 - CorrectCountThisTurn, 1);
// 			SubmitterState->SetScore(SubmitterState->GetScore() + ScoreToEarn);
//
// 			if (SIGameState->CurrentPresenter)
// 			{
// 				SIGameState->CurrentPresenter->SetScore(SIGameState->CurrentPresenter->GetScore() + 1);
// 			}
//
// 			CorrectCountThisTurn++;
// 		}
// 	}
// }
//
// // 이름 변경: EndMatch -> EndTurnMatch
// void ASIGameMode::EndTurnMatch()
// {
// 	ASIGameState* SIGameState = GetGameState<ASIGameState>();
// 	if (SIGameState)
// 	{
// 		// 마찬가지로 결과 발표 단계로 상태를 바꿉니다.
// 		SIGameState->CurrentGamePhase = ESIGamePhase::ResultPhase;
// 	}
//
// 	FTimerHandle ReturnTimerHandle;
// 	GetWorldTimerManager().SetTimer(ReturnTimerHandle, [this]()
// 		{
// 			ProcessServerTravel(TEXT("/Game/Maps/LobbyMap?listen"));
// 		}, 5.f, false);
// }