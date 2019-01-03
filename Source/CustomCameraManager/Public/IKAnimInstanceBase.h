// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "IKAnimInstanceBase.generated.h"

/**
 * 
 */
UCLASS()
class CUSTOMCAMERAMANAGER_API UIKAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// Native update override point. It is usually a good idea to simply gather data in this step and 
	// for the bulk of the work to be done in NativeUpdateAnimation.
	virtual void NativeUpdateAnimation(float DeltaSeconds);


public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsInAir = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float HipOffset = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRotator LeftFootRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float LeftEffecOffset = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRotator RightFootRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RightEffectOffset = 0.0f;
};
