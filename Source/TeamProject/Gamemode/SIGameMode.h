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

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void StartGameMatch();

	// 채팅 입력의 일반 메시지/정답 여부를 서버에서 판정합니다.
	void OnChatReceived(APlayerController* Sender, const FString& Message);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float BuildTimeLimit = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float GuessTimeLimit = 30.0f;

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
	void OnUITimerTick();
	void BroadcastChat(APlayerController* Sender, const FString& Message);

	void EndBuildPhase();
	void StartNextGuessTurn();
	void EndGuessTurn();
	void EndMatch();
};
