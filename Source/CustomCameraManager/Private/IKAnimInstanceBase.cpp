// Fill out your copyright notice in the Description page of Project Settings.

#include "IKAnimInstanceBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"


void UIKAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	APawn* PawnOwner = TryGetPawnOwner();

	if (!IsValid(PawnOwner))
		return;

	UPawnMovementComponent* MovementComponent = PawnOwner->GetMovementComponent();
	if (IsValid(MovementComponent))
	{
		IsInAir = MovementComponent->IsFalling();
	}

	Speed = PawnOwner->GetVelocity().Size();
}