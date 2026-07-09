// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/PlacementPreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Object/ShapeDefinitionRow.h"

// 로컬 전용 프리뷰 메시 컴포넌트를 생성한다.
APlacementPreviewActor::APlacementPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// 선택한 ShapeId에 맞는 프리뷰 메시, 머티리얼, 기본 스케일을 적용한다.
bool APlacementPreviewActor::SetPreviewShape(UDataTable* ShapeDefinitionTable, FName ShapeId)
{
	if (!ShapeDefinitionTable || ShapeId.IsNone())
	{
		return false;
	}

	const FShapeDefinitionRow* ShapeDefinition = ShapeDefinitionTable->FindRow<FShapeDefinitionRow>(ShapeId, TEXT("SetPreviewShape"));
	if (!ShapeDefinition || !ShapeDefinition->Mesh)
	{
		return false;
	}

	MeshComponent->SetStaticMesh(ShapeDefinition->Mesh);
	if (ShapeDefinition->PreviewMaterial)
	{
		MeshComponent->SetMaterial(0, ShapeDefinition->PreviewMaterial);
	}

	SetActorScale3D(ShapeDefinition->DefaultScale);
	return true;
}

// 표면 히트 위치를 기준으로 프리뷰 Transform을 갱신한다.
void APlacementPreviewActor::SetSurfaceAlignedTransform(const FVector& SurfacePoint, const FVector& SurfaceNormal, const FRotator& PreviewRotation, const FVector& PreviewScale)
{
	const FVector SafeSurfaceNormal = SurfaceNormal.GetSafeNormal();
	if (SafeSurfaceNormal.IsNearlyZero())
	{
		SetActorLocation(SurfacePoint);
		SetActorRotation(PreviewRotation);
		SetActorScale3D(PreviewScale);
		return;
	}

	const float SurfaceOffset = CalculateSurfaceOffset(SafeSurfaceNormal, PreviewRotation, PreviewScale);
	// 히트 지점에서 표면 바깥 방향으로 필요한 거리만큼 밀어 프리뷰가 표면을 뚫지 않게 한다.
	const FVector PreviewLocation = SurfacePoint + (SafeSurfaceNormal * SurfaceOffset);

	SetActorLocation(PreviewLocation);
	SetActorRotation(PreviewRotation);
	SetActorScale3D(PreviewScale);
}

// 현재 도형이 표면 법선 방향으로 차지하는 반쪽 길이를 구한다.
float APlacementPreviewActor::CalculateSurfaceOffset(const FVector& SurfaceNormal, const FRotator& PreviewRotation, const FVector& PreviewScale) const
{
	if (!MeshComponent || !MeshComponent->GetStaticMesh())
	{
		return 0.0f;
	}

	const FBoxSphereBounds MeshBounds = MeshComponent->GetStaticMesh()->GetBounds();
	const FVector ScaledBoundsOrigin = MeshBounds.Origin * PreviewScale;
	const FVector ScaledBoxExtent = MeshBounds.BoxExtent * PreviewScale.GetAbs();
	const FRotationMatrix PreviewRotationMatrix(PreviewRotation);

	const FVector RotatedBoundsOrigin = PreviewRotationMatrix.TransformVector(ScaledBoundsOrigin);
	const float CenterProjection = FVector::DotProduct(RotatedBoundsOrigin, SurfaceNormal);
	// 회전된 각 로컬 축을 표면 법선에 투영해, 현재 자세에서 필요한 오프셋을 구한다.
	const float ExtentProjection =
		FMath::Abs(FVector::DotProduct(PreviewRotationMatrix.GetUnitAxis(EAxis::X), SurfaceNormal)) * ScaledBoxExtent.X +
		FMath::Abs(FVector::DotProduct(PreviewRotationMatrix.GetUnitAxis(EAxis::Y), SurfaceNormal)) * ScaledBoxExtent.Y +
		FMath::Abs(FVector::DotProduct(PreviewRotationMatrix.GetUnitAxis(EAxis::Z), SurfaceNormal)) * ScaledBoxExtent.Z;

	return FMath::Max(0.0f, ExtentProjection - CenterProjection);
}
