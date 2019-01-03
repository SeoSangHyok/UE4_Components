// Fill out your copyright notice in the Description page of Project Settings.

#include "SSH_CollisionProfileHelper.h"
#include "Runtime/Engine/Classes/Engine/CollisionProfile.h"

ETraceTypeQuery USSH_CollisionProfileHelper::GetTraceQueryByTraceName(FName TraceChannelName)
{
	const UCollisionProfile* CollisionProfile = UCollisionProfile::Get();
	if (!IsValid(CollisionProfile))
		return ETraceTypeQuery::TraceTypeQuery_MAX;

	for (int i = 0; i<32; i++)
	{
		ECollisionChannel TraceCollisionChannel = CollisionProfile->ConvertToCollisionChannel(true, i);
		if (ECC_MAX != TraceCollisionChannel)
		{
			FName CheckTraceTypeName = CollisionProfile->ReturnChannelNameFromContainerIndex(TraceCollisionChannel);
			if (TraceChannelName == CheckTraceTypeName)
			{
				return CollisionProfile->ConvertToTraceType(TraceCollisionChannel);
			}
		}
	}

// 
// 
// 	ECollisionChannel CollisionChannel;
// 	FCollisionResponseParams CollisionResponseParams;
// 
// 	if (CollisionProfile->GetChannelAndResponseParams(TraceChannelName, CollisionChannel, CollisionResponseParams))
// 	{
// 		return CollisionProfile->ConvertToTraceType(CollisionChannel);
// 
// 	}

	return ETraceTypeQuery::TraceTypeQuery_MAX;
}

EObjectTypeQuery USSH_CollisionProfileHelper::GetObjectQueryByObjectName(FName ObjectChannelName)
{
	const UCollisionProfile* CollisionProfile = UCollisionProfile::Get();
	if (!IsValid(CollisionProfile))
		return EObjectTypeQuery::ObjectTypeQuery_MAX;

	for (int i = 0; i<32; i++)
	{
		ECollisionChannel ObjectCollisionChannel = CollisionProfile->ConvertToCollisionChannel(false, i);
		if (ECC_MAX != ObjectCollisionChannel)
		{
			FName CheckObjectTypeName = CollisionProfile->ReturnChannelNameFromContainerIndex(ObjectCollisionChannel);
			if (ObjectChannelName == CheckObjectTypeName)
			{
				return CollisionProfile->ConvertToObjectType(ObjectCollisionChannel);
			}
		}
	}

// 	ECollisionChannel CollisionChannel;
// 	FCollisionResponseParams CollisionResponseParams;
// 
// 	if (CollisionProfile->GetChannelAndResponseParams(TraceChannelName, CollisionChannel, CollisionResponseParams))
// 	{
// 		return CollisionProfile->ConvertToObjectType(CollisionChannel);
// 	}

	return EObjectTypeQuery::ObjectTypeQuery_MAX;
}