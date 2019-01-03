// Fill out your copyright notice in the Description page of Project Settings.

#include "SSH_CameraControlComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "SSH_MathLibrary.h"
#include "GameFramework/PlayerController.h"

const FName USSH_CameraControlComponent::SocketName(TEXT("CameraSocket"));

// Sets default values for this component's properties
USSH_CameraControlComponent::USSH_CameraControlComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bAutoActivate = true;
	bTickInEditor = true;

	bDrawDebug = true;
	ControllMode = ESSH_CameraControlMode::Normal;
	PrevControllerRatation = FRotator::ZeroRotator;
	// ...

	RelativeSocketLocation = FVector::ZeroVector;
	RelativeSocketRotation = FQuat::Identity;

	MovementAxisRotate = FRotator::ZeroRotator;
	AllowZAxisMovement = false;
}


// Called when the game starts
void USSH_CameraControlComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void USSH_CameraControlComponent::UpdateNormalMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// 일반 모드는 컨트롤러의 회전값의 변화량을 카메라 암 회전값으로 사용
	TargetArmRotator += ControlRotatorDelta;

	// 카메라 암의 목표 회전값
	TargetArmRotator = TargetArmRotator;

	// 카메라암의 회전중심 목표위치
	TargetArmOriginLoc = GetComponentLocation();

	// 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CameraTargetLoc = TargetArmOriginLoc - TargetArmRotator.Vector() * TargetArmLength;

	// 카메라의 목표 회전값. 일반모드에선 암의 회전값이 카메라의 목표회전값
	TargetCameraRotator = TargetArmRotator;

	// 보간(카메라렉)처리 보간을 사용하지 않는다면 바로 목표위치를 사용한다. 
	if (DoCameraLag)
	{
		CurrentArmRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetArmRotator, CurrentArmRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentArmOriginLoc = USSH_MathLibrary::SSH_VectorInerp(TargetArmOriginLoc, CurrentArmOriginLoc, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentCameraRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetCameraRotator, CurrentCameraRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
	}
	else
	{
		CurrentArmRotator = TargetArmRotator;
		CurrentArmOriginLoc = TargetArmOriginLoc;
		CurrentCameraRotator = TargetCameraRotator;
	}

	// 보간된 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * TargetArmLength;

	// 카메라 충돌처리 체크
	if (bDoCollisionTest && (TargetArmLength != 0.0f))
	{
		FHitResult HitResult;
		FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(USSH_CameraControlComponent), false, GetOwner());
		GetWorld()->SweepSingleByChannel(HitResult, CurrentArmOriginLoc, CurrentCameraLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), CollisionQueryParams);

		if (HitResult.bBlockingHit)
		{
			CurrentCameraLoc = HitResult.Location;
		}
	}

	// 엑터 이동축 계산
	CalMovementAxis();

	// 디버깅 라인 출력
	if (bDrawDebug)
		DrawDebug();

	// 최종 카메라 소켓 트랜스폼 처리
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);
	// Convert to relative to component
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdateTargetMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (IsValid(TargetActor) == false || IsValid(OwningPawn) == false)
	{
		UpdateNormalMode(DoCollisionTest, DoCameraLag, DeltaTime);
		return;
	}

	// 타깃 모드는 컨트롤러의 회전값을 사용하는게 아니라 타깃으로 설정된 엑터와 의 방향을 기준으로 회전값을 계산
	FVector TargetDirection = TargetActor->GetActorLocation() - OwningPawn->GetActorLocation();
	TargetDirection.Normalize();

	// 카메라 암의 목표 회전값
	TargetArmRotator = TargetDirection.Rotation() + GetRelativeTransform().Rotator();

	// 카메라암의 회전중심 목표위치
	TargetArmOriginLoc = GetComponentLocation();

	// 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CameraTargetLoc = TargetArmOriginLoc - TargetArmRotator.Vector() * TargetArmLength;
	
	// 카메라의 목표 회전값. 타깃모드에선 최종 카메라 위치값 -> 목표물의 방향으로 카메라를 회전시킨다.
	FVector CameraLookDirection = TargetActor->GetActorLocation() - CameraTargetLoc;
	TargetCameraRotator = CameraLookDirection.Rotation();

	// 보간(카메라렉)처리 보간을 사용하지 않는다면 바로 목표위치를 사용한다. 
	if (DoCameraLag)
	{
		CurrentArmRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetArmRotator, CurrentArmRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentArmOriginLoc = USSH_MathLibrary::SSH_VectorInerp(TargetArmOriginLoc, CurrentArmOriginLoc, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentCameraRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetCameraRotator, CurrentCameraRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
	}
	else
	{
		CurrentArmRotator = TargetArmRotator;
		CurrentArmOriginLoc = TargetArmOriginLoc;
		CurrentCameraRotator = TargetCameraRotator;
	}

	// 보간된 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * TargetArmLength;

	// 카메라 충돌처리 체크
	if (bDoCollisionTest && (TargetArmLength != 0.0f))
	{
		FHitResult HitResult;
		FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(USSH_CameraControlComponent), false, GetOwner());
		GetWorld()->SweepSingleByChannel(HitResult, CurrentArmOriginLoc, CurrentCameraLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), CollisionQueryParams);

		if (HitResult.bBlockingHit)
		{
			CurrentCameraLoc = HitResult.Location;
		}
	}

	// 엑터 이동축 계산
	CalMovementAxis();

	// 디버깅 라인 출력
	if (bDrawDebug)
		DrawDebug();

	// 최종 카메라 소켓 트랜스폼 처리
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// 구해진 트렌스폼을 현제 컴포넌트의 로컬 트렌스폼으로 변환
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// 구해진 로컬 트렌스폼을 최종 카메라 위치(소켓) 값으로 이용
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// 아래 함수를 호출하면 GetSocketTransform 함수가 호출되고 해당 함수에서 카메라 소켓의 최종 위치를 반환
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdateFixMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// 카메라의 최종 목표위치 계산 -> 고정 모드는 특정위치가 카메라의 목표위치
	CameraTargetLoc = FixPosition;

	// 카메라의 목표 회전값. 고정모드에선 최종 카메라 위치값 -> 엑터의 방향으로 카메라를 회전시킨다.
	FVector CameraLookDirection = GetOwner()->GetActorLocation() - CameraTargetLoc;
	TargetCameraRotator = CameraLookDirection.Rotation();

	// 카메라 암의 목표 회전값
	TargetArmRotator = TargetCameraRotator;

	// 카메라암의 회전중심 목표위치
	TargetArmOriginLoc = GetComponentLocation();

	// 보간(카메라렉)처리 보간을 사용하지 않는다면 바로 목표위치를 사용한다. 
	if (DoCameraLag)
	{
		CurrentArmRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetArmRotator, CurrentArmRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentArmOriginLoc = USSH_MathLibrary::SSH_VectorInerp(TargetArmOriginLoc, CurrentArmOriginLoc, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentCameraRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetCameraRotator, CurrentCameraRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
	}
	else
	{
		CurrentArmRotator = TargetArmRotator;
		CurrentArmOriginLoc = TargetArmOriginLoc;
		CurrentCameraRotator = TargetCameraRotator;
	}

	float ArmLength = (GetOwner()->GetActorLocation() - CameraTargetLoc).Size();

	// 보간된 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * ArmLength;

	// 고정모드는 카메라 충돌처리를 하지 않는다.

	// 엑터 이동축 계산
	CalMovementAxis();

	// 디버깅 라인 출력
	if (bDrawDebug)
		DrawDebug();

	// 최종 카메라 소켓 트랜스폼 처리
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// 구해진 트렌스폼을 현제 컴포넌트의 로컬 트렌스폼으로 변환
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// 구해진 로컬 트렌스폼을 최종 카메라 위치(소켓) 값으로 이용
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// 아래 함수를 호출하면 GetSocketTransform 함수가 호출되고 해당 함수에서 카메라 소켓의 최종 위치를 반환
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdatePlaneMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// PlaneMode는 카메라가 특정 평면에 붙어서 엑터를 따라가는 모드

	// 카메라의 최종 목표위치 계산. 엑터에서 평면 노멀 벡터로 레이를 쏜 교접이 최종 카메라 위치
	FVector PlaneNormal = CameraPlane.GetSafeNormal();
	FVector ActorLoc = GetOwner()->GetActorLocation();
	FVector LineStart = ActorLoc - PlaneNormal * 100000.0f;
	FVector LineEnd = ActorLoc + PlaneNormal * 100000.0f;
	CameraTargetLoc = FMath::LinePlaneIntersection(LineStart, LineEnd, CameraPlane);

	// 카메라 암의 목표 회전값. 최종 카메라 위치값 -> 엑터의 방향으로 카메라암을 회전시킨다.
	TargetArmRotator = (ActorLoc - CameraTargetLoc).Rotation();

	// 카메라의 목표 회전값. 카메라 암의 목표 회전값과 동일
	TargetCameraRotator = TargetArmRotator;

	// 카메라암의 회전중심 목표위치
	TargetArmOriginLoc = GetComponentLocation();

	// 보간(카메라렉)처리 보간을 사용하지 않는다면 바로 목표위치를 사용한다. 
	if (DoCameraLag)
	{
		CurrentArmRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetArmRotator, CurrentArmRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentArmOriginLoc = USSH_MathLibrary::SSH_VectorInerp(TargetArmOriginLoc, CurrentArmOriginLoc, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentCameraRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetCameraRotator, CurrentCameraRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
	}
	else
	{
		CurrentArmRotator = TargetArmRotator;
		CurrentArmOriginLoc = TargetArmOriginLoc;
		CurrentCameraRotator = TargetCameraRotator;
	}

	// PlaneMode는 TargetArmLength를 사용하지 않고 카메라 위치와 엑터와의 거리를 ArmLength로 사용한다. 
	float ArmLength = (GetOwner()->GetActorLocation() - CameraTargetLoc).Size();

	// 보간된 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * ArmLength;

	// PlaneMode는 카메라 충돌처리를 하지 않는다.

	// 엑터 이동축 계산
	CalMovementAxis();

	// 디버깅 라인 출력
	if (bDrawDebug)
		DrawDebug();

	// 최종 카메라 소켓 트랜스폼 처리
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// 구해진 트렌스폼을 현제 컴포넌트의 로컬 트렌스폼으로 변환
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// 구해진 로컬 트렌스폼을 최종 카메라 위치(소켓) 값으로 이용
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// 아래 함수를 호출하면 GetSocketTransform 함수가 호출되고 해당 함수에서 카메라 소켓의 최종 위치를 반환
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdateCylinderMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// CylinderMode는 회전 중점과 엑터의 위치를 기반으로 카메라가 원통형으로 도는모드 (원형계간을 생각하면 쉽다.)
	// Z축 방향의 실린더 모드만 지원
	FVector ActorLoc = GetOwner()->GetActorLocation();

	FVector ArrangedCylinderOrigin = CylinderOrigin;
	ArrangedCylinderOrigin.Z = ActorLoc.Z;

	// 카메라 암의 목표 회전값. 정렬된 실린더 중점(엑터의 Z축과 높이를 맞춘 실린더 중점) -> 엑터의 방향으로 카메라암을 회전시킨다.
	TargetArmRotator = (ArrangedCylinderOrigin - ActorLoc).Rotation();

	// 카메라의 목표 회전값. 카메라 암의 목표 회전값과 동일
	TargetCameraRotator = TargetArmRotator;

	// 카메라암의 회전중심 목표위치
	TargetArmOriginLoc = GetComponentLocation();

	CameraTargetLoc = ActorLoc - (TargetArmRotator.Vector() * TargetArmLength);

	// 보간(카메라렉)처리 보간을 사용하지 않는다면 바로 목표위치를 사용한다. 
	if (DoCameraLag)
	{
		CurrentArmRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetArmRotator, CurrentArmRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentArmOriginLoc = USSH_MathLibrary::SSH_VectorInerp(TargetArmOriginLoc, CurrentArmOriginLoc, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
		CurrentCameraRotator = USSH_MathLibrary::SSH_RotatorInerp(TargetCameraRotator, CurrentCameraRotator, DeltaTime, CameraLagSpeed, CameraLagMaxTimeStep);
	}
	else
	{
		CurrentArmRotator = TargetArmRotator;
		CurrentArmOriginLoc = TargetArmOriginLoc;
		CurrentCameraRotator = TargetCameraRotator;
	}

	// 보간된 카메라의 최종 목표위치 계산 -> 목표 회전값을 중심으로 회전한만큼의 위치
	CurrentCameraLoc = CurrentArmOriginLoc - (CurrentArmRotator.Vector() * TargetArmLength);

	// 실린더모드는 카메라 충돌처리를 하지 않는다.

	// 엑터 이동축 계산
	CalMovementAxis();

	// 디버깅 라인 출력
	if (bDrawDebug)
		DrawDebug();

	// 최종 카메라 소켓 트랜스폼 처리
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// 구해진 트렌스폼을 현제 컴포넌트의 로컬 트렌스폼으로 변환
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// 구해진 로컬 트렌스폼을 최종 카메라 위치(소켓) 값으로 이용
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// 아래 함수를 호출하면 GetSocketTransform 함수가 호출되고 해당 함수에서 카메라 소켓의 최종 위치를 반환
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::CalMovementAxis()
{
	// 이동축 계산은 카메라 보는 방향->이동 전방(MoveUp-Down)
	// 카메라 우측방향이 -> 측방 이동(MoveLeft-Right)

	// 카메라의 월드회전 타깃을 받아온후 이동축 회전 보정을 더해서 처리
	FRotator MovementAxisRotator = TargetCameraRotator + MovementAxisRotate;

	// 전방벡터 회전후 해당 축을 앞뒤 이동 기준으로 사용.
	MovementFowardDir = MovementAxisRotator.RotateVector(FVector::ForwardVector);

	// z축이동을 허용처리(엑터가 하늘로??)
	if (false == AllowZAxisMovement)
		MovementFowardDir.Z = 0.0f;

	// 최종 산출된 이동축 정규화
	MovementFowardDir.Normalize();


	// 측면이동처리도 동일
	MovementRightDir = MovementAxisRotator.RotateVector(FVector::RightVector);

	if (false == AllowZAxisMovement)
		MovementRightDir.Z = 0.0f;

	MovementRightDir.Normalize();
}

void USSH_CameraControlComponent::DrawDebug()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// 카메라암의 회전중심 목표위치 표시
	DrawDebugSphere(GetWorld(), TargetArmOriginLoc, 16.0f, 8, FColor::Red);

	// 카메라의 최종 목표위치
	DrawDebugDirectionalArrow(GetWorld(), TargetArmOriginLoc, CameraTargetLoc, 14.0f, FColor::Red);

	// 카메라의 목표 회전값
	DrawDebugDirectionalArrow(GetWorld(), CameraTargetLoc, (CameraTargetLoc + TargetCameraRotator.Vector() * 50.0f), 14.0f, FColor::Blue);

	// 보간중인 카메라암의 회전중심 목표위치 표시
	DrawDebugSphere(GetWorld(), CurrentArmOriginLoc, 16.0f, 8, FColor::Yellow);

	// 보간중이 카메라의 최종 목표위치
	DrawDebugDirectionalArrow(GetWorld(), CurrentArmOriginLoc, CurrentCameraLoc, 14.0f, FColor::Yellow);

	// 보간중인 카메라의 목표 회전값
	DrawDebugDirectionalArrow(GetWorld(), CurrentCameraLoc, (CurrentCameraLoc + CurrentCameraRotator.Vector() * 50.0f), 14.0f, FColor::Cyan);

	// 이동 축 디버깅 드로우
	FVector ActorLoc = GetOwner()->GetActorLocation();
	DrawDebugDirectionalArrow(GetWorld(), ActorLoc, (ActorLoc + MovementFowardDir * 50.0f), 14.0f, FColor::Red);

	DrawDebugDirectionalArrow(GetWorld(), ActorLoc, (ActorLoc + MovementRightDir * 50.0f), 14.0f, FColor::Green);
#endif
}

// Called every frame
void USSH_CameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ControlRotatorDelta = FRotator::ZeroRotator;

	// 컨트롤러 회전값 델타를 산출
	APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (IsValid(OwningPawn))
	{
		FRotator ControllerRotator = OwningPawn->GetControlRotation();

		APlayerController* PlayerController = OwningPawn->GetController<APlayerController>();
		if (IsValid(PlayerController))
		{
			//ControlRotatorDelta = PlayerController->RotationInput;
			ControlRotatorDelta = ControllerRotator - PrevControllerRatation;
		}
		PrevControllerRatation = ControllerRotator;
	}
	else
	{
		PrevControllerRatation = FRotator::ZeroRotator;
	}

	switch (ControllMode)
	{
	case ESSH_CameraControlMode::Normal:
		UpdateNormalMode(bDoCollisionTest, bEnableCameraLag, DeltaTime);
		break;
	case ESSH_CameraControlMode::Targeting:
		UpdateTargetMode(bDoCollisionTest, bEnableCameraLag, DeltaTime);
		break;
	case ESSH_CameraControlMode::Fix:
		UpdateFixMode(bDoCollisionTest, bEnableCameraLag, DeltaTime);
		break;
	case ESSH_CameraControlMode::Plane:
		UpdatePlaneMode(bDoCollisionTest, bEnableCameraLag, DeltaTime);
		break;
	case ESSH_CameraControlMode::Cylinder:
		UpdateCylinderMode(bDoCollisionTest, bEnableCameraLag, DeltaTime);
		break;
	default:
		break;
	}

	// ...
}

void USSH_CameraControlComponent::OnRegister()
{
	Super::OnRegister();

	TargetArmRotator = FRotator(GetComponentTransform().GetRotation());
	CurrentArmRotator = TargetArmRotator;

	TargetCameraRotator = FRotator::ZeroRotator;
	CurrentCameraRotator = TargetCameraRotator;

	TargetArmOriginLoc = GetComponentLocation();
	CurrentArmOriginLoc = TargetArmOriginLoc;

	UpdateNormalMode(bDoCollisionTest, bEnableCameraLag, 0.0f);
}

// USceneComponent interface
bool USSH_CameraControlComponent::HasAnySockets() const
{
	return true;
}

FTransform USSH_CameraControlComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace /*= RTS_World*/) const
{
	FTransform RelativeTransform(RelativeSocketRotation, RelativeSocketLocation);

	switch (TransformSpace)
	{
	case RTS_World:
	{
		return RelativeTransform * GetComponentTransform();
		break;
	}
	case RTS_Actor:
	{
		if (const AActor* Actor = GetOwner())
		{
			FTransform SocketTransform = RelativeTransform * GetComponentTransform();
			return SocketTransform.GetRelativeTransform(Actor->GetTransform());
		}
		break;
	}
	case RTS_Component:
	{
		return RelativeTransform;
	}
	}
	return RelativeTransform;
}

void USSH_CameraControlComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	new (OutSockets) FComponentSocketDescription(SocketName, EComponentSocketType::Socket);
}
// End of USceneComponent interface

FVector USSH_CameraControlComponent::GetCaracterMovementDir(FVector2D InputAxis)
{
	return (MovementFowardDir * InputAxis.X) + (MovementRightDir * InputAxis.Y);
}