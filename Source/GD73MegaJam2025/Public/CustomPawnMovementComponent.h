// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "CustomPawnMovementComponent.generated.h"


UCLASS(meta = (BlueprintSpawnableComponent))
class GD73MEGAJAM2025_API UCustomPawnMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:

	// 
	// ===== CONSTRUCTOR & OVERRIDES ===== //
	//

	UCustomPawnMovementComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	//
	// ===== FUNCTIONS ===== //
	//


	// ===== Server ===== //

	// Spring Force that keeps the Pawn off the ground
	UFUNCTION(BlueprintAuthorityOnly)
	void ApplySpringForce  (float DeltaTime);

	// Movement Force applied to the Pawn on the X and Y axis
	UFUNCTION(BlueprintAuthorityOnly)
	void ApplyMovementForce(float DeltaTime);

	// Gyroscope and Yaw Rotation of Pawn
	UFUNCTION(BlueprintAuthorityOnly)
	void ApplyRotationForce(float DeltaTime);

	UFUNCTION(BlueprintAuthorityOnly)
	void Jump();

	UFUNCTION(BlueprintAuthorityOnly)
	void SetSlide(bool bShouldSlide);

	UFUNCTION(BlueprintAuthorityOnly)
	void UpdateTimers(float DeltaTime);

	UFUNCTION(BlueprintAuthorityOnly)
	bool ShouldSkipTick(float DeltaTime);

	UFUNCTION(BlueprintAuthorityOnly)
	void CleanInputs();


	// ===== Remote Procedure Calls ===== //

	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void Server_SendMoveInput(const FVector& MoveInput);

	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void Server_SendControlRotation(const FRotator& ControlRotation);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestJump();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestSlide(bool bShouldSlide);


	// ===== Replication Notify Callbacks ===== //

	UFUNCTION()
	void OnRep_bIsGrounded();

	UFUNCTION()
	void OnRep_bIsJumping();


	//
	// ===== COMPONENTS ===== //
	//

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CustomPhysics|Components")
	class UCapsuleComponent* Capsule;


	//
	// ===== TUNABLES ===== //
	//


	// ===== Spring Parameters ===== //

	// Strength of Gravity applied to the character in units per second squared (ACCELERATION IN CM/S^2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	float Gravity = 980.f;

	// Mass of the character used for Gravity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	float Mass = 1.5f;

	// Strength of the Spring Force applied to the character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	float SpringStrength = 150.f;

	// Damping factor for the spring to reduce oscillation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	float SpringDamping = 15.f;

	// Desired distance from the ground the spring will try to maintain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	float SpringRideHeight = 120.f;

	// Maximum distance the trace/ray can check for ground
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	float SpringLength = 150.f;

	// Direction the spring will trace to find the ground
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	FVector SpringTraceDirection = FVector(0.f, 0.f, -1.f);

	// Whether to trace relative to the character's rotation or world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Spring")
	bool bSpringTraceInWorldSpace = true;


	// ===== Locomotion Parameters ===== //

	// Maximum Ground speed of the character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float MaxGroundSpeed = 1500.f;

	// Maximum Air speed of the character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float MaxAirSpeed = 5000.f;

	// Acceleration rate applied when changing velocity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float Acceleration = 1250.f;

	// Maximum force allowed to be applied for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float MaxMovementForce = 200.f;

	// Acceleration multiplier when moving against current velocity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	UCurveFloat* TurnDotMultiplierCurve;

	// Multiplier applied to Ride Height when sliding (this should be less than 1 to make the pawn lower to the ground)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float SlideRideHeightMultiplier = .5f;

	// Multiplier applied to slide force from Gravity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float SlideGravityMultiplier = 1.f;

	// Multiplier applied to slide force from Input
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float SlideSteeringMultiplier = .25f;

	// Multiplier applied to slide force when initiating a slide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float SlideBoostMultiplier = 5.f;

	// Amount of damping applied to slide velocity (friction)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Locomotion")
	float SlideDamping = 1.f;


	// ===== Rotation Parameters ===== //

	// Speed at which the character rotates
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Rotation")
	float RotationSpringStrength = 250.f;

	// Damping factor for the yaw rotation to reduce oscillations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Rotation")
	float RotationSpringDamping = 15.f;

	// Speed at which the character rotates back to upright
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Rotation")
	float UprightSpringStrength = 500.f;

	// Damping factor for the upright rotation to reduce oscillations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Rotation")
	float UprightSpringDamping = 50.f;

	// Maximimum tilt amount applied to the character when moving fully against current velocity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Rotation")
	float MaxTiltDegrees = 45.f;


	// ===== Jumping Parameters ===== //

	// Force applied when the character jumps
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Jumping")
	float JumpForce = 1000.f;

	// Force Multiplier applied on the X and Y axis when the character jumps
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Jumping")
	float JumpHorizontalMultiplier = 1.2f;

	// Air control multiplier applied when the character is in the air
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Jumping")
	UCurveFloat* AirControlMultiplierCurve;

	// Coyote Time - Time allowed to jump after leaving the ground
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Jumping")
	float JumpCoyoteTime = 0.2f;

	// Jump Buffer Time - Time allowed to jump after pressing the jump button before landing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Jumping")
	float JumpBufferTime = 0.1f;

	// Jump Cooldown Time - Time before the next jump can be initiated
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomPhysics|Movement|Jumping")
	float JumpCooldownTime = 0.2f;


	//
	// ===== STATE ===== //
	//


	// ===== Replicated ===== //

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Movement", Replicated)
	FVector CapsuleVelocity;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Movement", Replicated)
	FVector GoalVelocity;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Movement", Replicated)
	bool bGroundDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Movement", Replicated)
	FHitResult LastGroundHitResult;

	// Controls whether the spring can apply force towards the ground, sometimes undesirable. E.G. We want to tstick to the ground, but we do not want to accelerate towards it after losing grounded status
	UPROPERTY(EditAnywhere,	   BlueprintReadWrite, Category = "CustomPhysics|State|Movement", ReplicatedUsing = OnRep_bIsGrounded)
	bool bIsGrounded = true;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Movement", Replicated)
	bool bCanMove = true;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Movement", Replicated)
	bool bIsSliding = false;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Movement", Replicated)
	bool bCanJump = true;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Movement", ReplicatedUsing = OnRep_bIsJumping)
	bool bIsJumping = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Movement", Replicated)
	float TimeSinceGrounded = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Movement", Replicated)
	float JumpCooldownTimer = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Input",    Replicated)
	FVector LastValidMovementInputVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,  Category = "CustomPhysics|State|Input",    Replicated)
	FVector MovementInputNormalized;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Capsule",  Replicated)
	FTransform CapsuleTransform;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Capsule",  Replicated)
	bool bIsUsingUprightSpring = true;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Capsule",  Replicated)
	FRotator CurrentBodyRotation;

	UPROPERTY(EditAnywhere,    BlueprintReadOnly,  Category = "CustomPhysics|State|Capsule",  Replicated)
	FVector CurrentBodyForwardVector;

	UPROPERTY(EditAnywhere,    BlueprintReadWrite, Category = "CustomPhysics|State|Debug",    Replicated)
	bool bEnableDebugDrawing = true;


	// ===== Authoratative inputs received from owning client ===== //

	UPROPERTY(Transient)
	FVector LastReceivedMoveInput = FVector::ZeroVector;

	UPROPERTY(Transient)
	FRotator LastReceivedControlRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	FVector2D LastReceivedCameraInput = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FRotator LastReceivedCameraRotation = FRotator::ZeroRotator;


protected:


};

