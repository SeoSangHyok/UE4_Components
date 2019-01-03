// Fill out your copyright notice in the Description page of Project Settings.

#include "SSH_IKComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SSH_CollisionProfileHelper.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
USSH_IKComponent::USSH_IKComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


// Called when the game starts
void USSH_IKComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USSH_IKComponent::SetTargetSkeletalMeshComp(USkeletalMeshComponent* SkeletalMeshComp)
{
	TargetSkeletaMeshComp = SkeletalMeshComp;
}

void USSH_IKComponent::AddIKTarget(FName BoneName, FName SocketName)
{
	for (auto& FootIKInfo : FootIKInfoList)
	{
		if (FootIKInfo.IKTargetBoneName == BoneName)
		{
			FootIKInfo.IKTargetSocketName = SocketName;
			return;
		}
	}

	FSSH_FootIKInfo NewFootIKInfo;
	NewFootIKInfo.IKTargetBoneName = BoneName;
	NewFootIKInfo.IKTargetSocketName = SocketName;

	FootIKInfoList.Add(NewFootIKInfo);
}

void USSH_IKComponent::RemoveIKTarget(FName BoneName)
{
	for (int i=0 ; i<FootIKInfoList.Num() ; i++)
	{
		if (FootIKInfoList[i].IKTargetBoneName == BoneName)
		{
			FootIKInfoList.RemoveAt(i);
			return;
		}
	}
}

FSSH_FootIKInfo USSH_IKComponent::GetIkResult(FName BoneName)
{
	FSSH_FootIKInfo RetVal;

	for (auto& FootIKInfo : FootIKInfoList)
	{
		if (FootIKInfo.IKTargetBoneName == BoneName)
		{
			RetVal = FootIKInfo;
			break;
		}
	}

	return RetVal;
}

// Called every frame
void USSH_IKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsActive)
	{
		UpdateIk();
	}
}

void USSH_IKComponent::SetActive(bool bNewActive, bool bReset/* = false*/)
{
	if (bIsActive == bNewActive)
		return;

	Super::SetActive(bNewActive, bReset);

	if (false == bNewActive)
		ResetVal();
}

void USSH_IKComponent::UpdateIk()
{
	HipOffset = 0.0f;

	for (auto& FootIKInfo : FootIKInfoList)
	{
		IKTrace(FootIKInfo);
	}

	AdjustIkFootOffset();

	BroadCastIkResult();
}

bool USSH_IKComponent::IKTrace(FSSH_FootIKInfo& FootIKInfo)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(TargetSkeletaMeshComp))
		return false;


	FootIKInfo.FootOffset = 0.0f;
	FootIKInfo.ImpactRotate = FRotator::ZeroRotator;

	FVector BoneOrSocketLoc;
	USkeletalMeshSocket const* IKSocket = TargetSkeletaMeshComp->GetSocketByName(FootIKInfo.IKTargetSocketName);
	if (nullptr == IKSocket)
		BoneOrSocketLoc = TargetSkeletaMeshComp->GetBoneLocation(FootIKInfo.IKTargetBoneName);
	else
		BoneOrSocketLoc = IKSocket->GetSocketLocation(TargetSkeletaMeshComp);

	FVector ActorUpVector = OwnerActor->GetActorUpVector();
	FVector ActorDownVector = -1 * ActorUpVector;

	// FootIk�� ���ͱ��� �ٴ����� ���ϴ� ���ͷ� ó���ȴ�.
	// ���͹ٴ��� ������ ó���� ���� ������ġ�� �������� ���� ��/���� �����̼��� �������� ���ؼ� �ش� ������ �������� Ʈ���̽� ���������� ��´�.
	FVector TraceStart = FMath::LinePlaneIntersection(BoneOrSocketLoc, (BoneOrSocketLoc + ActorUpVector * 1000.0f), OwnerActor->GetActorLocation(), ActorUpVector);
	FVector TraceEnd = TraceStart +  ActorDownVector*(TraceDist + ActorHalfHeight);

	// ���⼭�� ���ǻ� visibility Ʈ���̽����� ó�������� ���� Ȯ���ؼ� Ik���� Ʈ���̽� ���������� ����°͵� ������ ����.
	ETraceTypeQuery TraceTypeQuery = USSH_CollisionProfileHelper::GetTraceQueryByTraceName(FName("Visibility"));
	TArray<AActor*> TraceIgnore = { OwnerActor };
	FHitResult HitResult;

	// ���̷�Ż �޽��� ��Ĺ�� �������� �ٴڿ� �����ɽ��� �׽�Ʈ
	bool TraceSuccess = UKismetSystemLibrary::LineTraceSingle(
		OwnerActor->GetWorld(),
		TraceStart,
		TraceEnd,
		TraceTypeQuery,
		true,
		TraceIgnore,
		(DrawDebugTrace == true ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None),
		HitResult,
		true
	);

	if (TraceSuccess)
	{
		if (HitResult.bBlockingHit)
		{
			FootIKInfo.FootOffset = (HitResult.Location - TraceEnd).Size() - TraceDist + AdjustOffset;
			FootIKInfo.ImpactRotate = ImpactNormalToRotator(HitResult.ImpactNormal, ActorUpVector);
		}
	}

	// Ʈ���̽� ����Ʈ�� �ɸ��� �ٴ�(ĸ���ٴ�)���� �Ʒ��� ��ġ�Ѱ�� hipOffset�� ������ IK�������� ����ش�.
	// HipOffset�� ���� �Ʒ��� Ʈ���̽� ����Ʈ������ ó���Ѵ�.
	HipOffset = FMath::Min3<float>(HipOffset, FootIKInfo.FootOffset, 0.0f);

	return true;
}

FRotator USSH_IKComponent::ImpactNormalToRotator(FVector ImpactNormal, FVector ActorUpVector)
{
	// �浹 ��ְ����� �߸��� ȸ���� ���ϴ°� �����Ͱ� (0,0,1)�ΰ��� �� ���������� ���Ͱ� ȸ���ؼ� UPVector�� �޶����� ��� ���������� �۵����� �ʰ� �ִ�. �ش�κ��� �ذ��ؾ��� ����..
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
		return FRotator::ZeroRotator;

	FRotator RetVal;
  	RetVal.Roll = UKismetMathLibrary::DegAtan2(ImpactNormal.Y, ImpactNormal.Z) - OwnerActor->GetActorRotation().Roll;
 	RetVal.Pitch = UKismetMathLibrary::MakeRotFromZY(ImpactNormal, FVector::RightVector).Pitch;
 	RetVal.Yaw = 0.0f;

	RetVal += AdjustRotator;

	return RetVal;
}

void USSH_IKComponent::AdjustIkFootOffset()
{
	for (auto& FootIKInfo : FootIKInfoList)
	{
		FootIKInfo.FootOffset -= HipOffset;
	}
}

void USSH_IKComponent::BroadCastIkResult()
{
	if (IKUpdateNotify.IsBound())
	{
		IKUpdateNotify.Broadcast(FootIKInfoList, HipOffset);
	}
}

void USSH_IKComponent::ResetVal()
{
	for (auto& FootIKInfo : FootIKInfoList)
	{
		FootIKInfo.FootOffset = 0.0f;
		FootIKInfo.ImpactRotate = FRotator::ZeroRotator;
	}

	BroadCastIkResult();
}

void USSH_IKComponent::TestBlueprintNativeEventFunc_Implementation()
{

}