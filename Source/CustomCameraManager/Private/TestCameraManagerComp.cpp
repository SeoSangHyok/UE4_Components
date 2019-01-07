// Fill out your copyright notice in the Description page of Project Settings.

#include "TestCameraManagerComp.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

UTestCameraManagerComp::UTestCameraManagerComp()
{
	CameraArmType = ECameraAramType::Follow;
}

void UTestCameraManagerComp::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateDesiredArmLocation(bDoCollisionTest, bEnableCameraLag, bEnableCameraRotationLag, DeltaTime);
}

void UTestCameraManagerComp::SetTargetActor(AActor* InActor)
{
	TargetActor = InActor;
}

FRotator UTestCameraManagerComp::CalComponentInputRotationDelta()
{
	FRotator RetVal = FRotator::ZeroRotator;

	FRotator CurrentInputRot = GetComponentRotation();
	if (bUsePawnControlRotation)
	{
		if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			const FRotator PawnViewRotation = OwningPawn->GetViewRotation();
			if (CurrentInputRot != PawnViewRotation)
			{
				CurrentInputRot = PawnViewRotation;
			}
		}
	}

	// If inheriting rotation, check options for which components to inherit
	if (!bAbsoluteRotation)
	{
		if (!bInheritPitch)
		{
			CurrentInputRot.Pitch = RelativeRotation.Pitch;
		}

		if (!bInheritYaw)
		{
			CurrentInputRot.Yaw = RelativeRotation.Yaw;
		}

		if (!bInheritRoll)
		{
			CurrentInputRot.Roll = RelativeRotation.Roll;
		}
	}

	RetVal = CurrentInputRot - PrevInputRotation;
	PrevInputRotation = CurrentInputRot;

	return RetVal;
}

void UTestCameraManagerComp::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	switch (CameraArmType)
	{
	case ECameraAramType::Default:
		UpdateDefaultMode(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
		break;

	case ECameraAramType::Follow:
		UpdateDefaultMode(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
		if (IsValid(TargetActor))
		{
			UpdateFollowMode(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
		}
		else
		{
			UpdateDefaultMode(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
		}
		break;

	case ECameraAramType::Fix:
		UpdateFixMode(DeltaTime);
		break;
	default:
		break;
	}
}

void UTestCameraManagerComp::UpdateDefaultMode(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	FRotator RotationDelta = CalComponentInputRotationDelta();
	DesiredRot += RotationDelta;

	const float InverseCameraLagMaxTimeStep = (1.f / CameraLagMaxTimeStep);

	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	FVector DesiredLoc = ArmOrigin;

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLoc == DesiredLoc)
		{
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	FTransform WorldCamTM(DesiredRot, ResultLoc);
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
}


void UTestCameraManagerComp::UpdateFollowMode(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	FVector LookDir = TargetActor->GetActorLocation() - GetOwner()->GetActorLocation();
	LookDir.Normalize();

	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	FVector DesiredLoc = ArmOrigin;

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	DesiredLoc -= LookDir * TargetArmLength;
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLoc == DesiredLoc)
		{
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	FTransform WorldCamTM(DesiredRot, ResultLoc);
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();

	TArray<USceneComponent*> ChildrenComponents;
	GetChildrenComponents(false, ChildrenComponents);
	if (IsValid(TargetActor))
	{
		UCameraComponent* CameraComp = nullptr;
		for (auto& SceneComp : ChildrenComponents)
		{
			CameraComp = Cast<UCameraComponent>(SceneComp);
			if (IsValid(CameraComp))
			{
				break;
			}
		}

		if (IsValid(CameraComp))
		{
			if (IsValid(TargetActor))
			{
				FVector DesireLookDirection = TargetActor->GetActorLocation() - CameraComp->GetComponentLocation();
				DesireLookDirection.Normalize();

				CameraComp->SetWorldRotation(DesireLookDirection.Rotation());
			}
			else
			{
				CameraComp->SetRelativeRotation(FRotator::ZeroRotator);
			}
		}
	}

	return;
}

void UTestCameraManagerComp::UpdateFixMode(float DeltaTime)
{
}