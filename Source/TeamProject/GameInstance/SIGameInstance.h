// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Enums/SITypes.h"
#include "SIGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_API USIGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	// 서버의 GameInstance에만 보관되며 플레이어가 방을 만들어 호스트가 되었을때 설정한 방설정을 적용하는데 사용합니다.
	void CreateRoom(const FSIRoomSettings& NewRoomSettings);   // 지금은 내부에서 OpenLevel(...,"listen") 호출
	void JoinRoom(const FString& Address); // 지금은 내부에서 OpenLevel(Address) 호출

	// 서버의 GameInstance에만 보관되며 레벨 이동 후 Main GameMode가 자동 시작에 사용합니다.
	void PreparePendingMatch(int32 InExpectedPlayerCount);
	bool IsPendingMatchReady(int32 ConnectedPlayerCount) const;
	void ConsumePendingMatch();

	void AddChatLog(const FString& SenderName, const FString& Message);
	const TArray<FChatLogEntry>& GetChatHistory() const { return ChatHistory; }
	void ClearChatHistory();

private:
	UPROPERTY()
	FSIRoomSettings RoomSettings;
	
	UPROPERTY()
	bool bPendingMatchStart = false;
	
	UPROPERTY()
	int32 ExpectedPlayerCount = 0;

	UPROPERTY()
	TArray<FChatLogEntry> ChatHistory;

	static constexpr int32 MaxChatHistoryCount = 100;
};
