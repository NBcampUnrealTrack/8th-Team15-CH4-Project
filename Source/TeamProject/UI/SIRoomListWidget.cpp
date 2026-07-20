// SIRoomListWidget.cpp


#include "UI/SIRoomListWidget.h"
#include "kismet/GameplayStatics.h"
#include "GameInstance/SIGameInstance.h"
#include "GameInstance/SISessionSubsystem.h"
#include "UI/SIRoomEntryWidget.h"

#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"

void USIRoomListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleBackClicked);
	}

	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleJoinClicked);
	}

	if (Button_Refresh)
	{
		Button_Refresh->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleRefreshClicked);
	}

	if (EditableText_RoomNameFilter)
	{
		EditableText_RoomNameFilter->OnTextChanged.AddDynamic(
			this, &USIRoomListWidget::HandleFilterTextChanged);
	}

	if (USISessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->OnFindSessionsCompleteEvent.AddUniqueDynamic(
			this, &USIRoomListWidget::HandleFindSessionsComplete);
		Subsystem->OnJoinSessionCompleteEvent.AddUniqueDynamic(
			this, &USIRoomListWidget::HandleJoinSessionComplete);
	}

	UpdateJoinControls();

	// F5를 받으려면 키보드 포커스가 이 위젯 트리 안에 있어야 한다.
	// Construct 시점엔 아직 Slate 트리에 붙기 전이라 여기서 포커스를 요청하면 조용히 무시된다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				FocusForKeyboardShortcuts();
			}));
	}

	// 목록을 열자마자 한 번 검색
	RefreshRoomList();
}

void USIRoomListWidget::FocusForKeyboardShortcuts()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !IsInViewport())
	{
		return;
	}

	// 이 화면은 UI만 다루므로 GameAndUI가 아니라 UIOnly.
	// GameAndUI면 아무 데나 클릭하는 순간 포커스가 게임 뷰포트로 돌아가 F5가 다시 안 먹는다.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
}

FReply USIRoomListWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F5)
	{
		RefreshRoomList();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void USIRoomListWidget::NativeDestruct()
{
	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &USIRoomListWidget::HandleBackClicked);
	}

	if (Button_Join)
	{
		Button_Join->OnClicked.RemoveDynamic(this, &USIRoomListWidget::HandleJoinClicked);
	}

	if (Button_Refresh)
	{
		Button_Refresh->OnClicked.RemoveDynamic(this, &USIRoomListWidget::HandleRefreshClicked);
	}

	if (EditableText_RoomNameFilter)
	{
		EditableText_RoomNameFilter->OnTextChanged.RemoveDynamic(
			this, &USIRoomListWidget::HandleFilterTextChanged);
	}

	if (USISessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->OnFindSessionsCompleteEvent.RemoveDynamic(
			this, &USIRoomListWidget::HandleFindSessionsComplete);
		Subsystem->OnJoinSessionCompleteEvent.RemoveDynamic(
			this, &USIRoomListWidget::HandleJoinSessionComplete);
	}

	ClearRoomList();

	Super::NativeDestruct();
}

USISessionSubsystem* USIRoomListWidget::GetSessionSubsystem() const
{
	UGameInstance* GameInstanceRef = GetGameInstance();

	return IsValid(GameInstanceRef) ? GameInstanceRef->GetSubsystem<USISessionSubsystem>() : nullptr;
}

void USIRoomListWidget::RefreshRoomList()
{
	// F5 연타/버튼 중복 클릭 방어 — 검색이나 입장이 진행 중이면 무시
	if (bRequestInProgress)
	{
		return;
	}
	
	if (RefreshSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), RefreshSound);
	}

	USISessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!IsValid(Subsystem))
	{
		SetStatusText(FText::FromString(TEXT("세션 시스템을 찾을 수 없습니다")));
		return;
	}

	ClearRoomList();

	bRequestInProgress = true;
	UpdateJoinControls();

	SetStatusText(FText::FromString(TEXT("방을 찾는 중…")));

	Subsystem->FindSessions();
}

void USIRoomListWidget::ClearRoomList()
{
	for (USIRoomEntryWidget* Entry : RoomEntries)
	{
		if (IsValid(Entry))
		{
			Entry->OnRoomEntrySelected.RemoveDynamic(this, &USIRoomListWidget::HandleRoomSelected);
		}
	}

	RoomEntries.Reset();
	SelectedSearchResultIndex = INDEX_NONE;

	if (ScrollBox_RoomList)
	{
		ScrollBox_RoomList->ClearChildren();
	}
}

void USIRoomListWidget::HandleFindSessionsComplete(const TArray<FSISessionInfo>& Sessions, const bool bWasSuccessful)
{
	bRequestInProgress = false;

	if (!bWasSuccessful)
	{
		CachedSessions.Reset();
		ClearRoomList();
		SetStatusText(FText::FromString(TEXT("방 검색에 실패했습니다")));
		UpdateJoinControls();
		return;
	}

	CachedSessions = Sessions;
	RebuildRoomEntries();
}

void USIRoomListWidget::HandleFilterTextChanged(const FText& NewText)
{
	// 검색어가 바뀌어도 재검색은 하지 않는다 — 캐시된 결과만 다시 거른다
	RebuildRoomEntries();
}

void USIRoomListWidget::RebuildRoomEntries()
{
	if (!ScrollBox_RoomList || !RoomEntryWidgetClass)
	{
		SetStatusText(FText::FromString(TEXT("방 목록 위젯 설정이 비어 있습니다")));
		UpdateJoinControls();
		return;
	}

	// 다시 그려도 고르던 방은 유지되게 기억해둔다
	const int32 PreviousSelection = SelectedSearchResultIndex;

	ClearRoomList();

	const FString FilterText = EditableText_RoomNameFilter
		? EditableText_RoomNameFilter->GetText().ToString().TrimStartAndEnd()
		: FString();

	for (const FSISessionInfo& Info : CachedSessions)
	{
		// 빈 검색어 = 전체 표시. 부분 일치 + 대소문자 무시
		if (!FilterText.IsEmpty() && !Info.RoomTitle.Contains(FilterText))
		{
			continue;
		}

		USIRoomEntryWidget* Entry = CreateWidget<USIRoomEntryWidget>(this, RoomEntryWidgetClass);
		if (!IsValid(Entry))
		{
			continue;
		}

		Entry->Setup(Info);
		Entry->OnRoomEntrySelected.AddDynamic(this, &USIRoomListWidget::HandleRoomSelected);

		// 슬롯에 여백을 줘서 항목 사이를 띄운다 (Spacer 위젯을 끼워넣는 것보다 관리가 쉽다)
		if (UScrollBoxSlot* EntrySlot = Cast<UScrollBoxSlot>(ScrollBox_RoomList->AddChild(Entry)))
		{
			EntrySlot->SetPadding(FMargin(0.0f, RoomEntrySpacing * 0.5f));
			EntrySlot->SetHorizontalAlignment(HAlign_Fill);
		}

		RoomEntries.Add(Entry);
	}

	if (CachedSessions.IsEmpty())
	{
		SetStatusText(FText::FromString(TEXT("열려 있는 방이 없습니다")));
	}
	else if (RoomEntries.IsEmpty())
	{
		SetStatusText(FText::FromString(TEXT("검색어와 일치하는 방이 없습니다")));
	}
	else
	{
		SetStatusText(FText::GetEmpty());
	}

	// 하나만 남았으면 자동 선택 — 검색해서 좁힌 뒤 바로 참가할 수 있게
	if (RoomEntries.Num() == 1 && IsValid(RoomEntries[0]))
	{
		HandleRoomSelected(RoomEntries[0]->GetSearchResultIndex());
		return;
	}

	// 여럿이면 고르던 방이 아직 목록에 있을 때만 선택을 되살린다
	if (PreviousSelection != INDEX_NONE)
	{
		for (const USIRoomEntryWidget* Entry : RoomEntries)
		{
			if (IsValid(Entry) && Entry->GetSearchResultIndex() == PreviousSelection)
			{
				HandleRoomSelected(PreviousSelection);
				return;
			}
		}
	}

	UpdateJoinControls();
}

void USIRoomListWidget::HandleRoomSelected(const int32 SearchResultIndex)
{
	// 검색어를 칠 때마다 목록이 다시 그려지며 같은 방이 재선택된다.
	// 그때 입력 중이던 비밀번호를 날리지 않도록 "실제로 바뀐 경우"만 초기화한다.
	const bool bSelectionChanged = (SelectedSearchResultIndex != SearchResultIndex);

	SelectedSearchResultIndex = SearchResultIndex;

	for (USIRoomEntryWidget* Entry : RoomEntries)
	{
		if (IsValid(Entry))
		{
			Entry->SetSelected(Entry->GetSearchResultIndex() == SearchResultIndex);
		}
	}

	// 다른 방을 고른 경우에만 이전 비밀번호 입력을 지운다
	if (bSelectionChanged && EditableText_JoinPassword)
	{
		EditableText_JoinPassword->SetText(FText::GetEmpty());
	}

	SetStatusText(FText::GetEmpty());
	UpdateJoinControls();
}

USIRoomEntryWidget* USIRoomListWidget::FindSelectedEntry() const
{
	if (SelectedSearchResultIndex == INDEX_NONE)
	{
		return nullptr;
	}

	for (USIRoomEntryWidget* Entry : RoomEntries)
	{
		if (IsValid(Entry) && Entry->GetSearchResultIndex() == SelectedSearchResultIndex)
		{
			return Entry;
		}
	}

	return nullptr;
}

void USIRoomListWidget::UpdateJoinControls()
{
	const USIRoomEntryWidget* SelectedEntry = FindSelectedEntry();

	if (Button_Join)
	{
		Button_Join->SetIsEnabled(SelectedEntry != nullptr && !bRequestInProgress);
	}

	if (Button_Refresh)
	{
		Button_Refresh->SetIsEnabled(!bRequestInProgress);
	}

	// 비밀번호 칸은 항상 보인다. 선택에 따라 숨겼다 보였다 하면 하단 UI가 들썩여 산만하고,
	// 비밀번호가 없는 방에 입력해도 서버가 무시하므로 열어둬도 무해하다.
}

void USIRoomListWidget::HandleJoinClicked()
{
	if (bRequestInProgress)
	{
		return;
	}

	const USIRoomEntryWidget* SelectedEntry = FindSelectedEntry();
	if (!SelectedEntry)
	{
		SetStatusText(FText::FromString(TEXT("입장할 방을 선택해주세요")));
		return;
	}

	USISessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!IsValid(Subsystem))
	{
		SetStatusText(FText::FromString(TEXT("세션 시스템을 찾을 수 없습니다")));
		return;
	}

	FString Password;
	if (SelectedEntry->HasPassword())
	{
		Password = EditableText_JoinPassword
			? EditableText_JoinPassword->GetText().ToString()
			: FString();

		if (Password.IsEmpty())
		{
			SetStatusText(FText::FromString(TEXT("비밀번호를 입력해주세요")));
			return;
		}
	}

	bRequestInProgress = true;
	UpdateJoinControls();

	SetStatusText(FText::FromString(TEXT("입장하는 중…")));

	if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
	{
		SIInstance->PrepareForJoin();
	}

	Subsystem->JoinSessionByIndex(SelectedEntry->GetSearchResultIndex(), Password);
}

void USIRoomListWidget::HandleJoinSessionComplete(const bool bWasSuccessful)
{
	bRequestInProgress = false;

	// 성공하면 곧바로 ClientTravel이 걸린다. 이 위젯은 레벨과 함께 사라지므로 할 일이 없다.
	if (bWasSuccessful)
	{
		return;
	}

	// 실패 원인이 비밀번호인지 여부는 서버가 끊은 뒤에야 알 수 있어서
	// 여기서는 일반 안내만 하고, 상세 사유는 메인메뉴 복귀 시 ConsumeLastFailure가 담당한다.
	SetStatusText(FText::FromString(TEXT("입장에 실패했습니다. 목록을 새로 고쳐주세요")));
	UpdateJoinControls();
}

void USIRoomListWidget::HandleRefreshClicked()
{
	RefreshRoomList();
}

void USIRoomListWidget::HandleBackClicked()
{
	HandleCancelled();
}

void USIRoomListWidget::SetStatusText(const FText& InText)
{
	if (Text_Status)
	{
		Text_Status->SetText(InText);
	}
}
