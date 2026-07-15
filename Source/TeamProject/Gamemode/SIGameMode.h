#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Blueprint/UserWidget.h"
#include "SIGameMode.generated.h"

class UDataTable;

/**
 * 모든 플레이어가 동시에 제작한 뒤 각 작업공간을 순회하며 정답을 맞히는 게임 모드입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASIGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void StartGameMatch();

	// 채팅 입력을 검증하고 모든 클라이언트에 전달합니다.
	void OnChatReceived(APlayerController* Sender, const FString& Message);

	// 정답 입력 UI에서 제출된 답을 서버에서 검증하고 점수를 지급합니다.
	void OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float BuildTimeLimit = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float GuessTimeLimit = 30.0f;

	// 정답 순위별 점수입니다. 정답자가 배열보다 많으면 마지막 점수를 계속 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Score")
	TArray<int32> CorrectAnswerScores = { 2, 1 };

protected:
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu UI")
	TSubclassOf<UUserWidget> MainMenuWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Game Settings|Keyword")
	TObjectPtr<UDataTable> KeywordDataTable;

	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow|Spawn")
	void SpawnPlayersToIndividualWorkspaces();

	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow|Spawn")
	void SpawnPlayersToTargetWorkspace(APlayerController* TargetOwner);

private:
	UPROPERTY()
	TArray<APlayerController*> PlayerOrderList;

	// 두 컨테이너는 GameMode에만 있으므로 정답 원본은 서버에만 존재합니다.
	UPROPERTY()
	TMap<APlayerController*, FString> PlayerAssignedWords;

	UPROPERTY()
	TSet<APlayerController*> CorrectPlayersThisTurn;

	FTimerHandle GameTimerHandle;
	FTimerHandle UITimerTickHandle;

	int32 CurrentWorkspaceIndex = -1;

	bool AssignWordsToPlayers();
	void TryStartPendingMatch();
	void OnUITimerTick();
	void BroadcastChat(APlayerController* Sender, const FString& Message);

	void EndBuildPhase();
	void StartNextGuessTurn();
	void EndGuessTurn();
	void EndMatch();
};
