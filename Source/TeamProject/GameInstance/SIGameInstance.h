// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Enums/SITypes.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SIGameInstance.generated.h"

class USILodingWidget;

/**
 *
 */
UCLASS()
class TEAMPROJECT_API USIGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// 맵 이동 구간을 덮는 로딩 화면. 레벨이 언로드되면 위젯도 같이 사라지므로
	// 트래블을 사이에 두고 "다시 띄워주는" 책임까지 여기서 진다.
	// 내리는 건 새 맵이 준비되면 자동 — 호출한 쪽이 짝을 맞출 필요가 없다.
	void ShowLoadingScreen();
	void HideLoadingScreen();

	// 방을 나간다. 세션 정리가 끝나면 OnSessionLeftEvent를 타고 메인메뉴로 돌아간다.
	// 호스트가 부르면 방이 해체되고, 남은 인원은 연결 끊김으로 같은 경로를 탄다.
	void LeaveRoom();

	// 세션 생성을 요청한다. 방 설정의 보관 주체는 USISessionSubsystem(GetHostSessionParams()로 조회).
	// 세션이 실제로 만들어진 뒤에야 로비 맵을 listen으로 연다.
	void CreateRoom(const FSICreateSessionParams& NewRoomSettings);

	// 입장은 USISessionSubsystem::JoinSessionByIndex가 담당한다.
	// (OSS가 해석한 주소로 ClientTravel + 비밀번호를 URL 옵션으로 동봉)
	// 채팅 기록 초기화처럼 입장 시점에 필요한 로컬 정리는 여기서 한다.
	void PrepareForJoin();

	// 서버의 GameInstance에만 보관되며 레벨 이동 후 Main GameMode가 자동 시작에 사용합니다.
	void PreparePendingMatch(int32 InExpectedPlayerCount);
	bool IsPendingMatchReady(int32 ConnectedPlayerCount) const;
	void ConsumePendingMatch();

	void AddChatLog(const FString& SenderName, const FString& Message);
	const TArray<FChatLogEntry>& GetChatHistory() const { return ChatHistory; }
	void ClearChatHistory();

private:
	// 세션 생성 완료를 받아 로비 맵을 listen으로 여는 지점
	UFUNCTION()
	void HandleCreateSessionComplete(bool bWasSuccessful);

	// 유저가 나갔든 연결이 끊겼든, 방을 벗어난 뒤 메인메뉴로 복귀시키는 단일 출구
	UFUNCTION()
	void HandleSessionLeft(ESISessionLeaveReason Reason);

	/** 새 맵 로드가 끝난 시점 — 로딩 화면을 새 월드에 다시 붙인다 */
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** 로컬 플레이어가 새 레벨을 실제로 볼 수 있는 상태가 됐는지 폴링 */
	void PollLevelReady();

	void CreateLoadingWidget();

	/** 뷰포트에서 떼고 참조를 버린다. 포인터만 버리면 화면에 남은 위젯을 다신 못 떼니 항상 이걸 거친다. */
	void RemoveLoadingWidget();

	void ClearLoadingScreenTimer();

private:
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USILodingWidget> LoadingWidgetClass;

	UPROPERTY()
	TObjectPtr<USILodingWidget> LoadingWidget;

	/** 트래블을 건너 로딩 화면을 유지해야 하는지 — 위젯 포인터는 맵마다 갈리므로 의도는 따로 들고 있는다 */
	bool bLoadingScreenActive = false;

	float LoadingScreenElapsedTime = 0.0f;

	FTimerHandle LoadingScreenPollTimer;

	FDelegateHandle PostLoadMapHandle;

	/** 게임 UI 위에 확실히 덮이도록 */
	static constexpr int32 LoadingScreenZOrder = 1000;

	static constexpr float LoadingScreenPollInterval = 0.1f;

	/** 준비 신호가 끝내 안 와도 화면이 영영 가려지지 않게 하는 안전장치 */
	static constexpr float MaxLoadingScreenSeconds = 15.0f;

private:
	UPROPERTY()
	bool bPendingMatchStart = false;
	
	UPROPERTY()
	int32 ExpectedPlayerCount = 0;

	UPROPERTY()
	TArray<FChatLogEntry> ChatHistory;

	static constexpr int32 MaxChatHistoryCount = 100;
};
