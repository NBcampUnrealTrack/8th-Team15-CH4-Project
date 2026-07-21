#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Blueprint/UserWidget.h"
#include "SIGameMode.generated.h"

class UDataTable;
class AActor;

USTRUCT()
struct FSIAssignedKeyword
{
	GENERATED_BODY()

	UPROPERTY()
	FString Keyword;

	UPROPERTY()
	int32 Level = 0;
};

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

	/** 이미 시작된 매치로의 중도 참여를 거절한다.
		단, 논심리스 ServerTravel은 로비에서 함께 넘어온 기존 플레이어도 PreLogin을 다시 태우므로
		"게임 중이면 무조건 거절"로 짜면 전원이 튕겨나간다.
		→ 로비 출발 시 봉인해둔 참가자 명단(USIGameInstance)에 있는지로 구분한다. */
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** PlayerStart가 부족해 폰이 아예 스폰되지 않는 것을 막는다.
		엔진 기본 동작은 PlayerStart가 점유돼 있고 주변에 비켜설 자리도 없으면 nullptr을 반환하고,
		그러면 RestartPlayerAtPlayerStart가 폰을 만들지 않고 끝낸다(= 조작 불가능한 관전 카메라).
		이 레벨은 턴 시작 시 제작자와 추측자를 각 작업공간 주변으로 재배치하므로
		최초 스폰 위치가 겹쳐도 무해하다 → 겹치더라도 스폰시키는 쪽이 옳다. */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void StartGameMatch();

	// BuildPhase 배치 시 BP가 선택한 플레이어별 작업공간을 서버에 기록합니다.
	UFUNCTION(BlueprintCallable, Category = "GameFlow|Spawn")
	void RegisterPlayerWorkspace(APlayerController* Player, AActor* WorkspaceArea);

	// 채팅 입력을 검증하고 모든 클라이언트에 전달합니다.
	void OnChatReceived(APlayerController* Sender, const FString& Message);

	// 정답 입력 UI에서 제출된 답을 서버에서 검증하고 점수를 지급합니다.
	void OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer, int32 SubmittedRound);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float BuildTimeLimit = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings|Placement", meta = (ClampMin = "10", ClampMax = "40"))
	int32 MaxPlacedShapeCount = 25;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Turn", meta = (ClampMin = "0.0"))
	float CorrectAnswerTransitionDelay = 2.0f;

	// 정답 순위별 점수입니다. 정답자가 배열보다 많으면 마지막 점수를 계속 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Score")
	TArray<int32> CorrectAnswerScores = { 2, 1 };

	// The creator earns the configured level score for the first correct answer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Score")
	TMap<int32, int32> CreatorFirstCorrectScoresByLevel;

	// From the second correct answer onward, the creator earns this score regardless of level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Score", meta = (ClampMin = "0"))
	int32 CreatorAdditionalCorrectScore = 1;

	// MainLevel 중앙 빈 공간에 배치할 기준 액터의 Actor Tag입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Guess Spawn")
	FName GuessViewingAreaTag = TEXT("GuessViewingArea");

	// 중앙 기준점에서 현재 작업공간 쪽으로 관람 대형을 이동시키는 거리입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Guess Spawn", meta = (ClampMin = "0.0"))
	float GuessFormationOffsetTowardTarget = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Guess Spawn", meta = (ClampMin = "50.0"))
	float GuessPlayerSpacing = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Guess Spawn", meta = (ClampMin = "50.0"))
	float GuessRowSpacing = 120.0f;

	// WorkspaceArea 중심보다 이만큼 높은 지점을 바라봅니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings|Guess Spawn")
	float GuessLookAtHeightOffset = 100.0f;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu UI")
	TSubclassOf<UUserWidget> MainMenuWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Game Settings|Keyword")
	TObjectPtr<UDataTable> KeywordDataTable;

	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow|Spawn")
	void AssignPlayerWorkspaces();

	bool MoveBuilderToWorkspace(APlayerController* Builder);
	void SpawnGuessersToTargetWorkspace(APlayerController* TargetOwner);

private:
	UPROPERTY()
	TArray<APlayerController*> PlayerOrderList;

	// 두 컨테이너는 GameMode에만 있으므로 정답 원본은 서버에만 존재합니다.
	UPROPERTY()
	TMap<APlayerController*, FSIAssignedKeyword> PlayerAssignedWords;

	UPROPERTY()
	TSet<APlayerController*> CorrectPlayersThisTurn;

	// 서버 전용: BuildPhase에서 실제로 배정된 플레이어별 WorkspaceArea입니다.
	UPROPERTY()
	TMap<APlayerController*, TObjectPtr<AActor>> PlayerWorkspaceAreas;

	FTimerHandle GameTimerHandle;
	FTimerHandle UITimerTickHandle;
	FTimerHandle TurnTransitionTimerHandle;

	// 결과 화면이 끝나고 로비로 복귀하기까지의 타이머들입니다.
	FTimerHandle ReturnToLobbyTimerHandle;
	FTimerHandle ResultLoadingScreenTimerHandle;

	int32 CurrentWorkspaceIndex = -1;
	bool bTurnTransitionPending = false;

	// 로비 복귀 트래블이 시작됐는지. 트래블로 인한 Logout을 진짜 퇴장과 구분하는 데 쓴다
	// (ASILobbyGameMode의 같은 이름 플래그와 같은 역할).
	bool bTravelRequested = false;

	void ApplyHostMatchSettings();
	int32 GetCreatorScoreForCorrectAnswer(int32 KeywordLevel, bool bIsFirstCorrect) const;

	bool AssignWordsToPlayers();
	void TryStartPendingMatch();
	void OnUITimerTick();
	void BroadcastChat(APlayerController* Sender, const FString& Message);

	void StartNextTurn();
	void EndCurrentTurn();
	void ScheduleTurnEndAfterAllCorrect();
	void CompleteDelayedTurnEnd();
	void AdvanceFromDisconnectedBuilder();
	int32 GetEligibleGuesserCount(APlayerController* Builder) const;
	bool HaveAllEligibleGuessersAnswered(APlayerController* Builder) const;
	void SendTurnRoles(APlayerController* Builder);
	void EndMatch();
	void ReturnToLobby();
};
