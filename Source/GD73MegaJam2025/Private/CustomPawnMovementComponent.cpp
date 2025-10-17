// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomPawnMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UCustomPawnMovementComponent::UCustomPawnMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	PawnOwner = Cast<APawn>(GetOwner());

	// Set up Capsule
	Capsule = PawnOwner->FindComponentByClass<UCapsuleComponent>();

	if (!Capsule)
	{
		Capsule = NewObject<UCapsuleComponent>(PawnOwner, TEXT("MovementCapsule"));
		Capsule->RegisterComponent();
		Capsule->SetupAttachment(PawnOwner->GetRootComponent());
	}
	
	Capsule->SetCapsuleHalfHeight(45.f);
	Capsule->SetCapsuleRadius(35.f);
	Capsule->SetSimulatePhysics(true);
	Capsule->SetEnableGravity(false);
	Capsule->SetIsReplicated(true);

}

void UCustomPawnMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCustomPawnMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

