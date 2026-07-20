// SILobbySettingWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "Enums/SITypes.h"	// FChatMessagePayload
#include "SILobbySettingWidget.generated.h"

class ASIGameState;
class ASIPlayerState;

class UButton;
class UEditableText;

UCLASS()
class TEAMPROJECT_API USILobbySettingWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsLocalPlayerHost = false;
	
private:
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UButton> Button_GameStart;

	/** 방 나가기 — 호스트가 누르면 방이 해체된다 */
	UPROPERTY(Meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LeaveRoom;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_RoomName;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_RoomPassword;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_BuildTimeLimit;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_GuessTimeLimit;

	// ── 채팅 (인게임 HUD와 같은 구성) ──
	// BindWidgetOptional인 이유: 이 셋이 없어도 로비의 나머지 기능은 그대로 동작해야 한다.
	UPROPERTY(Meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_ChatLog;

	UPROPERTY(Meta = (BindWidgetOptional))
	TObjectPtr<UEditableText> EditableText_ChatInput;

	// WBP_LobbySetting 디테일에서 WBP_ChatLine을 지정해야 채팅이 보인다 (HUD와 동일)
	UPROPERTY(EditDefaultsOnly, Category = "Lobby|Chat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIChatLineWidget> ChatLineWidgetClass;
	
	TWeakObjectPtr<ASIPlayerState> CachedPlayerState;
	TWeakObjectPtr<ASIGameState> CachedGameState;

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestStartGame();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestLeaveRoom();

private:
	/** 입력칸에서 엔터/포커스 이탈로 편집이 끝났을 때 서버에 반영을 요청한다.
		OnTextChanged(키 입력마다)가 아니라 커밋 시점인 이유는 RPC 폭주를 막기 위해서다. */
	UFUNCTION()
	void HandleRoomSettingCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** 비밀번호 칸은 숫자만 받는다 (실제 필터는 USIUserWidget::FilterEditableTextToDigits) */
	UFUNCTION()
	void HandleRoomPasswordChanged(const FText& Text);

	/** ── 채팅 ── 기록은 ASIPlayerController가 남기고 여기선 그리기만 한다 */
	UFUNCTION()
	void HandleChatMessage(const FChatMessagePayload& Payload);

	UFUNCTION()
	void HandleChatCommitted(const FText& Chat, ETextCommit::Type CommitMethod);

public:
	virtual void FocusChatInput() override;

private:

	void SubmitRoomSettings();
	
private:	
	UFUNCTION()
	void HandleHostStatusChanged(bool bNewIsHost);

	/** GameState에 복제된 방 설정을 위젯에 반영 */
	UFUNCTION()
	void RefreshRoomInfo();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override; 
	
private:
	void ResolveHostStatus();

	/** GameState 복제가 늦을 수 있어 유효해질 때까지 재시도 후 구독 */
	void ResolveRoomInfo();

	FTimerHandle HostResolveRetryTimer;
	FTimerHandle RoomInfoResolveRetryTimer;

};
