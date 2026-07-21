// SILobbySettingWidget.cpp

#include "UI/SILobbySettingWidget.h"
#include "GameState/SIGameState.h"
#include "GameInstance/SIGameInstance.h"
#include "GameInstance/SISessionSubsystem.h"
#include "PlayerState/SIPlayerState.h"

#include "Engine/GameInstance.h"

#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "PlayerController/SIPlayerController.h"
#include "UI/SIChatLineWidget.h"

void USILobbySettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	HandleHostStatusChanged(bIsLocalPlayerHost);
	ResolveHostStatus();
	ResolveRoomInfo();

	if (Button_GameStart)
	{
		Button_GameStart->OnClicked.AddDynamic(this, &USILobbySettingWidget::RequestStartGame);
	}

	if (Button_LeaveRoom)
	{
		Button_LeaveRoom->OnClicked.AddDynamic(this, &USILobbySettingWidget::RequestLeaveRoom);
	}

	for (UEditableText* SettingText : { EditableText_RoomName.Get(), EditableText_RoomPassword.Get(),
		EditableText_BuildTimeLimit.Get(), EditableText_MaxPlacedShapeCount.Get() })
	{
		if (IsValid(SettingText))
		{
			SettingText->OnTextCommitted.AddDynamic(
				this, &USILobbySettingWidget::HandleRoomSettingCommitted);
		}
	}

	if (IsValid(EditableText_RoomPassword))
	{
		EditableText_RoomPassword->OnTextChanged.AddDynamic(
			this, &USILobbySettingWidget::HandleRoomPasswordChanged);
	}

	if (IsValid(EditableText_MaxPlacedShapeCount))
	{
		EditableText_MaxPlacedShapeCount->OnTextChanged.AddDynamic(
			this, &USILobbySettingWidget::HandleMaxPlacedShapeCountChanged);
	}

	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->OnTextCommitted.AddDynamic(
			this, &USILobbySettingWidget::HandleChatCommitted);
	}

	// 과거 대화를 먼저 그리고 → 실시간 구독을 붙인다.
	// (순서가 반대면 복원 중에 들어온 메시지가 뒤엉킨다)
	RestoreChatHistoryTo(ScrollBox_ChatLog, ChatLineWidgetClass);

	// Enter 포커스가 이 위젯의 채팅창으로 오도록 등록 (로비엔 HUD가 없다)
	if (ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>())
	{
		PC->RegisterChatWidget(this);
	}
}

void USILobbySettingWidget::FocusChatInput()
{
	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->SetKeyboardFocus();
	}
}

void USILobbySettingWidget::HandleChatMessage(const FChatMessagePayload& Payload)
{
	// 저장은 ASIPlayerController가 한다 — 여기서 또 저장하면 같은 줄이 두 번 쌓인다
	AddChatLineTo(ScrollBox_ChatLog, ChatLineWidgetClass,
		ResolveChatSenderName(Payload), Payload.Message);
}

void USILobbySettingWidget::HandleChatCommitted(const FText& Chat, const ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>();
	const FString Message = Chat.ToString().TrimStartAndEnd();

	if (!Message.IsEmpty() && IsValid(PC))
	{
		PC->Server_SendChat(Message);
	}

	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->SetText(FText::GetEmpty());
	}

	// 빈 메시지로 엔터를 쳐도 입력 모드는 닫는다 (HUD와 동일한 동작)
	if (IsValid(PC))
	{
		PC->EndChatFocus();
	}
}

void USILobbySettingWidget::NativeDestruct()
{
	if (Button_GameStart)
	{
		Button_GameStart->OnClicked.RemoveDynamic(this, &USILobbySettingWidget::RequestStartGame);
	}

	if (Button_LeaveRoom)
	{
		Button_LeaveRoom->OnClicked.RemoveDynamic(this, &USILobbySettingWidget::RequestLeaveRoom);
	}

	for (UEditableText* SettingText : { EditableText_RoomName.Get(), EditableText_RoomPassword.Get(),
		EditableText_BuildTimeLimit.Get(), EditableText_MaxPlacedShapeCount.Get() })
	{
		if (IsValid(SettingText))
		{
			SettingText->OnTextCommitted.RemoveDynamic(
				this, &USILobbySettingWidget::HandleRoomSettingCommitted);
		}
	}

	if (IsValid(EditableText_RoomPassword))
	{
		EditableText_RoomPassword->OnTextChanged.RemoveDynamic(
			this, &USILobbySettingWidget::HandleRoomPasswordChanged);
	}

	if (IsValid(EditableText_MaxPlacedShapeCount))
	{
		EditableText_MaxPlacedShapeCount->OnTextChanged.RemoveDynamic(
			this, &USILobbySettingWidget::HandleMaxPlacedShapeCountChanged);
	}

	if (CachedPlayerState.IsValid())
	{
		CachedPlayerState->OnHostStatusChanged.RemoveDynamic(this, &USILobbySettingWidget::HandleHostStatusChanged);
	}

	if (CachedGameState.IsValid())
	{
		CachedGameState->OnLobbyRoomInfoChanged.RemoveDynamic(this, &USILobbySettingWidget::RefreshRoomInfo);
		CachedGameState->OnChatMessage.RemoveDynamic(this, &USILobbySettingWidget::HandleChatMessage);
	}

	if (IsValid(EditableText_ChatInput))
	{
		EditableText_ChatInput->OnTextCommitted.RemoveDynamic(
			this, &USILobbySettingWidget::HandleChatCommitted);
	}

	if (ASIPlayerController* PC = GetOwningPlayer<ASIPlayerController>())
	{
		PC->UnregisterChatWidget(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostResolveRetryTimer);
		World->GetTimerManager().ClearTimer(RoomInfoResolveRetryTimer);
	}

	Super::NativeDestruct();
}

void USILobbySettingWidget::RequestStartGame()
{
	if (!bIsLocalPlayerHost)
	{
		return;
	}
	
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	PS->Server_RequestStartGame();
}

void USILobbySettingWidget::RequestLeaveRoom()
{
	// 나가기는 호스트/참가자 구분 없이 누구나 가능하다.
	// 뒷정리(세션 파괴 → 메인메뉴 복귀)는 GameInstance가 일괄 처리한다.
	if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
	{
		SIInstance->LeaveRoom();
	}
}

void USILobbySettingWidget::HandleRoomPasswordChanged(const FText& Text)
{
	FilterEditableTextToDigits(EditableText_RoomPassword, Text);
}

void USILobbySettingWidget::HandleMaxPlacedShapeCountChanged(const FText& Text)
{
	FilterEditableTextToDigits(EditableText_MaxPlacedShapeCount, Text);
}

void USILobbySettingWidget::HandleRoomSettingCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	// 참가자 칸은 ReadOnly라 여기까지 오지 않지만, 호스트 승계 직후 등을 대비해 한 번 더 막는다
	if (!bIsLocalPlayerHost)
	{
		return;
	}

	// 포커스만 스쳐 지나간 경우(OnCleared)는 무시 — 실제 편집 확정만 보낸다
	if (CommitMethod != ETextCommit::OnEnter && CommitMethod != ETextCommit::OnUserMovedFocus)
	{
		return;
	}

	SubmitRoomSettings();
}

void USILobbySettingWidget::SubmitRoomSettings()
{
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	// 칸 하나만 바뀌어도 네 값을 모두 실어 보낸다 — 서버가 항상 완전한 설정을 받게 해서
	// 부분 갱신으로 값이 어긋나는 경우를 없앤다.
	const FString RoomTitle = EditableText_RoomName
		? EditableText_RoomName->GetText().ToString().TrimStartAndEnd()
		: FString();

	const FString Password = EditableText_RoomPassword
		? EditableText_RoomPassword->GetText().ToString()
		: FString();

	// 빈칸/숫자가 아니면 0 = "미지정" 센티널이 되어 GameMode 기본값이 쓰인다
	const float BuildTime = EditableText_BuildTimeLimit
		? FCString::Atof(*EditableText_BuildTimeLimit->GetText().ToString())
		: 0.0f;

	const int32 MaxPlacedShapeCount = EditableText_MaxPlacedShapeCount
		? FCString::Atoi(*EditableText_MaxPlacedShapeCount->GetText().ToString())
		: SIRoomSettingLimits::DefaultPlacedShapeCount;

	PS->Server_UpdateRoomSettings(RoomTitle, Password, BuildTime, MaxPlacedShapeCount);
}

void USILobbySettingWidget::HandleHostStatusChanged(bool bNewIsHost)
{
	bIsLocalPlayerHost = bNewIsHost;
	
	if (IsValid(Button_GameStart))
	{
		if (bIsLocalPlayerHost)
		{
			Button_GameStart->SetVisibility(ESlateVisibility::Visible);
			Button_GameStart->SetIsEnabled(true);
		}
		else
		{
			Button_GameStart->SetVisibility(ESlateVisibility::Collapsed);
			Button_GameStart->SetIsEnabled(false);
		}
	}

	// 방 설정을 바꿀 수 있는 건 호스트뿐 — 나머지는 값을 보기만 한다
	const bool bReadOnly = !bIsLocalPlayerHost;
	for (UEditableText* SettingText : { EditableText_RoomName.Get(), EditableText_RoomPassword.Get(),
		EditableText_BuildTimeLimit.Get(), EditableText_MaxPlacedShapeCount.Get() })
	{
		if (IsValid(SettingText))
		{
			SettingText->SetIsReadOnly(bReadOnly);
		}
	}

	// 호스트 판정이 방 설정 표시보다 늦게 확정될 수 있어 비밀번호 칸을 다시 채운다
	RefreshRoomInfo();
}

void USILobbySettingWidget::ResolveHostStatus()
{
	ASIPlayerState* PS = GetOwningPlayerState<ASIPlayerState>();
	if (!IsValid(PS))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				HostResolveRetryTimer, this, &USILobbySettingWidget::ResolveHostStatus, 0.1f, false);
		}
		return;
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostResolveRetryTimer);
	}

	CachedPlayerState = PS;

	PS->OnHostStatusChanged.AddDynamic(this, &USILobbySettingWidget::HandleHostStatusChanged);
	HandleHostStatusChanged(PS->bIsHost);
}

void USILobbySettingWidget::ResolveRoomInfo()
{
	ASIGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASIGameState>() : nullptr;
	if (!IsValid(GS))
	{
		// 클라이언트는 GameState 복제가 위젯 생성보다 늦을 수 있다
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				RoomInfoResolveRetryTimer, this, &USILobbySettingWidget::ResolveRoomInfo, 0.1f, false);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoomInfoResolveRetryTimer);
	}

	CachedGameState = GS;

	GS->OnLobbyRoomInfoChanged.AddDynamic(this, &USILobbySettingWidget::RefreshRoomInfo);
	GS->OnChatMessage.AddDynamic(this, &USILobbySettingWidget::HandleChatMessage);
	RefreshRoomInfo();
}

void USILobbySettingWidget::RefreshRoomInfo()
{
	const ASIGameState* GS = CachedGameState.Get();
	if (!IsValid(GS))
	{
		return;
	}

	if (IsValid(EditableText_RoomName))
	{
		EditableText_RoomName->SetText(FText::FromString(GS->LobbyRoomTitle));
	}

	// 제한 시간 0 이하 = GameMode 기본값 사용 → 빈 칸으로 표시
	auto FormatTimeLimit = [](const float Seconds)
	{
		return Seconds > 0.0f
			? FText::FromString(FString::FromInt(FMath::RoundToInt(Seconds)))
			: FText::GetEmpty();
	};

	if (IsValid(EditableText_BuildTimeLimit))
	{
		EditableText_BuildTimeLimit->SetText(FormatTimeLimit(GS->LobbyBuildTime));
	}

	if (IsValid(EditableText_MaxPlacedShapeCount))
	{
		EditableText_MaxPlacedShapeCount->SetText(
			FText::AsNumber(GS->LobbyMaxPlacedShapeCount));
	}

	// 비밀번호는 복제되지 않는다 — 호스트만 자기 서브시스템에서 원본을 읽는다
	if (IsValid(EditableText_RoomPassword))
	{
		FString PasswordToShow;
		if (bIsLocalPlayerHost)
		{
			if (const UGameInstance* GameInstanceRef = GetGameInstance())
			{
				if (const USISessionSubsystem* Subsystem = GameInstanceRef->GetSubsystem<USISessionSubsystem>())
				{
					PasswordToShow = Subsystem->GetHostSessionParams().Password;
				}
			}
		}

		EditableText_RoomPassword->SetText(FText::FromString(PasswordToShow));
	}
}
