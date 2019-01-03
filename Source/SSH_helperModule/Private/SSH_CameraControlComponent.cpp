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
	// �Ϲ� ���� ��Ʈ�ѷ��� ȸ������ ��ȭ���� ī�޶� �� ȸ�������� ���
	TargetArmRotator += ControlRotatorDelta;

	// ī�޶� ���� ��ǥ ȸ����
	TargetArmRotator = TargetArmRotator;

	// ī�޶���� ȸ���߽� ��ǥ��ġ
	TargetArmOriginLoc = GetComponentLocation();

	// ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CameraTargetLoc = TargetArmOriginLoc - TargetArmRotator.Vector() * TargetArmLength;

	// ī�޶��� ��ǥ ȸ����. �Ϲݸ�忡�� ���� ȸ������ ī�޶��� ��ǥȸ����
	TargetCameraRotator = TargetArmRotator;

	// ����(ī�޶�)ó�� ������ ������� �ʴ´ٸ� �ٷ� ��ǥ��ġ�� ����Ѵ�. 
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

	// ������ ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * TargetArmLength;

	// ī�޶� �浹ó�� üũ
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

	// ���� �̵��� ���
	CalMovementAxis();

	// ����� ���� ���
	if (bDrawDebug)
		DrawDebug();

	// ���� ī�޶� ���� Ʈ������ ó��
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

	// Ÿ�� ���� ��Ʈ�ѷ��� ȸ������ ����ϴ°� �ƴ϶� Ÿ������ ������ ���Ϳ� �� ������ �������� ȸ������ ���
	FVector TargetDirection = TargetActor->GetActorLocation() - OwningPawn->GetActorLocation();
	TargetDirection.Normalize();

	// ī�޶� ���� ��ǥ ȸ����
	TargetArmRotator = TargetDirection.Rotation() + GetRelativeTransform().Rotator();

	// ī�޶���� ȸ���߽� ��ǥ��ġ
	TargetArmOriginLoc = GetComponentLocation();

	// ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CameraTargetLoc = TargetArmOriginLoc - TargetArmRotator.Vector() * TargetArmLength;
	
	// ī�޶��� ��ǥ ȸ����. Ÿ���忡�� ���� ī�޶� ��ġ�� -> ��ǥ���� �������� ī�޶� ȸ����Ų��.
	FVector CameraLookDirection = TargetActor->GetActorLocation() - CameraTargetLoc;
	TargetCameraRotator = CameraLookDirection.Rotation();

	// ����(ī�޶�)ó�� ������ ������� �ʴ´ٸ� �ٷ� ��ǥ��ġ�� ����Ѵ�. 
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

	// ������ ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * TargetArmLength;

	// ī�޶� �浹ó�� üũ
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

	// ���� �̵��� ���
	CalMovementAxis();

	// ����� ���� ���
	if (bDrawDebug)
		DrawDebug();

	// ���� ī�޶� ���� Ʈ������ ó��
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// ������ Ʈ�������� ���� ������Ʈ�� ���� Ʈ���������� ��ȯ
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// ������ ���� Ʈ�������� ���� ī�޶� ��ġ(����) ������ �̿�
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// �Ʒ� �Լ��� ȣ���ϸ� GetSocketTransform �Լ��� ȣ��ǰ� �ش� �Լ����� ī�޶� ������ ���� ��ġ�� ��ȯ
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdateFixMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// ī�޶��� ���� ��ǥ��ġ ��� -> ���� ���� Ư����ġ�� ī�޶��� ��ǥ��ġ
	CameraTargetLoc = FixPosition;

	// ī�޶��� ��ǥ ȸ����. ������忡�� ���� ī�޶� ��ġ�� -> ������ �������� ī�޶� ȸ����Ų��.
	FVector CameraLookDirection = GetOwner()->GetActorLocation() - CameraTargetLoc;
	TargetCameraRotator = CameraLookDirection.Rotation();

	// ī�޶� ���� ��ǥ ȸ����
	TargetArmRotator = TargetCameraRotator;

	// ī�޶���� ȸ���߽� ��ǥ��ġ
	TargetArmOriginLoc = GetComponentLocation();

	// ����(ī�޶�)ó�� ������ ������� �ʴ´ٸ� �ٷ� ��ǥ��ġ�� ����Ѵ�. 
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

	// ������ ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * ArmLength;

	// �������� ī�޶� �浹ó���� ���� �ʴ´�.

	// ���� �̵��� ���
	CalMovementAxis();

	// ����� ���� ���
	if (bDrawDebug)
		DrawDebug();

	// ���� ī�޶� ���� Ʈ������ ó��
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// ������ Ʈ�������� ���� ������Ʈ�� ���� Ʈ���������� ��ȯ
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// ������ ���� Ʈ�������� ���� ī�޶� ��ġ(����) ������ �̿�
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// �Ʒ� �Լ��� ȣ���ϸ� GetSocketTransform �Լ��� ȣ��ǰ� �ش� �Լ����� ī�޶� ������ ���� ��ġ�� ��ȯ
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdatePlaneMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// PlaneMode�� ī�޶� Ư�� ��鿡 �پ ���͸� ���󰡴� ���

	// ī�޶��� ���� ��ǥ��ġ ���. ���Ϳ��� ��� ��� ���ͷ� ���̸� �� ������ ���� ī�޶� ��ġ
	FVector PlaneNormal = CameraPlane.GetSafeNormal();
	FVector ActorLoc = GetOwner()->GetActorLocation();
	FVector LineStart = ActorLoc - PlaneNormal * 100000.0f;
	FVector LineEnd = ActorLoc + PlaneNormal * 100000.0f;
	CameraTargetLoc = FMath::LinePlaneIntersection(LineStart, LineEnd, CameraPlane);

	// ī�޶� ���� ��ǥ ȸ����. ���� ī�޶� ��ġ�� -> ������ �������� ī�޶���� ȸ����Ų��.
	TargetArmRotator = (ActorLoc - CameraTargetLoc).Rotation();

	// ī�޶��� ��ǥ ȸ����. ī�޶� ���� ��ǥ ȸ������ ����
	TargetCameraRotator = TargetArmRotator;

	// ī�޶���� ȸ���߽� ��ǥ��ġ
	TargetArmOriginLoc = GetComponentLocation();

	// ����(ī�޶�)ó�� ������ ������� �ʴ´ٸ� �ٷ� ��ǥ��ġ�� ����Ѵ�. 
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

	// PlaneMode�� TargetArmLength�� ������� �ʰ� ī�޶� ��ġ�� ���Ϳ��� �Ÿ��� ArmLength�� ����Ѵ�. 
	float ArmLength = (GetOwner()->GetActorLocation() - CameraTargetLoc).Size();

	// ������ ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CurrentCameraLoc = CurrentArmOriginLoc - CurrentArmRotator.Vector() * ArmLength;

	// PlaneMode�� ī�޶� �浹ó���� ���� �ʴ´�.

	// ���� �̵��� ���
	CalMovementAxis();

	// ����� ���� ���
	if (bDrawDebug)
		DrawDebug();

	// ���� ī�޶� ���� Ʈ������ ó��
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// ������ Ʈ�������� ���� ������Ʈ�� ���� Ʈ���������� ��ȯ
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// ������ ���� Ʈ�������� ���� ī�޶� ��ġ(����) ������ �̿�
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// �Ʒ� �Լ��� ȣ���ϸ� GetSocketTransform �Լ��� ȣ��ǰ� �ش� �Լ����� ī�޶� ������ ���� ��ġ�� ��ȯ
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::UpdateCylinderMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime)
{
	// CylinderMode�� ȸ�� ������ ������ ��ġ�� ������� ī�޶� ���������� ���¸�� (�����谣�� �����ϸ� ����.)
	// Z�� ������ �Ǹ��� ��常 ����
	FVector ActorLoc = GetOwner()->GetActorLocation();

	FVector ArrangedCylinderOrigin = CylinderOrigin;
	ArrangedCylinderOrigin.Z = ActorLoc.Z;

	// ī�޶� ���� ��ǥ ȸ����. ���ĵ� �Ǹ��� ����(������ Z��� ���̸� ���� �Ǹ��� ����) -> ������ �������� ī�޶���� ȸ����Ų��.
	TargetArmRotator = (ArrangedCylinderOrigin - ActorLoc).Rotation();

	// ī�޶��� ��ǥ ȸ����. ī�޶� ���� ��ǥ ȸ������ ����
	TargetCameraRotator = TargetArmRotator;

	// ī�޶���� ȸ���߽� ��ǥ��ġ
	TargetArmOriginLoc = GetComponentLocation();

	CameraTargetLoc = ActorLoc - (TargetArmRotator.Vector() * TargetArmLength);

	// ����(ī�޶�)ó�� ������ ������� �ʴ´ٸ� �ٷ� ��ǥ��ġ�� ����Ѵ�. 
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

	// ������ ī�޶��� ���� ��ǥ��ġ ��� -> ��ǥ ȸ������ �߽����� ȸ���Ѹ�ŭ�� ��ġ
	CurrentCameraLoc = CurrentArmOriginLoc - (CurrentArmRotator.Vector() * TargetArmLength);

	// �Ǹ������� ī�޶� �浹ó���� ���� �ʴ´�.

	// ���� �̵��� ���
	CalMovementAxis();

	// ����� ���� ���
	if (bDrawDebug)
		DrawDebug();

	// ���� ī�޶� ���� Ʈ������ ó��
	FTransform WorldCamTM(CurrentCameraRotator, CurrentCameraLoc);

	// ������ Ʈ�������� ���� ������Ʈ�� ���� Ʈ���������� ��ȯ
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// ������ ���� Ʈ�������� ���� ī�޶� ��ġ(����) ������ �̿�
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	// �Ʒ� �Լ��� ȣ���ϸ� GetSocketTransform �Լ��� ȣ��ǰ� �ش� �Լ����� ī�޶� ������ ���� ��ġ�� ��ȯ
	UpdateChildTransforms();
}

void USSH_CameraControlComponent::CalMovementAxis()
{
	// �̵��� ����� ī�޶� ���� ����->�̵� ����(MoveUp-Down)
	// ī�޶� ���������� -> ���� �̵�(MoveLeft-Right)

	// ī�޶��� ����ȸ�� Ÿ���� �޾ƿ��� �̵��� ȸ�� ������ ���ؼ� ó��
	FRotator MovementAxisRotator = TargetCameraRotator + MovementAxisRotate;

	// ���溤�� ȸ���� �ش� ���� �յ� �̵� �������� ���.
	MovementFowardDir = MovementAxisRotator.RotateVector(FVector::ForwardVector);

	// z���̵��� ���ó��(���Ͱ� �ϴ÷�??)
	if (false == AllowZAxisMovement)
		MovementFowardDir.Z = 0.0f;

	// ���� ����� �̵��� ����ȭ
	MovementFowardDir.Normalize();


	// �����̵�ó���� ����
	MovementRightDir = MovementAxisRotator.RotateVector(FVector::RightVector);

	if (false == AllowZAxisMovement)
		MovementRightDir.Z = 0.0f;

	MovementRightDir.Normalize();
}

void USSH_CameraControlComponent::DrawDebug()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// ī�޶���� ȸ���߽� ��ǥ��ġ ǥ��
	DrawDebugSphere(GetWorld(), TargetArmOriginLoc, 16.0f, 8, FColor::Red);

	// ī�޶��� ���� ��ǥ��ġ
	DrawDebugDirectionalArrow(GetWorld(), TargetArmOriginLoc, CameraTargetLoc, 14.0f, FColor::Red);

	// ī�޶��� ��ǥ ȸ����
	DrawDebugDirectionalArrow(GetWorld(), CameraTargetLoc, (CameraTargetLoc + TargetCameraRotator.Vector() * 50.0f), 14.0f, FColor::Blue);

	// �������� ī�޶���� ȸ���߽� ��ǥ��ġ ǥ��
	DrawDebugSphere(GetWorld(), CurrentArmOriginLoc, 16.0f, 8, FColor::Yellow);

	// �������� ī�޶��� ���� ��ǥ��ġ
	DrawDebugDirectionalArrow(GetWorld(), CurrentArmOriginLoc, CurrentCameraLoc, 14.0f, FColor::Yellow);

	// �������� ī�޶��� ��ǥ ȸ����
	DrawDebugDirectionalArrow(GetWorld(), CurrentCameraLoc, (CurrentCameraLoc + CurrentCameraRotator.Vector() * 50.0f), 14.0f, FColor::Cyan);

	// �̵� �� ����� ��ο�
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

	// ��Ʈ�ѷ� ȸ���� ��Ÿ�� ����
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