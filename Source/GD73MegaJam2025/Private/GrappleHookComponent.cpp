// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleHookComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "CustomPawnMovementComponent.h"

//									   //
// ===== CONSTRUCTOR & OVERRIDES ===== //
//									   //

// Constructor
UGrappleHookComponent::UGrappleHookComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGrappleHookComponent::OnRegister()
{
	Super::OnRegister();

	if (!Cast<APawn>(GetOwner())) return;

	Capsule = GetOwner()->FindComponentByClass<UCapsuleComponent>();
	CustomMovementComponent = GetOwner()->FindComponentByClass<UCustomPawnMovementComponent>();
}
void UGrappleHookComponent::BeginPlay()
{
	Super::BeginPlay();

    Capsule						= Cast<UCapsuleComponent>(CapsuleRef.GetComponent(GetOwner()));
	CustomMovementComponent	    = Cast<UCustomPawnMovementComponent>(CustomMovementComponentRef.GetComponent(GetOwner()));
	GrappleAttachSceneComponent = Cast<USceneComponent>(GrappleAttachSceneComponentRef.GetComponent(GetOwner()));

	// ...
	
}


//						 //
// ===== FUNCTIONS ===== //
//						 //

//
// ===== Server ===== //
//

void UGrappleHookComponent::ApplyZiplineForce()
{
	if (GetOwnerRole() != ENetRole::ROLE_Authority || !Capsule || !CustomMovementComponent || !GrappleAttachSceneComponent || !CurrentGrappleTarget) return;

	if (GrappleState != EGrappleState::Ziplining && GrappleState != EGrappleState::Releasing)
	{
		GrappleState = EGrappleState::Ziplining;
	}


	CustomMovementComponent->bIsGrounded = false;
	CustomMovementComponent->bCanMove = false;
	GrappleAttachSceneComponent->SetWorldLocation(CurrentGrappleTarget->GetComponentLocation());

	ToGrappleTarget = CurrentGrappleTarget->GetComponentLocation() - Capsule->GetComponentLocation();

	const FVector Force = ToGrappleTarget.GetSafeNormal() * ZiplineStrength;
	const FVector InputForce = (CustomMovementComponent->LastReceivedMoveInput.GetSafeNormal() * ZiplineControl);
	const FVector Damping = (CustomMovementComponent->CapsuleVelocity * ZiplineDamping);

	FVector ZiplineForce = (Force + InputForce) - Damping;

	FName BoneName = NAME_None;
	const bool BIsIgnoringMass = true;

	Capsule->AddForce(ZiplineForce, BoneName, BIsIgnoringMass);

	switch (GrappleState)
	{
		case EGrappleState::Ziplining:
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGrappleHookComponent::ApplyZiplineForce);
			break;
		default:
			break;
	}
}

void UGrappleHookComponent::ApplySwingForce()
{
	if (GetOwnerRole() != ENetRole::ROLE_Authority || !Capsule || !CustomMovementComponent || !GrappleAttachSceneComponent || !CurrentGrappleTarget) return;

	if (GrappleState != EGrappleState::Swinging && GrappleState != EGrappleState::Releasing)
	{
		GrappleState = EGrappleState::Swinging;
	}

	CustomMovementComponent->bCanMove = false;
	GrappleAttachSceneComponent->SetWorldLocation(CurrentGrappleTarget->GetComponentLocation());


	ToGrappleTarget = CurrentGrappleTarget->GetComponentLocation() - Capsule->GetComponentLocation();
	float Distance = ToGrappleTarget.Length();
	bool bIsUnderTension = Distance > CurrentGrappleDistance;

	// Swing
	const float RelativeVelocity = FVector::DotProduct(ToGrappleTarget.GetSafeNormal(), CustomMovementComponent->CapsuleVelocity);
	const float Offset = Distance - CurrentGrappleDistance;

	const float Force = (Offset * SwingSpringStrength) - (RelativeVelocity * SwingSpringDamping);
	//const FVector SwingGravityForce = FVector::VectorPlaneProject(FVector::DownVector * (CustomMovementComponent->Gravity * CustomMovementComponent->Mass), ToGrappleTarget.GetSafeNormal());
	//const FVector TangentVelocity;
	const FVector InputForce = (CustomMovementComponent->LastReceivedMoveInput.GetSafeNormal() * SwingControl);

	FVector CompleteSwingForce = bIsUnderTension ? ((ToGrappleTarget.GetSafeNormal() * Force) + InputForce) : InputForce;
	CompleteSwingForce -= (CustomMovementComponent->CapsuleVelocity * SwingDamping);

	FName BoneName = NAME_None;
	const bool BIsIgnoringMass = true;

	Capsule->AddForce(CompleteSwingForce, BoneName, BIsIgnoringMass);

	if (bControlsRotation && bIsUnderTension)
	{
		// Upright force
		CustomMovementComponent->bIsUsingUprightSpring = false;
		CustomMovementComponent->bIsGrounded = false;

		FQuat UprightQuaternionBetween = FQuat::FindBetween(Capsule->GetComponentQuat().GetUpVector(), ToGrappleTarget.GetSafeNormal());

		FVector UprightAxis  = FVector::ZeroVector;
		float UprightRadians = 0.0f;
		UprightQuaternionBetween.ToAxisAndAngle(UprightAxis, UprightRadians);

		FVector UprightTorque = (UprightAxis * (UprightRadians * SwingRotationSpringStrength) - (Capsule->GetPhysicsAngularVelocityInRadians() * SwingRotationSpringDamping));

		Capsule->AddTorqueInRadians(UprightTorque, NAME_None, true);
	}
	else if (bIsUnderTension == false)
	{
		CustomMovementComponent->bIsUsingUprightSpring = true;

		if (CustomMovementComponent->bIsGrounded)
		{
			CustomMovementComponent->bCanMove = true;
			CustomMovementComponent->bIsUsingUprightSpring = true;
		}
	}

	switch (GrappleState)
	{
		case EGrappleState::Swinging:
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGrappleHookComponent::ApplySwingForce);
			break;

		default:
			break;
	}
}

void UGrappleHookComponent::ThrowGrapple()
{
	if (GetOwnerRole() != ENetRole::ROLE_Authority || !Capsule || !CustomMovementComponent || !GrappleAttachSceneComponent || !CurrentGrappleTarget) return;

	if (GrappleState != EGrappleState::Throwing && GrappleState != EGrappleState::Releasing)
	{
		GrappleState = EGrappleState::Throwing;
	}

	GrappleFlightTimer += GetWorld()->GetDeltaSeconds();

	if (GrappleFlightTimer > GrappleFlightTime)
	{
		GrappleAttachSceneComponent->SetWorldLocation(CurrentGrappleTarget->GetComponentLocation());
		ToGrappleTarget = CurrentGrappleTarget->GetComponentLocation() - Capsule->GetComponentLocation();
		CurrentGrappleDistance = ToGrappleTarget.Length();

		GrappleState = bIsInputDown ? EGrappleState::Swinging : EGrappleState::Ziplining;
	}

	switch (GrappleState)
	{
		case EGrappleState::Throwing:
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGrappleHookComponent::ThrowGrapple);
			break;

		case EGrappleState::Ziplining:
			ApplyZiplineForce();
			break;

		case EGrappleState::Swinging:
			ApplySwingForce();
			break;

		default:
			break;
	}
}

void UGrappleHookComponent::ReleaseGrapple()
{
	if (GetOwnerRole() != ENetRole::ROLE_Authority || !Capsule || !CustomMovementComponent || !GrappleAttachSceneComponent) return;

	if (GrappleState != EGrappleState::Releasing)
	{
		GrappleState = EGrappleState::Releasing;

		CustomMovementComponent->bCanMove = true;
		CustomMovementComponent->bIsUsingUprightSpring = true;
		CustomMovementComponent->bIsGrounded = false;
	}

	GrappleAttachSceneComponent->ResetRelativeTransform();
	GrappleFlightTimer   = 0.f;
	CurrentGrappleTarget = nullptr;
	ToGrappleTarget		 = FVector::ZeroVector;
	GrappleState		 = EGrappleState::Inactive;
}

void UGrappleHookComponent::GrappleJump()
{
	if (GetOwnerRole() != ENetRole::ROLE_Authority || !Capsule || !CustomMovementComponent || !GrappleAttachSceneComponent) return;

	switch (GrappleState)
	{
		case EGrappleState::Ziplining:
			CustomMovementComponent->bCanJump = true;
			CustomMovementComponent->Server_RequestJump();
			ReleaseGrapple();
			break;

		case EGrappleState::Swinging:
			CustomMovementComponent->bCanJump = true;
			CustomMovementComponent->Server_RequestJump();
			ReleaseGrapple();
			break;

		default:
			return;
			break;
	}


}

void UGrappleHookComponent::GetGrappleTarget()
{
	if (GetOwnerRole() != ENetRole::ROLE_Authority || !Capsule || !CustomMovementComponent || !GrappleAttachSceneComponent) return;

	bool bHasTarget		= false;

	const FVector Start = Capsule->GetComponentLocation();
	const FVector End   = Start + (TraceDirection * TraceRange);
	const float Radius  = TraceRadius;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner()); // Ignore Self

	// Trace by channel
	const bool BHit = GetWorld()->SweepMultiByChannel
	(
		TraceHits,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		CollisionParams
	);

	if (BHit)
	{
		FHitResult BestHitResult;
		float BestDistance = TraceRange;
		float BestAngleDot = -1;
		for (const FHitResult& Hit : TraceHits)
		{
			UPrimitiveComponent* HitComponent = Hit.GetComponent();
			const FVector ComponentLocation   = HitComponent->GetComponentLocation();

			// Check for valid hit on GrappleTarget
			if (HitComponent && HitComponent->ComponentHasTag("GrappleTarget"))
			{
				FHitResult VisibilityCheckHit;
				bool      BVisibilityCheckDidHit = GetWorld()->LineTraceSingleByChannel
				(
					VisibilityCheckHit,
					Start,
					ComponentLocation,
					ECC_Visibility,
					CollisionParams
				);

				// Visibility check and comparison with best hit
				if (BVisibilityCheckDidHit && VisibilityCheckHit.GetComponent() == HitComponent)
				{
					const float Distance = (ComponentLocation - Start).Length();
					const float AngleDot = (FVector::DotProduct(TraceDirection, (ComponentLocation - Start).GetSafeNormal()));

					if (AngleDot > BestAngleDot)
					{
						BestDistance  = Distance;
						BestAngleDot  = AngleDot;
						BestHitResult = Hit;
						bHasTarget    = true;
						CurrentGrappleTarget = HitComponent;
					}
					else if (FMath::IsNearlyEqual(AngleDot, BestAngleDot))
					{
						// In the edge case that both targets are equally close to the center of the screen, pick the closest one in distance
						if (Distance < BestDistance)
						{
							BestDistance  = Distance;
							BestAngleDot  = AngleDot;
							BestHitResult = Hit;
							bHasTarget    = true;
							CurrentGrappleTarget = HitComponent;
						}
					}
				}
			}
		}
	}

	if (bHasTarget == true)
	{
		ThrowGrapple();
	}
}

void UGrappleHookComponent::UpdateTimers(float DeltaTime)
{

}


//
// ===== Remote Procedure Calls ===== //
//

void UGrappleHookComponent::Server_SendGrappleInput_Implementation(const FVector& AimDirection, bool bInput)
{
	TraceDirection = AimDirection;
	bIsInputDown   = bInput;

	if (bIsInputDown == true)
	{
		switch (GrappleState)
		{
			case EGrappleState::Inactive:
				GetGrappleTarget();
				break;

			default:
				break;
		}
	}
	else if (bIsInputDown == false)
	{
		switch (GrappleState)
		{
			case EGrappleState::Swinging:
				ReleaseGrapple();
				break;

			default:
				break;
		}
	}
}

void UGrappleHookComponent::Server_RequestGrappleJump_Implementation()
{
	GrappleJump();
}

//
// ===== Misc ===== //
//

void UGrappleHookComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrappleHookComponent, TraceHits);
	DOREPLIFETIME(UGrappleHookComponent, TraceDirection);
	DOREPLIFETIME(UGrappleHookComponent, bIsInputDown);
	DOREPLIFETIME(UGrappleHookComponent, GrappleState);
}

