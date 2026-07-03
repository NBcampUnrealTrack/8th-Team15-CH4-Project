#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Blueprint/UserWidget.h"
#include "SIGameMode.generated.h"

/**
 * 게임의 서버 측 핵심 규칙과 흐름(턴제, 점수)을 제어하는 마스터 클래스입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASIGameMode();

	// 플레이어가 방에 접속할 때마다 호출되어 출석부에 기록합니다.
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 게임(턴제 매치)을 본격적으로 시작합니다.
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void StartTurnMatch();

	// 플레이어가 정답을 제출했을 때 검증하고 점수를 부여합니다.
	void OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer);

protected:
	// 메인 메뉴 또는 인게임 UI를 띄우기 위해 블루프린트에서 설정할 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu UI")
	TSubclassOf<UUserWidget> MainMenuWidget;

private:
	// 차례 방식을 진행하기 위한 접속자 명단 (출석부)
	UPROPERTY()
	TArray<APlayerController*> PlayerOrderList;

	int32 CurrentPresenterIndex;   // 현재 출제자의 인덱스
	FString CurrentCorrectAnswer;  // 이번 턴의 정답
	int32 CorrectCountThisTurn;    // 이번 턴에 정답을 맞춘 사람 수 (점수 차등 지급용)

	// 다음 출제자에게 턴을 넘깁니다.
	void NextTurn();

	// 모든 플레이어의 턴이 끝나면 게임을 종료하고 로비로 돌아갑니다.
	void EndTurnMatch();
};