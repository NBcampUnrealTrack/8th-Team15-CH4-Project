// SIHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "Enums/SITypes.h"
#include "SIHUDWidget.generated.h"

class UTextBlock;
class UScrollBox;
class UEditableText;

class ASIGameState;
class ASIPlayerState;

class USIChatLineWidget;

UCLASS()
class TEAMPROJECT_API USIHUDWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	// 서버에서 넘겨준 이번 턴 제시어
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FString CurrentSecretWord;

	// 현재 게임 페이즈 캐시
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	ESIGamePhase CurrentPhase = ESIGamePhase::None;

	// 남은 시간(초) 캐시
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 RemainingTime = 0;
	
	UFUNCTION()
	void SetSecretWord(const FString& NewSecretWord);
	
private:	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Timer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Score;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Keyword;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_ChatLog;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_ChatInput;
	
	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIChatLineWidget> ChatLineWidgetClass;
	
	TWeakObjectPtr<ASIPlayerState> CachedPlayerState;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePhaseChanged(ESIGamePhase NewPhase);

	UFUNCTION()
	void HandleTimeUpdated(int32 NewTime);

	UFUNCTION()
	void HandleChatMessage(const FChatMessagePayload& Payload);
	
	UFUNCTION()
	void HandleScoreUpdated(int32 NewScore);
	
	UFUNCTION()
	void HandleChatCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
	// 바인딩된 GameState (해제 시 참조 필요)
	TWeakObjectPtr<ASIGameState> CachedGameState;
};
