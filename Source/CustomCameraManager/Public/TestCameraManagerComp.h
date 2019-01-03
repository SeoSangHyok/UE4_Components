// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "TestCameraManagerComp.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECameraAramType : uint8
{
	Default,
	Follow,
	Fix
};

UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent))
class CUSTOMCAMERAMANAGER_API UTestCameraManagerComp : public USpringArmComponent
{
	GENERATED_BODY()


public:
	UTestCameraManagerComp();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetTargetActor(AActor* InActor);

	FRotator CalComponentInputRotationDelta();

protected:
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime);

private:
	void UpdateDefaultMode(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime);
	void UpdateFollowMode(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime);
	void UpdateFixMode(float DeltaTime);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	ECameraAramType CameraArmType;

	UPROPERTY(VisibleAnywhere)
	AActor* TargetActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName TargetActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator DesiredRot;


	FRotator PrevInputRotation;
};
