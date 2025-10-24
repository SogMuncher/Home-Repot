// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomPawnMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

//									   //
// ===== CONSTRUCTOR & OVERRIDES ===== //
//									   //

// Constructor
UCustomPawnMovementComponent::UCustomPawnMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UCustomPawnMovementComponent::OnRegister()
{
	Super::OnRegister();

	PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner) return;

	// Set up Capsule
	Capsule = PawnOwner->FindComponentByClass<UCapsuleComponent>();

	if (!Capsule)
	{
		Capsule = NewObject<UCapsuleComponent>(PawnOwner, UCapsuleComponent::StaticClass(), TEXT("CustomPawnCapsule"));
		Capsule->RegisterComponent();
		if (PawnOwner->GetRootComponent())
		{
			Capsule->SetupAttachment(PawnOwner->GetRootComponent());
		}
		else
		{
			PawnOwner->SetRootComponent(Capsule);
		}
	}

	//Capsule->SetCapsuleHalfHeight(45.f);
	//Capsule->SetCapsuleRadius(35.f);
	Capsule->SetSimulatePhysics(true);
	Capsule->SetEnableGravity(false);
	Capsule->SetIsReplicated(true);
}

void UCustomPawnMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (PawnOwner->HasAuthority() == true)
	{
		CapsuleTransform    = Capsule->GetComponentTransform();
		CurrentBodyRotation = Capsule->GetComponentRotation();
	}
}

void UCustomPawnMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ShouldSkipTick(DeltaTime) == true) return;

	if (PawnOwner->HasAuthority() == true) // ===== SERVER ===== //
	{
		UpdateTimers(DeltaTime);

		CapsuleVelocity			= Capsule->GetComponentVelocity();
		MovementInputNormalized = GetPendingInputVector().GetSafeNormal();
		CapsuleTransform	    = Capsule->GetComponentTransform();
		CurrentBodyRotation	    = Capsule->GetComponentRotation();

		// Simulation
		ApplySpringForce  (DeltaTime);
		ApplyMovementForce(DeltaTime);
		ApplyRotationForce(DeltaTime);

	}
}


//						 //
// ===== FUNCTIONS ===== //
//						 //

//
// ===== Server ===== //
//

void UCustomPawnMovementComponent::ApplySpringForce(float DeltaTime)
{
	if (!Capsule || PawnOwner->HasAuthority() == false) return;

	const bool BIsIgnoringMass = true; // We use our own mass calculations
	FName BoneName = NAME_None;


	// Ground Check
	const FVector RelativeSpringTraceDirection = bSpringTraceInWorldSpace ? FVector::DownVector : Capsule->GetComponentQuat().RotateVector(SpringTraceDirection);
	const FVector SpringStartLocation          = Capsule->GetComponentLocation();
	const FVector SpringEndLocation            = SpringStartLocation + (RelativeSpringTraceDirection * SpringLength);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(PawnOwner); // Ignore Self in Ground Check

	bGroundDetected = GetWorld()->LineTraceSingleByChannel
	(
		LastGroundHitResult,
		SpringStartLocation,
		SpringEndLocation,
		ECC_Visibility, // Using Visibility Channel for Ground Check, can be changed to custom channel if needed
		CollisionParams
	);


	bIsGrounded = bGroundDetected == true && bIsJumping == false;

	// Apply Gravity when ungrounded
	if (bIsGrounded == false)
	{
		Capsule->AddForce(FVector::DownVector * (Gravity * Mass), BoneName, BIsIgnoringMass);
	}


	// Calculate Spring when Ground Detected
	if (bGroundDetected == true)
	{
		TimeSinceGrounded = 0.f;

		const float RideHeight        = bIsSliding ? SpringRideHeight * SlideRideHeightMultiplier : SpringRideHeight; // Adjust ride height if sliding
		const float RelativeVelocity  = FVector::DotProduct(RelativeSpringTraceDirection, CapsuleVelocity);			  // Velocity along spring direction
		const float SpringOffset      = LastGroundHitResult.Distance - RideHeight;									  // Positive if above ride height, Negative if below
		const float SpringForce		  = (SpringOffset * SpringStrength) - (RelativeVelocity * SpringDamping);		  // Hooke's Law with Damping

		const bool BIsBelowRideHeight = (SpringOffset < 0);
		const bool BIsInJumpBuffer    = (JumpCooldownTimer <= JumpCooldownTime);


		if (bIsGrounded == false)
		{
			// This code should only run once when we land after being ungrounded
			if (BIsBelowRideHeight && BIsInJumpBuffer == false) 
			{
				// Slide Boost
				if (bIsSliding == true)
				{
					FVector SlideForce = FVector::VectorPlaneProject(CapsuleVelocity, LastGroundHitResult.Normal);
					Capsule->AddForce(SlideForce * SlideBoostMultiplier, BoneName, BIsIgnoringMass);
				}

				// Grounded Reset
				bIsGrounded = true;  // Re-enable downward spring force after being ungrounded and then landing
				bIsJumping  = false;  // Reset jumping state upon landing
			}
			else 
			{
				return; // When we are "Grounded" because the Trace hit something but we are still falling towards the ground after a jump, do nothing.
			}
		}

		// Apply Spring Force if Grounded and nothing prevents it
		if (bCanMove == true)
		{
			Capsule->AddForce(RelativeSpringTraceDirection * SpringForce, BoneName, BIsIgnoringMass);
		}
	}

#if WITH_EDITOR
	// Debug Spring Trace
	if (bEnableDebugDrawing)
	{
		FColor LineColor = bGroundDetected ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), SpringStartLocation, SpringEndLocation, LineColor, false, -1.f, 0, 2.f);
		if (bGroundDetected == true)
		{
			DrawDebugPoint(GetWorld(), LastGroundHitResult.ImpactPoint, 10.f, FColor::Yellow, false, -1.f, 0);
		}
		DrawDebugLine(GetWorld(), Capsule->GetComponentLocation() + (FVector::DownVector * 25.f), Capsule->GetComponentLocation() + (CapsuleVelocity), FColor::Blue, false, -1.f, 0, 2.f);
		DrawDebugLine(GetWorld(), Capsule->GetComponentLocation() + (FVector::DownVector * 25.f), Capsule->GetComponentLocation() + (LastReceivedMoveInput * (bIsGrounded ? MaxGroundSpeed : MaxAirSpeed)), FColor::Cyan, false, -1.f, 0, 2.f);
	}
#endif
}

void UCustomPawnMovementComponent::ApplyMovementForce(float DeltaTime)
{
	if (!Capsule || PawnOwner->HasAuthority() == false) return;


	// Calculate Goal Velocity based on Input and Max Speed
	FVector GoalMaxVelocity		 = (LastReceivedMoveInput.GetSafeNormal() * (bIsGrounded ? MaxGroundSpeed : MaxAirSpeed));
	float AccelerationMultiplier = 1.f;
	float DirectionChangeDot	 = FVector::DotProduct(LastReceivedMoveInput.GetSafeNormal(), CapsuleVelocity.GetSafeNormal2D());

	// Increase acceleration when trying to move against current velocity
	if (TurnDotMultiplierCurve)
	{
		AccelerationMultiplier = TurnDotMultiplierCurve->GetFloatValue(DirectionChangeDot);

		// If we have an air control curve, use it
		if (AirControlMultiplierCurve && bIsGrounded == false)
		{
			AccelerationMultiplier *= AirControlMultiplierCurve->GetFloatValue(TimeSinceGrounded);
		}
	}


	if (LastReceivedMoveInput.GetSafeNormal().IsNearlyZero() && bIsGrounded == true)
	{
		// Apply Braking on the ground
		GoalMaxVelocity = FVector::ZeroVector;
		if (TurnDotMultiplierCurve)
		{
			AccelerationMultiplier = TurnDotMultiplierCurve->GetFloatValue(-0.25f); // Reset multiplier to as if the player is turning around slightly
		}
	}


	// Interp to Goal Velocity using Acceleration
	GoalVelocity = FMath::VInterpConstantTo(GoalVelocity, GoalMaxVelocity, DeltaTime, Acceleration * AccelerationMultiplier);


	// If Sliding
	if (bIsSliding == true && bIsGrounded == true)
	{
		FVector SlideGravity       = FVector::VectorPlaneProject(FVector::DownVector * (Gravity * Mass), LastGroundHitResult.Normal);
		FVector SlideSteering      = FVector::VectorPlaneProject(LastReceivedMoveInput.GetSafeNormal() * MaxGroundSpeed, LastGroundHitResult.Normal);
		float   SlideControl       = FVector(CapsuleVelocity.X, CapsuleVelocity.Y, 0).Length() / MaxGroundSpeed;
		FVector SlideForce         = (SlideGravity * SlideGravityMultiplier) + ((SlideSteering * SlideSteeringMultiplier) * SlideControl);
		const FName BoneName       = NAME_None;
		const bool BIsIgnoringMass = true; // We use our own mass calculations
		Capsule->AddForce(SlideForce - (CapsuleVelocity * SlideDamping), BoneName, BIsIgnoringMass);
	}
	else if (bCanMove == false)
	{
		if (bIsSliding == false)
		{
			// This should preserve momentum out of actions but keep the "drifting" slide + movement storage tech
			GoalVelocity = CapsuleVelocity;
		}
	}
	else
	{
		// Calculate the Force need to reach Goal Velocity this frame
		const FVector NeededForceToReachGoalVelocity = FVector(GoalVelocity.X, GoalVelocity.Y, 0) - FVector(CapsuleVelocity.X, CapsuleVelocity.Y, 0);

		// Clamp to tuned Max Force
		const FVector ClampedNeededForce = NeededForceToReachGoalVelocity.GetClampedToMaxSize(MaxMovementForce * AccelerationMultiplier);

		// Apply Movement Force
		Capsule->SetPhysicsLinearVelocity(ClampedNeededForce, true);
	}


	if (LastReceivedMoveInput.IsNearlyZero() == false)
	{
		LastValidMovementInputVector = LastReceivedMoveInput;
	}


	ConsumeInputVector(); // Clear input after processing
}

void UCustomPawnMovementComponent::ApplyRotationForce(float DeltaTime)
{
	if (!Capsule || PawnOwner->HasAuthority() == false) return;


	// Update Variables
	const FVector CurrentAngularVelocity = Capsule->GetPhysicsAngularVelocityInRadians();

	CurrentBodyRotation		 = Capsule->GetComponentRotation();
	CurrentBodyForwardVector = Capsule->GetForwardVector();

	const FRotator CurrentRotation = Capsule->GetComponentRotation();
	const FQuat  CurrentQuaternion = Capsule->GetComponentQuat();

	FRotator TargetRotation;
	FQuat  TargetQuaternion;

	FVector CurrentVelocityNormalized;
	float   CurrentVelocityLength;
	CapsuleVelocity.ToDirectionAndLength(CurrentVelocityNormalized, CurrentVelocityLength);

	FVector CurrentVelocityFlattened = FVector(CapsuleVelocity.X, CapsuleVelocity.Y, 0); // Flatten the Velocity to XY plane

	bool    bIsStopping = false;
	FVector TargetVelocity = FVector::ZeroVector;
	FVector InputVector = LastReceivedMoveInput.GetSafeNormal();

	bool bHasInput = LastReceivedMoveInput.GetSafeNormal().IsNearlyZero() == false;
	bool bIsMoving = FMath::IsNearlyZero(CurrentVelocityFlattened.Length()) == false;

	if (bHasInput == false && bIsMoving == true)
	{
		InputVector = LastValidMovementInputVector * -1.0f;
		bIsStopping = true;
		TargetVelocity = FVector::ZeroVector;
	}
	else
	{
		bIsStopping = false;
		TargetVelocity = InputVector * (bIsGrounded ? MaxGroundSpeed : MaxAirSpeed); // Ideal speed and direction based on input and max speed
	}

	FVector VelocityOffset = TargetVelocity - CurrentVelocityFlattened;
	float VelocityOffsetNormalized = VelocityOffset.Length() / ((bIsGrounded ? MaxGroundSpeed : MaxAirSpeed) * 2); // between 0-1 . 1 is max offset (inputting completely opposite direction while at max speed)

	double Pitch = 0; // Has added tilt when changing directions, otherwise 0
	double Yaw = 0; // Usually head rotation
	double Roll = 0; // No roll, always 0

	Pitch = MaxTiltDegrees * VelocityOffsetNormalized;
	Yaw = LastReceivedControlRotation.Yaw;

	if (bIsStopping == true)
	{
		Pitch *= -1.0f; // invert pitch when stopping to tilt back
	}

	// There are strange issue with moving forward and the standard tilting method,
	// so there is a special case to handle that
	TargetRotation = FRotator(0, Yaw, Roll);
	TargetQuaternion = FQuat(TargetRotation);

	float ForwardDot       = FVector::DotProduct(LastValidMovementInputVector.GetSafeNormal(), TargetQuaternion.GetForwardVector());
	bool bIsMovingForward  = ForwardDot > 0.95f;
	bool bIsMovingBackward = ForwardDot < -0.95f;


	// if we are moving forward use the normal pitch calculation, 
	// We cannot use FRotationMatrix::MakeFromZX and tell it to use the same vector for both up and forward
	if (bIsMovingForward == true)
	{
		TargetRotation = FRotator(-Pitch, Yaw, Roll);
		TargetQuaternion = FQuat(TargetRotation);
	}
	else if (bIsMovingBackward == true)
	{
		TargetRotation = FRotator(Pitch, Yaw, Roll);
		TargetQuaternion = FQuat(TargetRotation);
	}
	else if (bIsMovingForward == false && bIsMovingBackward == false)
	{
		TargetQuaternion = FQuat::Slerp(TargetQuaternion, FRotationMatrix::MakeFromZX(LastValidMovementInputVector.GetSafeNormal(), TargetQuaternion.GetForwardVector()).ToQuat(), Pitch / 90);
	}

	if (bIsUsingUprightSpring == true)
	{
		// Upright force
		FQuat UprightQuaternionBetween = FQuat::FindBetween(CurrentQuaternion.GetUpVector(), TargetQuaternion.GetUpVector());

		FVector UprightAxis = FVector::ZeroVector;
		float UprightRadians = 0.0f;
		UprightQuaternionBetween.ToAxisAndAngle(UprightAxis, UprightRadians);

		FVector UprightTorque = (UprightAxis * (UprightRadians * UprightSpringStrength) - (CurrentAngularVelocity * UprightSpringDamping));

		Capsule->AddTorqueInRadians(UprightTorque, NAME_None, true);
	}

	// Yaw force
	FQuat YawQuaternionBetween = FQuat::FindBetween(CurrentQuaternion.GetForwardVector(), TargetQuaternion.GetForwardVector());

	FVector YawAxis = FVector::ZeroVector;
	float YawRadians = 0.0f;
	YawQuaternionBetween.ToAxisAndAngle(YawAxis, YawRadians);

	FVector YawTorque = (YawAxis * (YawRadians * RotationSpringStrength) - (CurrentAngularVelocity * RotationSpringDamping));
	YawTorque *= Capsule->GetUpVector(); // Ensure torque is applied around the up vector

	Capsule->AddTorqueInRadians(YawTorque, NAME_None, true);


	// Prevent over-rotation
	Pitch = FMath::Clamp(Capsule->GetComponentRotation().Pitch, -45, 45);
	Yaw = Capsule->GetComponentRotation().Yaw;
	Roll = FMath::Clamp(Capsule->GetComponentRotation().Roll, -45, 45);

	FRotator ClampedRotation = FRotator(Pitch, Yaw, Roll);
	Capsule->SetAllPhysicsRotation(ClampedRotation);
}

void UCustomPawnMovementComponent::Jump()
{
	if (!Capsule || bCanJump == false || PawnOwner->HasAuthority() == false) return;

	if (CapsuleVelocity.Z < 0)
	{
		CapsuleVelocity.Z = 0;
	}

	Capsule->SetPhysicsLinearVelocity(FVector(CapsuleVelocity.X * JumpHorizontalMultiplier, CapsuleVelocity.Y * JumpHorizontalMultiplier, JumpForce), false); // Shhh.. dont tell anyone jumping makes you faster

	JumpCooldownTimer = 0.0f; // Reset jump cooldown timer
	bIsJumping = true;
	bCanJump = false;
}

void UCustomPawnMovementComponent::SetSlide(bool bShouldSlide)
{
	bIsSliding = bShouldSlide;

	if (bIsGrounded == true && bIsSliding == true)
	{
		const FName BoneName = NAME_None;
		const bool BIsIgnoringMass = true; // We use our own mass calculations

		FVector SlideForce = FVector::VectorPlaneProject(CapsuleVelocity, LastGroundHitResult.Normal);
		Capsule->AddForce(SlideForce * SlideBoostMultiplier, BoneName, BIsIgnoringMass);
	}
}

void UCustomPawnMovementComponent::UpdateTimers(float DeltaTime)
{
	if (ShouldSkipTick(DeltaTime))
	{
		return;
	}

	if (bIsGrounded == false)
	{
		TimeSinceGrounded += DeltaTime;
	}

	if (JumpCooldownTimer < JumpCooldownTime)
	{
		JumpCooldownTimer += DeltaTime;
	}

	if (TimeSinceGrounded >= JumpCoyoteTime || JumpCooldownTimer < JumpCooldownTime)
	{
		bCanJump = false; // Disable jump if coyote time has passed or if cooldown is active
	}
	else if (TimeSinceGrounded < JumpCoyoteTime && JumpCooldownTimer >= JumpCooldownTime && bIsJumping == false)
	{
		bCanJump = true; // Enable jump if coyote time (includes grounded) is true and cooldown is over and we arent already jumping
	}
}

bool UCustomPawnMovementComponent::ShouldSkipTick(float DeltaTime)
{
	return
		PawnOwner->IsPendingKillPending() ||
		DeltaTime <= 0.0f ||
		GetWorld()->IsPaused();
}

void UCustomPawnMovementComponent::CleanInputs()
{
	LastReceivedMoveInput = FVector::ZeroVector;
}


//
// ===== Remote Procedure Calls ===== //
//

void UCustomPawnMovementComponent::Server_SendMoveInput_Implementation(const FVector& MoveInput)
{
	LastReceivedMoveInput = MoveInput;
}

void UCustomPawnMovementComponent::Server_SendControlRotation_Implementation(const FRotator& ControlRotation)
{
	LastReceivedControlRotation = ControlRotation;
}

void UCustomPawnMovementComponent::Server_RequestJump_Implementation()
{
	Jump();
}

void UCustomPawnMovementComponent::Server_RequestSlide_Implementation(bool bShouldSlide)
{
	SetSlide(bShouldSlide);
}


//
// ===== Replication Notify Callbacks ===== //
//

void UCustomPawnMovementComponent::OnRep_bIsGrounded() {}
void UCustomPawnMovementComponent::OnRep_bIsJumping() {}


//
// ===== Misc ===== //
//

void UCustomPawnMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCustomPawnMovementComponent, CapsuleVelocity);
	DOREPLIFETIME(UCustomPawnMovementComponent, GoalVelocity);
	DOREPLIFETIME(UCustomPawnMovementComponent, bGroundDetected);
	DOREPLIFETIME(UCustomPawnMovementComponent, bIsGrounded);
	DOREPLIFETIME(UCustomPawnMovementComponent, bCanJump);
	DOREPLIFETIME(UCustomPawnMovementComponent, bIsJumping);
	DOREPLIFETIME(UCustomPawnMovementComponent, TimeSinceGrounded);
	DOREPLIFETIME(UCustomPawnMovementComponent, JumpCooldownTimer);
	DOREPLIFETIME(UCustomPawnMovementComponent, LastValidMovementInputVector);
	DOREPLIFETIME(UCustomPawnMovementComponent, MovementInputNormalized);
	DOREPLIFETIME(UCustomPawnMovementComponent, CapsuleTransform);
	DOREPLIFETIME(UCustomPawnMovementComponent, CurrentBodyRotation);
	DOREPLIFETIME(UCustomPawnMovementComponent, CurrentBodyForwardVector);
}

