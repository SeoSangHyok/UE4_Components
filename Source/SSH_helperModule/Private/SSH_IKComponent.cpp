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

	// FootIk는 엑터기준 바닥으로 향하는 벡터로 처리된다.
	// 엑터바닥의 유동성 처리를 위해 엑터위치와 업백터의 평면과 본/소켓 로케이션의 교차점을 구해서 해당 평면과의 교차점을 트래이싱 시작점으로 잡는다.
	FVector TraceStart = FMath::LinePlaneIntersection(BoneOrSocketLoc, (BoneOrSocketLoc + ActorUpVector * 1000.0f), OwnerActor->GetActorLocation(), ActorUpVector);
	FVector TraceEnd = TraceStart +  ActorDownVector*(TraceDist + ActorHalfHeight);

	// 여기서는 편의상 visibility 트레이싱으로 처리했으나 좀더 확장해서 Ik전용 트레이싱 프로파일을 만드는것도 좋을거 같다.
	ETraceTypeQuery TraceTypeQuery = USSH_CollisionProfileHelper::GetTraceQueryByTraceName(FName("Visibility"));
	TArray<AActor*> TraceIgnore = { OwnerActor };
	FHitResult HitResult;

	// 스켈레탈 메시의 소캣을 기준으로 바닥에 레이케스팅 테스트
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

	// 트래이싱 포인트가 케릭터 바닥(캡슐바닥)보다 아래에 위치한경우 hipOffset을 내려서 IK포지션을 잡아준다.
	// HipOffset은 가장 아래쪽 트래이싱 포인트값으로 처리한다.
	HipOffset = FMath::Min3<float>(HipOffset, FootIKInfo.FootOffset, 0.0f);

	return true;
}

FRotator USSH_IKComponent::ImpactNormalToRotator(FVector ImpactNormal, FVector ActorUpVector)
{
	// 충돌 노멀값으로 발목의 회전을 구하는건 업벡터가 (0,0,1)인경우는 잘 동작하지만 엑터가 회전해서 UPVector가 달라지는 경우 정상적으로 작동하지 않고 있다. 해당부분은 해결해야할 문제..
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