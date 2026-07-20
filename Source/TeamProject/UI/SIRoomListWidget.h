// SIRoomListWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SIRoomListWidget.generated.h"

class UButton;
class UEditableText;
class UScrollBox;
class UTextBlock;
class USIRoomEntryWidget;
class USISessionSubsystem;
class USoundBase;

UCLASS()
class TEAMPROJECT_API USIRoomListWidget : public USIUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** F5 새로고침. Preview를 쓰는 이유는 비밀번호 칸이나 버튼이 포커스를 가져간 뒤에도
		키가 이 위젯까지 도달해야 하기 때문(Preview는 루트→포커스 경로를 먼저 훑는다). */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Refresh;

	/** 방 목록이 쌓이는 컨테이너 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_RoomList;

	/** 잠긴 방 입장용 비밀번호. 항상 표시되며, 잠기지 않은 방이면 입력값은 무시된다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional,AllowPrivateAccess = "true"))
	TObjectPtr<UEditableText> EditableText_JoinPassword;

	/** 방 제목 검색 — 치는 대로 목록이 걸러진다 */
	UPROPERTY(BlueprintReadOnly,meta = (BindWidgetOptional,AllowPrivateAccess = "true"))
	TObjectPtr<UEditableText> EditableText_RoomNameFilter;

	/** "검색 중…", "방이 없습니다" 같은 안내 문구 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	/** 목록 한 줄로 스폰할 위젯 클래스 (WBP_RoomEntry) */
	UPROPERTY(EditDefaultsOnly, Category = "Room", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIRoomEntryWidget> RoomEntryWidgetClass;

	/** 방 목록 항목 사이 세로 간격(px). 에디터에서 눈으로 보며 조절 */
	UPROPERTY(EditDefaultsOnly, Category = "Room", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float RoomEntrySpacing = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> RefreshSound;

private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	/** 항목 클릭 — 선택만 바꾸고 입장은 하지 않는다 */
	UFUNCTION()
	void HandleRoomSelected(int32 SearchResultIndex);

	UFUNCTION()
	void HandleFindSessionsComplete(const TArray<FSISessionInfo>& Sessions, bool bWasSuccessful);

	/** 검색어가 바뀔 때마다 재검색 없이 캐시된 결과만 다시 거른다 */
	UFUNCTION()
	void HandleFilterTextChanged(const FText& NewText);

	UFUNCTION()
	void HandleJoinSessionComplete(bool bWasSuccessful);

private:
	/** 키보드 포커스를 이 위젯으로 가져온다. Slate 트리에 붙은 뒤(= Construct 다음 틱)에 불러야 먹는다. */
	void FocusForKeyboardShortcuts();

	void RefreshRoomList(bool bPlaySound = true);

	/** 캐시된 검색 결과를 현재 검색어로 걸러 목록을 다시 그린다 */
	void RebuildRoomEntries();

	/** 선택 해제 + 목록 비우기 */
	void ClearRoomList();

	/** 선택 상태에 맞춰 입장 버튼과 비밀번호 칸을 갱신 */
	void UpdateJoinControls();

	void SetStatusText(const FText& InText);

	USISessionSubsystem* GetSessionSubsystem() const;

	/** 현재 선택된 항목 — 없으면 nullptr */
	USIRoomEntryWidget* FindSelectedEntry() const;

private:
	UPROPERTY()
	TArray<TObjectPtr<USIRoomEntryWidget>> RoomEntries;

	/** 마지막 검색 결과 원본 — 검색어 필터링은 이걸 대상으로 한다(재검색 불필요) */
	TArray<FSISessionInfo> CachedSessions;

	int32 SelectedSearchResultIndex = INDEX_NONE;

	/** 검색/입장 요청 중에는 버튼을 잠근다 */
	bool bRequestInProgress = false;
};
