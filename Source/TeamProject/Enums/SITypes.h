#pragma once

#include "CoreMinimal.h"
#include "SITypes.generated.h"

/**
 * 게임의 현재 진행 단계
 */
UENUM(BlueprintType)
enum class ESIGamePhase : uint8
{
	None UMETA(DisplayName = "시작 단계"),
	LobbyPhase UMETA(DisplayName = "로비 단계"),
	BuildPhase UMETA(DisplayName = "제작 단계"),
	GuessPhase UMETA(DisplayName = "정답 맞추기 단계"),
	ResultPhase UMETA(DisplayName = "결과 발표")
};

/**
 * 정답 제출 결과 페이로드 (모든 클라이언트에게 브로드캐스트되는 순간 이벤트)
 */
USTRUCT(BlueprintType)
struct TEAMPROJECT_API FAnswerResultPayload
{
	GENERATED_BODY()

	// 정답을 제출한 플레이어
	UPROPERTY(BlueprintReadOnly, Category = "AnswerResult")
	class APlayerState* Submitter = nullptr;

	// 제출한 답 문자열 (오답 표시 등에 사용)
	UPROPERTY(BlueprintReadOnly, Category = "AnswerResult")
	FString SubmittedAnswer;

	// 이번 제출로 획득한 점수 (오답이면 0)
	UPROPERTY(BlueprintReadOnly, Category = "AnswerResult")
	int32 ScoreEarned = 0;

	// 정답 여부
	UPROPERTY(BlueprintReadOnly, Category = "AnswerResult")
	bool bIsCorrect = false;
};

/**
 * 채팅 메시지 페이로드
 */
USTRUCT(BlueprintType)
struct TEAMPROJECT_API FChatMessagePayload
{
	GENERATED_BODY()

	// 메시지를 보낸 플레이어
	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	class APlayerState* Sender = nullptr;

	// 메시지 본문
	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString Message;
};

USTRUCT(BlueprintType)
struct TEAMPROJECT_API FChatLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString SenderName;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString Message;
};

// 방 설정은 FSICreateSessionParams(GameInstance/SISessionSubsystem.h)로 일원화됨.