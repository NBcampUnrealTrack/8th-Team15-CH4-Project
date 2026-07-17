// SIUIManagerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SIUIManagerComponent.generated.h"

class USIUserWidget;

UENUM()
enum class EUIType : uint8
{
	Exit,			// 종료 확인창
	CreateRoom,		// 방 만들기 설정
	RoomList,		// 방 목록
	LobbySetting,	// 방 설정 (로비 내)
};

USTRUCT()
struct FUIStackEntry
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EUIType UIType = EUIType::Exit;

	UPROPERTY()
	TObjectPtr<USIUserWidget> Widget;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIConfirmed, EUIType, type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateRoomRequested, const FSIRoomSettings&, payload);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEAMPROJECT_API USIUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USIUIManagerComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY()
	FOnUIConfirmed OnUIConfirmed;

	UPROPERTY()
	FOnCreateRoomRequested OnCreateRoomRequested;

private:
	UFUNCTION()
	void OnWidgetConfirmed();

	UFUNCTION()
	void OnWidgetCancelled();

	UFUNCTION()
	void HandleCreateRoomRequested();

private:
	UPROPERTY(EditAnywhere, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TMap<EUIType, TSubclassOf<USIUserWidget>> WidgetClasses;

	UPROPERTY()
	TArray<FUIStackEntry> WidgetStack;

public:
	void OpenWidget(EUIType Type);

	void CloseWidget();
	
};
