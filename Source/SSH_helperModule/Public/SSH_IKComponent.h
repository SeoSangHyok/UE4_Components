// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SSH_IKComponent.generated.h"

USTRUCT(BlueprintType)
struct FSSH_FootIKInfo
{
public:
	GENERATED_USTRUCT_BODY()

	// Foot IK가 처리될 타겟 본
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName IKTargetBoneName;

	// Foot IK처리시 사용할 소켓, 없을경우 본을 이용한다.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName IKTargetSocketName;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float FootOffset;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FRotator ImpactRotate;

	FSSH_FootIKInfo()
	{
		IKTargetBoneName = FName("");
		IKTargetSocketName = FName("");
		FootOffset = 0.0f;
		ImpactRotate = FRotator::ZeroRotator;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSSH_IKUpdate, const TArray<FSSH_FootIKInfo>&, IKResults, float, HipOffset);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SSH_HELPERMODULE_API USSH_IKComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USSH_IKComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void SetActive(bool bNewActive, bool bReset = false);

	UFUNCTION(BlueprintCallable, Category = "SSH_HelperModule|IK")
	void SetTargetSkeletalMeshComp(class USkeletalMeshComponent* SkeletalMeshComp);

	UFUNCTION(BlueprintCallable, Category = "SSH_HelperModule|IK")
	void AddIKTarget(FName BoneName, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "SSH_HelperModule|IK")
	void RemoveIKTarget(FName BoneName);

	UFUNCTION(BlueprintCallable, Category = "SSH_HelperModule|IK")
	FSSH_FootIKInfo GetIkResult(FName BoneName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void TestBlueprintNativeEventFunc();
	void TestBlueprintNativeEventFunc_Implementation();

private:
	void UpdateIk();

	// IK 트래이싱 처리
	bool IKTrace(FSSH_FootIKInfo& FootIKInfo);

	FRotator ImpactNormalToRotator(FVector ImpactNormal, FVector ActorUpVector);

	// IKTrace결과로 나온 hipOffset 최종값을 구해진 FootOffset값에 보정처리
	void AdjustIkFootOffset();

	void BroadCastIkResult();

	void ResetVal();


public:
	UPROPERTY(BlueprintAssignable)
	FSSH_IKUpdate IKUpdateNotify;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	TArray<FSSH_FootIKInfo> FootIKInfoList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	class USkeletalMeshComponent* TargetSkeletaMeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	float ActorHalfHeight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	float HipOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	float TraceDist = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	float AdjustOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	bool DrawDebugTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SSH_HelperModule|IK", meta = (AllowPrivateAccess = true))
	FRotator AdjustRotator;
};
