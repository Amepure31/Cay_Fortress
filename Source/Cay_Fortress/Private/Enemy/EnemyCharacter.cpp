// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyCharacter.h"
#include "Enemy/EnemyAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.f, 540.f, 0.f);
		Move->MaxWalkSpeed = RoamWalkSpeed;
		Move->NavAgentProps.bCanWalk = true;
	}
	LocomotionTargetMaxWalkSpeed = RoamWalkSpeed;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = FMath::Clamp(Health, 0.f, MaxHealth);
	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		LocomotionTargetMaxWalkSpeed = RoamWalkSpeed;
		Move->MaxWalkSpeed = RoamWalkSpeed;
	}
}

void AEnemyCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSmoothedMaxWalkSpeed(DeltaSeconds);
}

EEnemyBehavior AEnemyCharacter::GetVisualAIBehavior() const
{
	if (const AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
	{
		return AI->GetBehaviorState();
	}
	return EEnemyBehavior::Roam;
}

void AEnemyCharacter::SetHitReactMoveLocked(const bool bLocked)
{
	bHitReactMoveLocked = bLocked;
	const EEnemyBehavior B = GetVisualAIBehavior();
	const bool bHoldScreamInPlace = bScreamIntroPlaying && B == EEnemyBehavior::Chase;
	if (bHitReactMoveLocked || bHoldScreamInPlace)
	{
		ApplyMovementSpeedForBehavior(B, true);
	}
	else
	{
		ApplyMovementSpeedForBehavior(B, false);
	}
}

float AEnemyCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Actual);

	if (USkeletalMeshComponent* const Skel = GetMesh())
	{
		if (UAnimInstance* const AnimInst = Skel->GetAnimInstance())
		{
			if (HitReactMontage)
			{
				AnimInst->Montage_Play(HitReactMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
			}
		}
	}

	if (Health <= KINDA_SMALL_NUMBER)
	{
		PlayDeathMontageAndCleanup();
		return Actual;
	}

	if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
	{
		AI->NotifyDamagedBy(DamageCauser);
	}

	return Actual;
}

void AEnemyCharacter::HandleDeathSequenceStarted()
{
	if (UCharacterMovementComponent* const Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
}

void AEnemyCharacter::UpdateSmoothedMaxWalkSpeed(const float DeltaSeconds)
{
	if (bIsDead)
	{
		return;
	}

	UCharacterMovementComponent* const Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	const bool bScreamHoldInPlace = bScreamIntroPlaying && GetVisualAIBehavior() == EEnemyBehavior::Chase;
	if (bHitReactMoveLocked || bScreamHoldInPlace)
	{
		LocomotionTargetMaxWalkSpeed = 0.f;
		Move->MaxWalkSpeed = 0.f;
		return;
	}

	Move->MaxWalkSpeed = FMath::FInterpTo(
		Move->MaxWalkSpeed,
		LocomotionTargetMaxWalkSpeed,
		DeltaSeconds,
		MaxWalkSpeedInterpSpeed);
}

void AEnemyCharacter::ApplyMovementSpeedForBehavior(const EEnemyBehavior Behavior, const bool bDuringScreamHoldInPlace)
{
	UCharacterMovementComponent* const Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	if (bDuringScreamHoldInPlace || bHitReactMoveLocked)
	{
		LocomotionTargetMaxWalkSpeed = 0.f;
		Move->MaxWalkSpeed = 0.f;
		if (bDuringScreamHoldInPlace)
		{
			Move->StopMovementImmediately();
		}
		return;
	}

	switch (Behavior)
	{
	case EEnemyBehavior::Roam:
		LocomotionTargetMaxWalkSpeed = RoamWalkSpeed;
		break;
	case EEnemyBehavior::Alert:
		LocomotionTargetMaxWalkSpeed = AlertWalkSpeed;
		break;
	case EEnemyBehavior::Chase:
		LocomotionTargetMaxWalkSpeed = ChaseRunSpeed;
		break;
	case EEnemyBehavior::Attack:
		LocomotionTargetMaxWalkSpeed = AttackMoveSpeed;
		break;
	default:
		LocomotionTargetMaxWalkSpeed = RoamWalkSpeed;
		break;
	}
}

void AEnemyCharacter::PlayAttackMontageIfPossible()
{
	USkeletalMeshComponent* const Skel = GetMesh();
	if (!Skel || !AttackMontage)
	{
		if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
		{
			AI->NotifyAttackMontageFinished(false);
		}
		return;
	}
	UAnimInstance* const AnimInst = Skel->GetAnimInstance();
	if (!AnimInst)
	{
		if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
		{
			AI->NotifyAttackMontageFinished(false);
		}
		return;
	}

	if (AnimInst->Montage_IsActive(AttackMontage))
	{
		return;
	}

	const float Len = AnimInst->Montage_Play(AttackMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
	if (Len <= 0.f)
	{
		if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
		{
			AI->NotifyAttackMontageFinished(false);
		}
		return;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyCharacter::OnAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, AttackMontage);
}

void AEnemyCharacter::PlayDeathMontageAndCleanup()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
	{
		AI->NotifyDeathBegin();
	}

	HandleDeathSequenceStarted();

	USkeletalMeshComponent* const Skel = GetMesh();
	UAnimInstance* const AnimInst = Skel ? Skel->GetAnimInstance() : nullptr;
	if (AnimInst && DeathMontage)
	{
		AnimInst->Montage_Play(DeathMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyCharacter::OnDeathMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, DeathMontage);
	}
	else
	{
		SetLifeSpan(0.1f);
	}
}

void AEnemyCharacter::NotifyAIBehaviorChanged(const EEnemyBehavior Previous, const EEnemyBehavior NewState)
{
	if (bIsDead)
	{
		return;
	}

	if (Previous == EEnemyBehavior::Attack && NewState != EEnemyBehavior::Attack)
	{
		if (USkeletalMeshComponent* const Skel = GetMesh())
		{
			if (UAnimInstance* const AnimInst = Skel->GetAnimInstance(); AnimInst && AttackMontage)
			{
				if (AnimInst->Montage_IsActive(AttackMontage))
				{
					AnimInst->Montage_Stop(0.15f, AttackMontage);
				}
			}
		}
	}

	if (NewState == EEnemyBehavior::Chase && Previous == EEnemyBehavior::Alert)
	{
		USkeletalMeshComponent* const Skel = GetMesh();
		UAnimInstance* const AnimInst = Skel ? Skel->GetAnimInstance() : nullptr;
		if (AnimInst && ScreamMontage)
		{
			bScreamIntroPlaying = true;
			if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
			{
				AI->SetChaseMoveSuppressedForScream(true);
			}
			ApplyMovementSpeedForBehavior(NewState, true);

			const float Len = AnimInst->Montage_Play(ScreamMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
			if (Len > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &AEnemyCharacter::OnScreamMontageEnded);
				AnimInst->Montage_SetEndDelegate(EndDelegate, ScreamMontage);
			}
			else
			{
				NotifyScreamIntroMontageEnded();
			}
			return;
		}
	}

	ApplyMovementSpeedForBehavior(NewState, false);

	switch (NewState)
	{
	case EEnemyBehavior::Attack:
		PlayAttackMontageIfPossible();
		break;
	default:
		break;
	}
}

void AEnemyCharacter::NotifyScreamIntroMontageEnded()
{
	if (!bScreamIntroPlaying)
	{
		return;
	}
	bScreamIntroPlaying = false;
	ApplyMovementSpeedForBehavior(EEnemyBehavior::Chase, false);

	if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
	{
		AI->SetChaseMoveSuppressedForScream(false);
	}
}

void AEnemyCharacter::OnScreamMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
	(void)bInterrupted;
	if (Montage != ScreamMontage)
	{
		return;
	}
	NotifyScreamIntroMontageEnded();
}

void AEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
	if (Montage != AttackMontage)
	{
		return;
	}
	if (AEnemyAIController* const AI = Cast<AEnemyAIController>(GetController()))
	{
		AI->NotifyAttackMontageFinished(bInterrupted);
	}
}

void AEnemyCharacter::OnDeathMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
	(void)Montage;
	(void)bInterrupted;
	if (UCapsuleComponent* const Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetLifeSpan(0.05f);
}
