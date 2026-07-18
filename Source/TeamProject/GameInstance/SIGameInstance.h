// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Enums/SITypes.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SIGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_API USIGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;

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

private:
	UPROPERTY()
	bool bPendingMatchStart = false;
	
	UPROPERTY()
	int32 ExpectedPlayerCount = 0;

	UPROPERTY()
	TArray<FChatLogEntry> ChatHistory;

	static constexpr int32 MaxChatHistoryCount = 100;
};
