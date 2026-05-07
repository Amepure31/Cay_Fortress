// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/BTS_UpdateSuspicion.h"
#include "Alex_PlayerCharacter.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/EnemyBehavior.h"
#include "Enemy/EnemyBehaviorBlueprintLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UBTS_UpdateSuspicion::UBTS_UpdateSuspicion()
{
	NodeName = TEXT("Update Suspicion");
	bNotifyTick = true;
	Interval = 0.1f;
	RandomDeviation = 0.02f;
}

void UBTS_UpdateSuspicion::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* const BBC = OwnerComp.GetBlackboardComponent();
	if (!BBC)
	{
		return;
	}

	if (TargetActorKeyName.IsNone() || SuspicionKeyName.IsNone() || SuspicionMaxKeyName.IsNone())
	{
		return;
	}

	AActor* const TargetActor = Cast<AActor>(BBC->GetValueAsObject(TargetActorKeyName));
	AAlex_PlayerCharacter* const TargetAlex = Cast<AAlex_PlayerCharacter>(TargetActor);
	APawn* const SelfPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!SelfPawn || !TargetAlex)
	{
		return;
	}

	bool bHasLineOfSight = false;
	{
		const FVector EyeLocation = SelfPawn->GetPawnViewLocation();
		const FVector TargetLocation = TargetAlex->GetActorLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySuspicionLOS), false, SelfPawn);
		Params.AddIgnoredActor(SelfPawn);
		FHitResult Hit;
		UWorld* const World = SelfPawn->GetWorld();
		if (World && World->LineTraceSingleByChannel(Hit, EyeLocation, TargetLocation, ECC_Visibility, Params))
		{
			bHasLineOfSight = Hit.GetActor() == TargetAlex;
		}
		else
		{
			bHasLineOfSight = true;
		}
	}

	float Suspicion = BBC->GetValueAsFloat(SuspicionKeyName);
	const float SuspicionMax = FMath::Max(1.f, BBC->GetValueAsFloat(SuspicionMaxKeyName));

	if (bHasLineOfSight)
	{
		const float Dist = FVector::Dist(SelfPawn->GetActorLocation(), TargetAlex->GetActorLocation());
		const float Near = SuspicionNearDistance;
		const float Far = FMath::Max(Near + 50.f, SuspicionFarDistance);
		const float Alpha = FMath::Clamp((Dist - Near) / (Far - Near), 0.f, 1.f);
		const float Gain = FMath::Lerp(SuspicionGainPerSecondAtClose, SuspicionGainPerSecondAtFar, Alpha);
		Suspicion = FMath::Min(SuspicionMax, Suspicion + Gain * DeltaSeconds);
		if (!LastKnownLocationKeyName.IsNone())
		{
			const FVector Sample = UCayEnemyBehaviorBlueprintLibrary::GetActorNavSampleLocation(TargetAlex);
			FVector Snapped = Sample;
			UCayEnemyBehaviorBlueprintLibrary::SnapWorldLocationToNavMesh(SelfPawn, Sample, Snapped, 500.f, 800.f);
			BBC->SetValueAsVector(LastKnownLocationKeyName, Snapped);
			if (AEnemyAIController* const AI = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
			{
				AI->LastKnownPlayerLocation = Snapped;
				AI->bHasLastKnownPlayerLocation = true;
			}
		}
		if (!HasLastKnownKeyName.IsNone())
		{
			BBC->SetValueAsBool(HasLastKnownKeyName, true);
		}
	}
	else
	{
		Suspicion = FMath::Max(0.f, Suspicion - SuspicionLossPerSecond * DeltaSeconds);
	}

	if (Suspicion <= KINDA_SMALL_NUMBER)
	{
		Suspicion = 0.f;
		BBC->SetValueAsFloat(SuspicionKeyName, 0.f);
		if (AEnemyAIController* const AI = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
		{
			AI->Suspicion = 0.f;
			AI->SetBehaviorState(EEnemyBehavior::Roam);
		}
		return;
	}

	BBC->SetValueAsFloat(SuspicionKeyName, Suspicion);

	if (AEnemyAIController* const AI = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		AI->Suspicion = Suspicion;
	}

	if (bHasLineOfSight && Suspicion >= SuspicionMax - KINDA_SMALL_NUMBER && !BehaviorStateKeyName.IsNone())
	{
		AEnemyAIController* const AI = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
		if (AI && AI->GetBehaviorState() != EEnemyBehavior::Chase)
		{
			BBC->SetValueAsEnum(BehaviorStateKeyName, static_cast<uint8>(EEnemyBehavior::Chase));
			AI->SetBehaviorState(EEnemyBehavior::Chase);
			AI->bPlayerCurrentlyPerceived = true;
		}
	}
}
