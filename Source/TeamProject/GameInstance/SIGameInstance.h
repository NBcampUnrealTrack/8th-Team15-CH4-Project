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
	// 세션 생성을 요청한다. 방 설정의 보관 주체는 USISessionSubsystem(GetHostSessionParams()로 조회).
	// 세션이 실제로 만들어진 뒤에야 로비 맵을 listen으로 연다.
	void CreateRoom(const FSICreateSessionParams& NewRoomSettings);
	void JoinRoom(const FString& Address); // 지금은 내부에서 OpenLevel(Address) 호출

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

private:
	UPROPERTY()
	bool bPendingMatchStart = false;
	
	UPROPERTY()
	int32 ExpectedPlayerCount = 0;

	UPROPERTY()
	TArray<FChatLogEntry> ChatHistory;

	static constexpr int32 MaxChatHistoryCount = 100;
};
