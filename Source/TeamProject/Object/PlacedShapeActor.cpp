// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/PlacedShapeActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Object/ShapeDefinitionRow.h"

// 배치 도형의 기본 컴포넌트와 복제 설정을 초기화한다.
APlacedShapeActor::APlacedShapeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	bIsBeingEdited = false;
	ReplicatedScale = FVector::OneVector;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	EditTraceComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("EditTraceComponent"));
	EditTraceComponent->SetupAttachment(MeshComponent);
	EditTraceComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EditTraceComponent->SetCollisionObjectType(ECC_WorldDynamic);
	EditTraceComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	EditTraceComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	EditTraceComponent->SetBoxExtent(FVector(50.0f));
	EditTraceComponent->SetHiddenInGame(true);
}

// 배치 도형의 ShapeId, 편집 상태, 시각 정보를 클라이언트로 복제한다.
void APlacedShapeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlacedShapeActor, ReplicatedShapeId);
	DOREPLIFETIME(APlacedShapeActor, bIsBeingEdited);
	DOREPLIFETIME(APlacedShapeActor, ReplicatedMesh);
	DOREPLIFETIME(APlacedShapeActor, ReplicatedMaterial);
	DOREPLIFETIME(APlacedShapeActor, ReplicatedScale);
}

// 도형 정의의 기본 스케일을 사용해 배치 도형을 설정한다.
bool APlacedShapeActor::SetPlacedShape(UDataTable* ShapeDefinitionTable, FName ShapeId)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone())
	{
		return false;
	}

	const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(ShapeId, TEXT("SetPlacedShape"));
	if (!ShapeDefinition || !ShapeDefinition->Mesh)
	{
		return false;
	}

	return SetPlacedShape(ShapeDefinitionTable, ShapeId, ShapeDefinition->DefaultScale);
}

// 도형 정의와 전달받은 스케일을 사용해 배치 도형을 설정한다.
bool APlacedShapeActor::SetPlacedShape(UDataTable* ShapeDefinitionTable, FName ShapeId, const FVector& PlacedScale)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone())
	{
		return false;
	}

	const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(ShapeId, TEXT("SetPlacedShape"));
	if (!ShapeDefinition || !ShapeDefinition->Mesh)
	{
		return false;
	}

	ReplicatedShapeId = ShapeId;
	ReplicatedMesh = ShapeDefinition->Mesh;
	ReplicatedMaterial = ShapeDefinition->PlacedMaterial;
	ReplicatedScale = PlacedScale;

	ApplyShapeVisuals();
	return true;
}

// 현재 배치 도형의 ShapeId를 반환한다.
FName APlacedShapeActor::GetShapeId() const
{
	return ReplicatedShapeId;
}

// 도형이 편집 중인지 반환한다.
bool APlacedShapeActor::IsBeingEdited() const
{
	return bIsBeingEdited;
}

// 도형의 편집 중 상태를 설정한다.
void APlacedShapeActor::SetBeingEdited(bool bInIsBeingEdited)
{
	bIsBeingEdited = bInIsBeingEdited;
}

// 도형 호버 표시를 켜거나 끈다.
void APlacedShapeActor::SetHovered(bool bHovered)
{
	MeshComponent->SetRenderCustomDepth(bHovered);
	MeshComponent->SetCustomDepthStencilValue(bHovered ? 1 : 0);
}

// 복제된 시각 정보가 변경되면 실제 컴포넌트에 반영한다.
void APlacedShapeActor::OnRep_ShapeVisuals()
{
	ApplyShapeVisuals();
}

// 메시, 머티리얼, 스케일을 배치 도형에 적용한다.
void APlacedShapeActor::ApplyShapeVisuals()
{
	if (ReplicatedMesh)
	{
		MeshComponent->SetStaticMesh(ReplicatedMesh);
		UpdateEditTraceBounds();
	}

	if (ReplicatedMaterial)
	{
		MeshComponent->SetMaterial(0, ReplicatedMaterial);
	}

	SetActorScale3D(ReplicatedScale);
}

// 메시 bounds에 맞게 편집용 트레이스 박스 크기를 맞춘다.
void APlacedShapeActor::UpdateEditTraceBounds()
{
	if (!ReplicatedMesh || !EditTraceComponent)
	{
		return;
	}

	const FBoxSphereBounds MeshBounds = ReplicatedMesh->GetBounds();
	const FVector BoxExtent = MeshBounds.BoxExtent.ComponentMax(FVector(1.0f));

	EditTraceComponent->SetRelativeLocation(MeshBounds.Origin);
	EditTraceComponent->SetBoxExtent(BoxExtent);
}
