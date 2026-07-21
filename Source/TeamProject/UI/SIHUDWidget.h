// SIHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "Enums/SITypes.h"
#include "SIHUDWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuessSubmitted, const FString&, Guess);

class UTextBlock;
class UScrollBox;
class UEditableText;
class UVerticalBox;

class APlayerState;
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

	// 이번 턴에 감상 중인 작품의 주인. GuessPhase가 아니면 null일 수 있다.
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<APlayerState> CurrentWorkspaceOwner;

	UPROPERTY()
	FOnGuessSubmitted OnGuessSubmitted;
	
	UFUNCTION()
	void SetSecretWord(const FString& NewSecretWord);

	void RefreshTurnRoleVisibility();

	// 채팅 입력창에 키보드 포커스를 준다 (PlayerController가 채팅 열 때 호출)
	virtual void FocusChatInput() override;
	
private:	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Timer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Score;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Keyword;
	
	// "OO님의 작품" 안내. WBP_HUD에 아직 없어도 컴파일이 깨지지 않도록 Optional로 둔다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WorkspaceOwner;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_ChatLog;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_ChatInput;
	
	// 채팅 한 줄을 그릴 위젯 클래스. WBP_HUD 디테일에서 WBP_ChatLine을 지정해야 채팅이 표시됨.
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Chat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIChatLineWidget> ChatLineWidgetClass;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_AnswerInput;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_CorrectAnswer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_InCorrectAnswer;
	
	TWeakObjectPtr<ASIPlayerState> CachedPlayerState;
	
	FTimerHandle ResultHideTimerHandle;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// GameState/PlayerState가 준비될 때까지 재시도하며 델리게이트를 배선한다.
	void BindGameData();

	// GS/PS 확보 재시도 타이머
	FTimerHandle DataBindRetryTimer;

	void ShowResult(bool bCorrect);
	void HideResult();

	// 그리기는 USIUserWidget의 공용 헬퍼(AddChatLineTo/RestoreChatHistoryTo)를 쓴다.
	// 기록은 ASIPlayerController가 담당하므로 여기선 저장하지 않는다.
	
	UFUNCTION()
	void HandlePhaseChanged(ESIGamePhase NewPhase);

	UFUNCTION()
	void HandleTimeUpdated(int32 NewTime);

	UFUNCTION()
	void HandleWorkspaceOwnerChanged(APlayerState* NewWorkspaceOwner);

	// 페이즈와 주인이 각각 따로 복제돼 도착하므로, 둘 중 뭐가 먼저 와도 같은 결과가 되도록
	// 표시 갱신은 한 곳에 모아두고 양쪽 핸들러가 이걸 부른다.
	void RefreshWorkspaceOwnerText();

	UFUNCTION()
	void HandleChatMessage(const FChatMessagePayload& Payload);
	
	UFUNCTION()
	void HandleAnswerResult(const FAnswerResultPayload& Payload);
	
	UFUNCTION()
	void HandleScoreUpdated(int32 NewScore);
	
	UFUNCTION()
	void HandleChatCommitted(const FText& Chat, ETextCommit::Type CommitMethod);
	
	UFUNCTION()
	void HandleAnswerCommitted(const FText& Answer, ETextCommit::Type CommitMethod);
	
	// 바인딩된 GameState (해제 시 참조 필요)
	TWeakObjectPtr<ASIGameState> CachedGameState;
};
