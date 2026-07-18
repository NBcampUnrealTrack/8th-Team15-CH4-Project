// SIRoomEntryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SIRoomEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

/** 항목이 눌렸음을 목록 위젯에 알린다. 실제 입장 판단은 목록 위젯이 한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomEntrySelected, int32, SearchResultIndex);

/** 방 목록의 한 줄. FSISessionInfo 하나를 그대로 표시만 한다. */
UCLASS()
class TEAMPROJECT_API USIRoomEntryWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	FOnRoomEntrySelected OnRoomEntrySelected;

	/** 검색 결과 한 건을 위젯에 반영 */
	void Setup(const FSISessionInfo& InSessionInfo);

	const FSISessionInfo& GetSessionInfo() const { return SessionInfo; }

	int32 GetSearchResultIndex() const { return SessionInfo.SearchResultIndex; }

	bool HasPassword() const { return SessionInfo.bHasPassword; }

	/** 선택 상태 변경 — 시각적 표현은 BP에 맡긴다 */
	void SetSelected(bool bInSelected);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 선택 상태가 바뀔 때 BP에서 테두리/색상 등을 처리 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Room")
	void OnSelectionChanged(bool bInSelected);

private:
	UFUNCTION()
	void HandleEntryClicked();

private:
	/** 줄 전체를 덮는 투명 버튼 */
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UButton> Button_Select;

	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RoomTitle;

	UPROPERTY(Meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerCount;

	/** "대기중" / "게임중" */
	UPROPERTY(Meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoomState;

	/** 비밀번호 방 자물쇠 표시 — Image든 TextBlock이든 상관없어 UWidget으로 받는다 */
	UPROPERTY(Meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Icon_Lock;

	FSISessionInfo SessionInfo;

	bool bSelected = false;
};
