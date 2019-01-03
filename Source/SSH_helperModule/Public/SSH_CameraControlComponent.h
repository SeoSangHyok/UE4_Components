// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SSH_CameraControlComponent.generated.h"

UENUM(BlueprintType)
enum class ESSH_CameraControlMode : uint8
{
	Normal,
	Targeting,
	Fix,
	Plane,
	Cylinder,
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SSH_HELPERMODULE_API USSH_CameraControlComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USSH_CameraControlComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void UpdateNormalMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime);
	void UpdateTargetMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime);
	void UpdateFixMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime);
	void UpdatePlaneMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime);
	void UpdateCylinderMode(bool DoCollisionTest, bool DoCameraLag, float DeltaTime);
	
	void CalMovementAxis();
	void DrawDebug();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;

	// USceneComponent interface
	virtual bool HasAnySockets() const override;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const override;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const override;
	// End of USceneComponent interface

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	void SetControlMode(ESSH_CameraControlMode InControlMode) { ControllMode = InControlMode; }

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	void SetTargetActor(AActor* Actor) { TargetActor = Actor; }

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	void SetCameraFixPos(FVector Pos) { FixPosition = Pos; }

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	void SetCameraPlane(FVector PlanePos, FVector PlaneNormal) { CameraPlane = FPlane(PlanePos, PlaneNormal); }

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	void SetCameraCylinderOrigin(FVector Pos) { CylinderOrigin = Pos; }

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	FVector GetCaracterMovementDir(FVector2D InputAxis);

	UFUNCTION(BlueprintCallable, Category = "SSH_CameraControl")
	void SetMovementAxisRotateOffset(FRotator RotateOffset) { MovementAxisRotate = RotateOffset; }
	
private:
	/** The name of the socket at the end of the spring arm (looking back towards the spring arm origin) */
	static const FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common", meta = (AllowPrivateAccess = "true"))
	ESSH_CameraControlMode ControllMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common", meta = (AllowPrivateAccess = "true"))
	float TargetArmLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FRotator TargetArmRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FRotator CurrentArmRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FRotator TargetCameraRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FRotator CurrentCameraRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FVector TargetArmOriginLoc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FVector CurrentArmOriginLoc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FVector CameraTargetLoc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common", meta = (AllowPrivateAccess = "true"))
	FVector CurrentCameraLoc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Movement", meta = (AllowPrivateAccess = "true"))
	FRotator MovementAxisRotate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common|Movement", meta = (AllowPrivateAccess = "true"))
	FVector MovementFowardDir;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common|Movement", meta = (AllowPrivateAccess = "true"))
	FVector MovementRightDir;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Common|Movement", meta = (AllowPrivateAccess = "true"))
	bool AllowZAxisMovement;

	/** If true, do a collision test using ProbeChannel and ProbeSize to prevent camera clipping into level.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common", meta = (AllowPrivateAccess = "true"))
	uint32 bDrawDebug : 1;

	/** If true, do a collision test using ProbeChannel and ProbeSize to prevent camera clipping into level.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Collision", meta = (AllowPrivateAccess = "true"))
	uint32 bDoCollisionTest : 1;

	/** How big should the query probe sphere be (in unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Collision", meta = (editcondition = "bDoCollisionTest", AllowPrivateAccess = "true"))
	float ProbeSize;

	/** Collision channel of the query probe (defaults to ECC_Camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Collision", meta = (editcondition = "bDoCollisionTest", AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> ProbeChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Interp", meta = (AllowPrivateAccess = "true"))
	uint32 bEnableCameraLag : 1;

	/** Max time step used when sub-stepping camera lag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Interp", meta = (editcondition = "bEnableCameraLag", ClampMin = "0.005", ClampMax = "0.5", UIMin = "0.005", UIMax = "0.5", AllowPrivateAccess = "true"))
	float CameraLagMaxTimeStep;

	/** If bEnableCameraRotationLag is true, controls how quickly camera reaches target position. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common|Interp", meta = (editcondition = "bEnableCameraLag", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0", AllowPrivateAccess = "true"))
	float CameraLagSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TargetMode", meta = (AllowPrivateAccess = "true"))
	AActor* TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FixMode", meta = (AllowPrivateAccess = "true"))
	FVector FixPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaneMode", meta = (AllowPrivateAccess = "true"))
	FPlane CameraPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (AllowPrivateAccess = "true"))
	FVector CylinderOrigin;

	UPROPERTY()
	FRotator PrevControllerRatation;

	UPROPERTY()
	FRotator ControlRotatorDelta;

	/** Cached component-space socket location */
	FVector RelativeSocketLocation;
	/** Cached component-space socket rotation */
	FQuat RelativeSocketRotation;
};
