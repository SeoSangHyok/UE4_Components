// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SSH_CollisionProfileHelper.generated.h"

/**
 * 
 */
UCLASS()
class SSH_HELPERMODULE_API USSH_CollisionProfileHelper : public UObject
{
	GENERATED_BODY()


public:
	static ETraceTypeQuery GetTraceQueryByTraceName(FName TraceChannelName);
	static EObjectTypeQuery GetObjectQueryByObjectName(FName TraceChannelName);
};
