#include "PlayerController/SIPlayerController.h" // ������ �ִٸ� "PlayerController/SIPlayerController.h"
#include "GameMode/SIGameMode.h"         // ������ �ִٸ� "GameMode/SIGameMode.h"
#include "GameState/SIGameState.h"        // ������ �ִٸ� "GameState/SIGameState.h"
#include "UI/DetailPanelWidget.h"

#pragma region GameMode

// ==========================================
// [Client -> Server] ���� ���� ���� ������
// ==========================================
void ASIPlayerController::Server_SubmitAnswer_Implementation(const FString& Answer)
{
	if (HasAuthority())
	{
		ASIGameMode* GameMode = GetWorld()->GetAuthGameMode<ASIGameMode>();
		if (GameMode)
		{
			GameMode->OnAnswerSubmitted(this, Answer);
			UE_LOG(LogTemp, Warning, TEXT("[����] Ŭ���̾�Ʈ�� ������ �ܾ� ����: %s"), *Answer);
		}
	}
}

// ==========================================
// [Server -> Client] ���þ� ���� ���� ������
// ==========================================
void ASIPlayerController::Client_ReceiveSecretWord_Implementation(const FString& SecretWord)
{
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
	UE_LOG(LogTemp, Warning, TEXT("[Ŭ���̾�Ʈ] ����� �̹� �� ���� ���þ�� [%s] �Դϴ�!"), *SecretWord);
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
}

// ==========================================
// [�׽�Ʈ �ܼ� ��ɾ�] 
// ==========================================
void ASIPlayerController::TestAnswer(const FString& Answer)
{
	UE_LOG(LogTemp, Warning, TEXT("�ֿܼ��� ���� ���� �õ� ��... ���� �ܾ�: %s"), *Answer);
	Server_SubmitAnswer(Answer);
}

void ASIPlayerController::SetPhase(int32 PhaseIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("[�׽�Ʈ] ������ ���� ��� ����: %d"), PhaseIndex);
	Server_TestSetPhase(PhaseIndex);
}

void ASIPlayerController::SetTime(int32 Seconds)
{
	UE_LOG(LogTemp, Warning, TEXT("[�׽�Ʈ] Ÿ�̸� ���� ��� ����: %d��"), Seconds);
	Server_TestSetTime(Seconds);
}

// ==========================================
// [�׽�Ʈ RPC] ���ڵ� ���� ������ ���� ���� �α� ���
// ==========================================
void ASIPlayerController::Server_TestSetPhase_Implementation(int32 PhaseIndex)
{
	if (HasAuthority())
	{
		ASIGameState* SIGameState = GetWorld()->GetGameState<ASIGameState>();
		if (SIGameState)
		{
			switch (PhaseIndex)
			{
			case 1:
				SIGameState->CurrentGamePhase = ESIGamePhase::BuildPhase;
				UE_LOG(LogTemp, Warning, TEXT("[Server] Changed to BuildPhase"));
				break;
			case 2:
				SIGameState->CurrentGamePhase = ESIGamePhase::GuessPhase;
				UE_LOG(LogTemp, Warning, TEXT("[Server] Changed to GuessPhase"));
				break;
			default:
				SIGameState->CurrentGamePhase = ESIGamePhase::None;
				UE_LOG(LogTemp, Warning, TEXT("[Server] Changed to None"));
				break;
			}
		}
	}
}

void ASIPlayerController::Server_TestSetTime_Implementation(int32 Seconds)
{
	if (HasAuthority())
	{
		ASIGameState* SIGameState = GetWorld()->GetGameState<ASIGameState>();
		if (SIGameState)
		{
			SIGameState->RemainingTime = Seconds;
			UE_LOG(LogTemp, Warning, TEXT("[Server] Timer set to %d seconds"), Seconds);
		}
	}
}

#pragma endregion

void ASIPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	// 인스턴스가 없을 때, StaticClass만 존재한다면
	if (!DetailPanelWidgetInstance && DetailPanelWidget)
	{
		// StaticClass를 통해 Instance화
		DetailPanelWidgetInstance = CreateWidget<UDetailPanelWidget>(this, DetailPanelWidget);
	}

	// 인스턴스가 존재한다면
	if (DetailPanelWidgetInstance)
	{
		// 뷰포트에 노출
		DetailPanelWidgetInstance->AddToViewport();
	}
}