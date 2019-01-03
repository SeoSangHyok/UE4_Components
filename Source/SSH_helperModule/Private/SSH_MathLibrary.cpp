// Fill out your copyright notice in the Description page of Project Settings.

#include "SSH_MathLibrary.h"

FRotator USSH_MathLibrary::SSH_RotatorInerp(FRotator& TargetRotator, FRotator& CurrentRotator, float DeltaTime, float InterpSpeed /*= 0.0f*/, float MaxSubsteppingTime /*= 0.0f*/)
{
	FRotator DesiredRot = TargetRotator;

	if (MaxSubsteppingTime > 0.0f && DeltaTime > MaxSubsteppingTime && InterpSpeed > 0.f)
	{
		const float InverseCameraLagMaxTimeStep = (1.f / MaxSubsteppingTime);
		const FRotator RotStep = (TargetRotator - CurrentRotator).GetNormalized() * (MaxSubsteppingTime / DeltaTime);
		FRotator LerpTarget = CurrentRotator;
		FRotator PrevDesireRot = CurrentRotator;
		float RemainDeltaTime = DeltaTime;

		FRotator DesiredRot;
		while (RemainDeltaTime > KINDA_SMALL_NUMBER)
		{
			float LerpAmount = FMath::Min(MaxSubsteppingTime, RemainDeltaTime);
			LerpTarget += RotStep * (LerpAmount * InverseCameraLagMaxTimeStep);

			DesiredRot = FRotator(FMath::QInterpTo(FQuat(PrevDesireRot), FQuat(LerpTarget), LerpAmount, InterpSpeed));
			PrevDesireRot = DesiredRot;

			RemainDeltaTime -= LerpAmount;
		}
	}
	else
	{
		DesiredRot = FRotator(FMath::QInterpTo(FQuat(CurrentRotator), FQuat(TargetRotator), DeltaTime, InterpSpeed));
	}

	return DesiredRot;
}

FVector USSH_MathLibrary::SSH_VectorInerp(FVector& TargetVector, FVector& CurrentVector, float DeltaTime, float InterpSpeed /*= 0.0f*/, float MaxSubsteppingTime /*= 0.0f*/)
{
	FVector DesiredVector = TargetVector;

	if (MaxSubsteppingTime > 0.0f && DeltaTime > MaxSubsteppingTime && InterpSpeed > 0.f)
	{
		const float InverseCameraLagMaxTimeStep = (1.f / MaxSubsteppingTime);
		const FVector LerpStep = (TargetVector - CurrentVector) * (MaxSubsteppingTime / DeltaTime);
		FVector LerpTarget = CurrentVector;
		FVector PrevDesireVector = CurrentVector;
		float RemainDeltaTime = DeltaTime;

		FRotator DesiredRot;
		while (RemainDeltaTime > KINDA_SMALL_NUMBER)
		{
			float LerpAmount = FMath::Min(MaxSubsteppingTime, RemainDeltaTime);
			LerpTarget += LerpStep * (LerpAmount * InverseCameraLagMaxTimeStep);

			DesiredVector = FMath::VInterpTo(PrevDesireVector, LerpTarget, LerpAmount, InterpSpeed);
			PrevDesireVector = DesiredVector;

			RemainDeltaTime -= LerpAmount;
		}
	}
	else
	{
		DesiredVector = FMath::VInterpTo(CurrentVector, TargetVector, DeltaTime, InterpSpeed);
	}

	return DesiredVector;
}