// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GrappleHookComponent.generated.h"


UENUM()
enum class EGrappleState : uint8
{
	Inactive,
	Throwing,
	Releasing,
	Ziplining,
	Swinging
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GD73MEGAJAM2025_API UGrappleHookComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	// 
	// ===== CONSTRUCTOR & OVERRIDES ===== //
	//

	UGrappleHookComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay()  override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	//
	// ===== FUNCTIONS ===== //
	//


	// ===== Server ===== //

	UFUNCTION(BlueprintAuthorityOnly)
	void ApplyZiplineForce();

	UFUNCTION(BlueprintAuthorityOnly)
	void ApplySwingForce();

	UFUNCTION(BlueprintAuthorityOnly)
	void ThrowGrapple();

	UFUNCTION(BlueprintAuthorityOnly)
	void ReleaseGrapple();

	UFUNCTION(BlueprintAuthorityOnly)
	void GrappleJump();

	UFUNCTION(BlueprintAuthorityOnly)
	void GetGrappleTarget();

	UFUNCTION(BlueprintAuthorityOnly)
	void UpdateTimers(float DeltaTime);

	// ===== Remote Procedure Calls ===== //

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SendGrappleInput(const FVector& AimDirection, bool bInput);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestGrappleJump();


	// ===== Replication Notify Callbacks ===== //


	//
	// ===== COMPONENTS ===== //
	//

	UPROPERTY(EditAnywhere, Category = "GrappleHook|Components", meta = (AllowedClasses = "CapsuleComponent", UseComponentPicker))
	FComponentReference CapsuleRef;

	UPROPERTY(EditAnywhere, Category = "GrappleHook|Components", meta = (AllowedClasses = "CustomPawnMovementComponent", UseComponentPicker))
	FComponentReference CustomMovementComponentRef;

	UPROPERTY(EditAnywhere, Category = "GrappleHook|Components", meta = (AllowedClasses = "SceneComponent", UseComponentPicker))
	FComponentReference GrappleAttachSceneComponentRef;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GrappleHook|Components")
	//class UCableComponent* Cable;


	// 
	// ===== TUNABLES ===== //
	//


	// ===== Throw Parameters ===== //

	// The max range the sphere trace will do to look for grapple targets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Throwing")
	float TraceRange  = 1000.f;

	// The radius of the sphere used for the trace, bigger number means easier time collecting targets for sorting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Throwing")
	float TraceRadius = 500.f;

	// Maximum Dot prodruct between a grapple point and the center of the screen (MUST BE BETWEEN 1 AND -1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Throwing")
	float TargetMaxDot = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Throwing")
	float GrappleFlightTime = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Throwing")
	UCurveFloat* GrappleFlightCurve;


	// ===== Zipline Parameters ===== //

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Zipline")
	float ZiplineControl  = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Zipline")
	float ZiplineStrength = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Zipline")
	float ZiplineDamping  = 1.f;


	// ===== Swing Parameters ===== //

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	float SwingControl = .2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	float SwingDamping = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	float SwingSpringStrength = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	float SwingSpringDamping  = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	float SwingRotationSpringStrength = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	float SwingRotationSpringDamping  = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|Swinging")
	bool bControlsRotation = true;


	//
	// ===== STATE ===== //
	//


	// ===== Replicated ===== //

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	TArray<FHitResult> TraceHits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	FVector TraceDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	bool bIsInputDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	EGrappleState GrappleState = EGrappleState::Inactive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	UPrimitiveComponent* CurrentGrappleTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	float CurrentGrappleDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	FVector ToGrappleTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State", Replicated)
	float GrappleFlightTimer = 0.f;


	// ===== Not Replicated ===== //

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State")
	class UCapsuleComponent* Capsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State")
	class UCustomPawnMovementComponent* CustomMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GrappleHook|State")
	class USceneComponent* GrappleAttachSceneComponent;

};

