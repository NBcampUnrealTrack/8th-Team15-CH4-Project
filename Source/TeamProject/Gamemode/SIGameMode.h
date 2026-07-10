#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Blueprint/UserWidget.h"
#include "SIGameMode.generated.h"

/**
 * 동시 제작 후 순차적으로 순회하며 정답을 맞추는 게임 모드입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASIGameMode();

	// 플레이어가 방에 접속할 때마다 호출되어 출석부에 기록합니다.
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 게임을 본격적으로 시작합니다. (제작 페이즈 시작)
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void StartGameMatch();

	// 플레이어가 정답을 제출했을 때 검증하고 점수를 부여합니다.
	void OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer);

	// 호스트가 설정할 수 있는 게임 진행 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float BuildTimeLimit = 120.0f; // 개인 작업(제작) 시간 (기본값 2분)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	float GuessTimeLimit = 30.0f; // 정답 맞추기 시간 (기본값 30초)

protected:
	// 메인 메뉴 또는 인게임 UI를 띄우기 위해 블루프린트에서 설정할 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu UI")
	TSubclassOf<UUserWidget> MainMenuWidget;

	// 모든 플레이어를 각자의 8개 작업 공간으로 분산시켜 스폰합니다. (블루프린트에서 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow|Spawn")
	void SpawnPlayersToIndividualWorkspaces();

	// 모든 플레이어를 특정 플레이어의 작업 공간으로 모아서 스폰합니다. (블루프린트에서 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow|Spawn")
	void SpawnPlayersToTargetWorkspace(APlayerController* TargetOwner);

private:
	// 접속자 명단 (최대 8명)
	UPROPERTY()
	TArray<APlayerController*> PlayerOrderList;

	// 자동 페이즈 전환을 위한 타이머
	FTimerHandle GameTimerHandle;

	int32 CurrentWorkspaceIndex;   // 현재 구경 중인 작업공간 주인의 인덱스
	FString CurrentCorrectAnswer;  // 이번 방의 정답
	int32 CorrectCountThisTurn;    // 이번 방에서 정답을 맞춘 사람 수

	// 페이즈 제어 내부 함수
	void EndBuildPhase();          // 제작 시간 종료
	void StartNextGuessTurn();     // 다음 사람 방으로 이동하여 맞추기 시작
	void EndGuessTurn();           // 맞추기 시간 종료
	void EndMatch();               // 모든 투어 종료 및 로비 귀환
};