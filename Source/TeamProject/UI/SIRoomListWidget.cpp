// SIRoomListWidget.cpp


#include "UI/SIRoomListWidget.h"

#include "GameInstance/SIGameInstance.h"
#include "Components/Button.h"

void USIRoomListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleBackClicked);
	}

	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleCreateClicked);
	}
}

void USIRoomListWidget::HandleBackClicked()
{
	HandleCancelled();
}

void USIRoomListWidget::HandleCreateClicked()
{
	//테스트 코드입니다. Null SubSystem 도입시 삭제 바랍니다
	if (UWorld* World = GetWorld())
	{
		if (USIGameInstance* GI = World->GetGameInstance<USIGameInstance>())
		{
			GI->JoinRoom(TEXT("127.0.0.1"));   // 임시 하드코딩 접속
		}
	}
	
	HandleConfirmed();
}