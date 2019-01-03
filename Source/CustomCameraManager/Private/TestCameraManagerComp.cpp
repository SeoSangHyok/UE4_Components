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

/*
	// Apply 'lag' to rotation if desired
	if (bDoRotationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
		{
			const FRotator ArmRotStep = (DesiredRot - PreviousDesiredRot).GetNormalized() * (CameraLagMaxTimeStep / DeltaTime);
			FRotator LerpTarget = PreviousDesiredRot;
			float RemainingTime = DeltaTime;
			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmRotStep * (LerpAmount * InverseCameraLagMaxTimeStep);
				RemainingTime -= LerpAmount;

				DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
				PreviousDesiredRot = DesiredRot;
			}
		}
		else
		{
			DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(DesiredRot), DeltaTime, CameraRotationLagSpeed));
		}
	}
	PreviousDesiredRot = DesiredRot;
*/

	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actual camera position, so rotating the camera around does not have lag
	FVector DesiredLoc = ArmOrigin;
/*
	if (bDoLocationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraLagSpeed > 0.f)
		{
			const FVector ArmMovementStep = (ArmOrigin - PreviousArmOrigin) * (CameraLagMaxTimeStep / DeltaTime);
			FVector LerpTarget = PreviousArmOrigin;
			float RemainingTime = DeltaTime;
			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmMovementStep * (LerpAmount * InverseCameraLagMaxTimeStep);
				RemainingTime -= LerpAmount;

				DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, LerpTarget, LerpAmount, CameraLagSpeed);
				PreviousDesiredLoc = DesiredLoc;
			}
		}
		else
		{
			DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, DesiredLoc, DeltaTime, CameraLagSpeed);
		}

		// Clamp distance if requested
		bool bClampedDist = false;
		if (CameraLagMaxDistance > 0.f)
		{
			const FVector FromOrigin = DesiredLoc - ArmOrigin;
			if (FromOrigin.SizeSquared() > FMath::Square(CameraLagMaxDistance))
			{
				DesiredLoc = ArmOrigin + FromOrigin.GetClampedToMaxSize(CameraLagMaxDistance);
				bClampedDist = true;
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugLagMarkers)
		{
			DrawDebugSphere(GetWorld(), ArmOrigin, 5.f, 8, FColor::Green);
			DrawDebugSphere(GetWorld(), DesiredLoc, 5.f, 8, FColor::Yellow);

			const FVector ToOrigin = ArmOrigin - DesiredLoc;
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc, DesiredLoc + ToOrigin * 0.5f, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
			DrawDebugDirectionalArrow(GetWorld(), DesiredLoc + ToOrigin * 0.5f, ArmOrigin, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
		}
#endif
	}
*/
	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
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

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRot, ResultLoc);
	// Convert to relative to component
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


	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actual camera position, so rotating the camera around does not have lag
	FVector DesiredLoc = ArmOrigin;

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	// Now offset camera position back along our rotation
	DesiredLoc -= LookDir * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
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

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRot, ResultLoc);
	// Convert to relative to component
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