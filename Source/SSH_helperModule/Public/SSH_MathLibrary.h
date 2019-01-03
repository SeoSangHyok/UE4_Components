// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "SSH_MathLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SSH_HELPERMODULE_API USSH_MathLibrary : public UKismetMathLibrary
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintCallable, Category = SSH_Math)
	static FRotator SSH_RotatorInerp(FRotator& TargetRotator, FRotator& CurrentRotator, float DeltaTime, float InterpSpeed = 0.0f, float MaxSubsteppingTime = 0.0f);

	UFUNCTION(BlueprintCallable, Category = SSH_Math)
	static FVector SSH_VectorInerp(FVector& TargetVector, FVector& CurrentVector, float DeltaTime, float InterpSpeed = 0.0f, float MaxSubsteppingTime = 0.0f);
};
