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
		EditableText_BuildTimeLimit.Get(), EditableText_GuessTimeLimit.Get() })
	{
		if (IsValid(SettingText))
		{
			SettingText->OnTextCommitted.AddDynamic(
				this, &USILobbySettingWidget::HandleRoomSettingCommitted);
		}
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
		EditableText_BuildTimeLimit.Get(), EditableText_GuessTimeLimit.Get() })
	{
		if (IsValid(SettingText))
		{
			SettingText->OnTextCommitted.RemoveDynamic(
				this, &USILobbySettingWidget::HandleRoomSettingCommitted);
		}
	}

	if (CachedPlayerState.IsValid())
	{
		CachedPlayerState->OnHostStatusChanged.RemoveDynamic(this, &USILobbySettingWidget::HandleHostStatusChanged);
	}

	if (CachedGameState.IsValid())
	{
		CachedGameState->OnLobbyRoomInfoChanged.RemoveDynamic(this, &USILobbySettingWidget::RefreshRoomInfo);
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

	const float GuessTime = EditableText_GuessTimeLimit
		? FCString::Atof(*EditableText_GuessTimeLimit->GetText().ToString())
		: 0.0f;

	PS->Server_UpdateRoomSettings(RoomTitle, Password, BuildTime, GuessTime);
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
		EditableText_BuildTimeLimit.Get(), EditableText_GuessTimeLimit.Get() })
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

	if (IsValid(EditableText_GuessTimeLimit))
	{
		EditableText_GuessTimeLimit->SetText(FormatTimeLimit(GS->LobbyGuessTime));
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
