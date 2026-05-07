// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/EnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UEnemyAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	GroundSpeed = 0.f;
	Velocity = FVector::ZeroVector;
	bIsFalling = false;
	AIBehavior = EEnemyBehavior::Roam;
	bIsDead = false;
	bScreamIntroPlaying = false;
	bHitReactMoveLocked = false;

	const ACharacter* const Ch = Cast<ACharacter>(TryGetPawnOwner());
	if (!Ch)
	{
		SmoothedGroundSpeed = FMath::FInterpTo(SmoothedGroundSpeed, 0.f, DeltaSeconds, GroundSpeedSmoothing);
		return;
	}

	if (const UCharacterMovementComponent* const Move = Ch->GetCharacterMovement())
	{
		Velocity = Move->Velocity;
		GroundSpeed = Velocity.Size2D();
		bIsFalling = Move->IsFalling();
	}

	if (const AEnemyCharacter* const En = Cast<AEnemyCharacter>(Ch))
	{
		AIBehavior = En->GetVisualAIBehavior();
		bIsDead = En->IsDead();
		bScreamIntroPlaying = En->IsScreamIntroPlaying();
		bHitReactMoveLocked = En->IsHitReactMoveLocked();
	}

	SmoothedGroundSpeed = FMath::FInterpTo(SmoothedGroundSpeed, GroundSpeed, DeltaSeconds, GroundSpeedSmoothing);
}
