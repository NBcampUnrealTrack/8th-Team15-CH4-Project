// SILobbySettingWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
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
