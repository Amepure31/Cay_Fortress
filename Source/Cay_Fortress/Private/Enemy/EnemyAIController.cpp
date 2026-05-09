// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyAIController.h"
#include "AITypes.h"
#include "Alex_PlayerCharacter.h"
#include "BrainComponent.h"
#include "Enemy/EnemyCharacter.h"
#include "Enemy/EnemyBehaviorBlueprintLibrary.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

/** 惰性 FName，避免 TU 静态初始化顺序导致 NAME_None（日志里会打成 'None'）。 */
namespace EnemyBlackboardKeys
{
	inline FName KeyBehaviorState()
	{
		static const FName N(TEXT("BehaviorState"));
		return N;
	}
	inline FName KeyTargetActor()
	{
		static const FName N(TEXT("TargetActor"));
		return N;
	}
	inline FName KeyLastKnownLocation()
	{
		static const FName N(TEXT("LastKnownLocation"));
		return N;
	}
	inline FName KeyHasLastKnown()
	{
		static const FName N(TEXT("HasLastKnown"));
		return N;
	}
	inline FName KeySuspicion()
	{
		static const FName N(TEXT("Suspicion"));
		return N;
	}
	inline FName KeySuspicionMax()
	{
		static const FName N(TEXT("SuspicionMax"));
		return N;
	}
	inline FName KeyRoamTargetLocation()
	{
		static const FName N(TEXT("RoamTargetLocation"));
		return N;
	}
}

AEnemyAIController::AEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	DefaultSightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("DefaultSightConfig"));

	if (DefaultSightConfig)
	{
		DefaultSightConfig->SightRadius = SightRadius;
		DefaultSightConfig->LoseSightRadius = SightRadius + 400.f;
		DefaultSightConfig->PeripheralVisionAngleDegrees = SightHalfAngleDegrees;
		DefaultSightConfig->SetMaxAge(5.f);
		DefaultSightConfig->AutoSuccessRangeFromLastSeenLocation = 900.f;
		DefaultSightConfig->DetectionByAffiliation.bDetectEnemies = true;
		DefaultSightConfig->DetectionByAffiliation.bDetectFriendlies = true;
		DefaultSightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	}

	if (AIPerception && DefaultSightConfig)
	{
		AIPerception->ConfigureSense(*DefaultSightConfig);
		AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::HandleTargetPerceptionUpdated);
	}

	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTObject(TEXT("/Game/Enemy/Logic/BT_Enemy.BT_Enemy"));
	if (BTObject.Succeeded())
	{
		BehaviorTree = BTObject.Object;
	}
}

void AEnemyAIController::ConfigurePerceptionSenses()
{
	if (!AIPerception || !DefaultSightConfig)
	{
		return;
	}
	DefaultSightConfig->SightRadius = SightRadius;
	DefaultSightConfig->LoseSightRadius = SightRadius + 400.f;
	DefaultSightConfig->PeripheralVisionAngleDegrees = SightHalfAngleDegrees;
	AIPerception->ConfigureSense(*DefaultSightConfig);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RoamSeedAttemptCounter = 0;

	if (InPawn && !bRoamSpawnAnchorValid)
	{
		RoamSpawnAnchor = InPawn->GetActorLocation();
		bRoamSpawnAnchorValid = true;
	}

	ConfigurePerceptionSenses();

	const bool bUsingBT = BehaviorTree != nullptr;
	if (bUsingBT)
	{
		RunBehaviorTree(BehaviorTree);
		SeedRoamBlackboardTarget();
		SyncStateToBlackboard();
	}
	else if (bPrototypeRandomRoam && CurrentState == EEnemyBehavior::Roam)
	{
		RequestRandomMove();
	}
}

void AEnemyAIController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!BehaviorTree)
	{
		ChaseBlackboardRefreshAccumulator = 0.f;
		return;
	}

	AttackCooldownRemain = FMath::Max(0.f, AttackCooldownRemain - DeltaSeconds);

	if (!bEndAttackWhenAttackMontageCompletes && CurrentState == EEnemyBehavior::Attack && AttackPlaceholderRemain > 0.f)
	{
		AttackPlaceholderRemain -= DeltaSeconds;
		if (AttackPlaceholderRemain <= 0.f)
		{
			AttackPlaceholderRemain = 0.f;
			SetBehaviorState(EEnemyBehavior::Chase);
			ChaseBlackboardRefreshAccumulator = 0.f;
		}
	}

	AAlex_PlayerCharacter* const Alex = Cast<AAlex_PlayerCharacter>(FocusTarget);
	APawn* const SelfPawn = GetPawn();

	if (SelfPawn && Alex && CurrentState != EEnemyBehavior::Attack && AttackCooldownRemain <= 0.f)
	{
		const float RangeSq = FMath::Square(AttackTriggerRange);
		if (FVector::DistSquared(SelfPawn->GetActorLocation(), Alex->GetActorLocation()) <= RangeSq
			&& (CurrentState == EEnemyBehavior::Chase || CurrentState == EEnemyBehavior::Alert))
		{
			SetBehaviorState(EEnemyBehavior::Attack);
			ChaseBlackboardRefreshAccumulator = 0.f;
			return;
		}
	}

	if (CurrentState != EEnemyBehavior::Chase || !IsValid(FocusTarget) || !Alex || !bPlayerCurrentlyPerceived)
	{
		ChaseBlackboardRefreshAccumulator = 0.f;
		return;
	}

	if (ChaseBlackboardRefreshInterval > 0.f)
	{
		ChaseBlackboardRefreshAccumulator += DeltaSeconds;
		if (ChaseBlackboardRefreshAccumulator < ChaseBlackboardRefreshInterval)
		{
			return;
		}
		ChaseBlackboardRefreshAccumulator = 0.f;
	}

	ApplyLastKnownFromActor(Alex);
	SyncStateToBlackboard();
	if (bDriveChaseFromController)
	{
		DriveChaseMove();
	}
}

void AEnemyAIController::DriveChaseMove()
{
	if (CurrentState != EEnemyBehavior::Chase || !GetPawn())
	{
		return;
	}
	if (bSuppressChaseMoveForScream || bSuppressChaseMoveForHitReact)
	{
		return;
	}
	FAIMoveRequest MoveRequest(LastKnownPlayerLocation);
	MoveRequest.SetAcceptanceRadius(ChaseMoveAcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetAllowPartialPath(true);
	MoveTo(MoveRequest);
}

void AEnemyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (CurrentState == EEnemyBehavior::Chase && BehaviorTree)
	{
		if (AAlex_PlayerCharacter* const Alex = Cast<AAlex_PlayerCharacter>(FocusTarget))
		{
			ApplyLastKnownFromActor(Alex);
			SyncStateToBlackboard();
			// 不可在此调用 DriveChaseMove：MoveTo 可能同步触发 OnPathFinished → OnMoveCompleted，
			// 与 ApplyLastKnown → ProjectPointToNavigation 叠成无限递归（栈溢出）。
		}
		return;
	}

	if (CurrentState == EEnemyBehavior::Roam && BehaviorTree)
	{
		SeedRoamBlackboardTarget();
		SyncStateToBlackboard();
		return;
	}
	if (bPrototypeRandomRoam && CurrentState == EEnemyBehavior::Roam && !BehaviorTree)
	{
		RequestRandomMove();
	}
}

void AEnemyAIController::SetBehaviorStateByIndex(int32 Index)
{
	SetBehaviorState(UCayEnemyBehaviorBlueprintLibrary::EnemyBehaviorFromIndex(Index));
}

int32 AEnemyAIController::GetBehaviorStateIndex() const
{
	return UCayEnemyBehaviorBlueprintLibrary::EnemyBehaviorToIndex(CurrentState);
}

void AEnemyAIController::SyncStateToBlackboard()
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	BB->SetValueAsEnum(EnemyBlackboardKeys::KeyBehaviorState(), static_cast<uint8>(CurrentState));
	if (FocusTarget)
	{
		BB->SetValueAsObject(EnemyBlackboardKeys::KeyTargetActor(), FocusTarget);
	}
	else
	{
		BB->ClearValue(EnemyBlackboardKeys::KeyTargetActor());
	}
	BB->SetValueAsVector(EnemyBlackboardKeys::KeyLastKnownLocation(), LastKnownPlayerLocation);
	BB->SetValueAsBool(EnemyBlackboardKeys::KeyHasLastKnown(), bHasLastKnownPlayerLocation);
	BB->SetValueAsFloat(EnemyBlackboardKeys::KeySuspicion(), Suspicion);
	BB->SetValueAsFloat(EnemyBlackboardKeys::KeySuspicionMax(), SuspicionMax);
}

void AEnemyAIController::ApplyLastKnownFromActor(AActor* const SourceActor)
{
	if (!SourceActor)
	{
		return;
	}
	if (!Cast<AAlex_PlayerCharacter>(SourceActor))
	{
		return;
	}
	const FVector Sample = UCayEnemyBehaviorBlueprintLibrary::GetActorNavSampleLocation(SourceActor);
	FVector Snapped = Sample;
	UCayEnemyBehaviorBlueprintLibrary::SnapWorldLocationToNavMesh(this, Sample, Snapped, 500.f, 800.f);
	LastKnownPlayerLocation = Snapped;
	bHasLastKnownPlayerLocation = true;
}

void AEnemyAIController::SeedRoamBlackboardTarget()
{
	APawn* const ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!BB)
	{
		if (BehaviorTree && GetWorld())
		{
			if (RoamSeedAttemptCounter < 12)
			{
				++RoamSeedAttemptCounter;
				GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AEnemyAIController::SeedRoamBlackboardTarget));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("EnemyAI: BlackboardComponent still null after %d deferred ticks. Check BT Blackboard."), RoamSeedAttemptCounter);
			}
		}
		return;
	}

	RoamSeedAttemptCounter = 0;

	const FName RoamKeyName = EnemyBlackboardKeys::KeyRoamTargetLocation();
	const FBlackboard::FKey RoamKey = BB->GetKeyID(RoamKeyName);
	if (RoamKey == FBlackboard::InvalidKey)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAI: Blackboard has no Vector key '%s'. Add it to BB_Enemy and point BT_Enemy at that blackboard."),
			*RoamKeyName.ToString());
		return;
	}

	if (!bRoamSpawnAnchorValid)
	{
		RoamSpawnAnchor = ControlledPawn->GetActorLocation();
		bRoamSpawnAnchorValid = true;
	}

	const FVector Anchor = RoamSpawnAnchor;
	const ACharacter* const AsChar = Cast<ACharacter>(ControlledPawn);
	const FVector AgentLoc = AsChar ? AsChar->GetNavAgentLocation() : ControlledPawn->GetActorLocation();

	TArray<FVector, TInlineAllocator<4>> SearchCenters;
	SearchCenters.Add(Anchor);
	if (!AgentLoc.Equals(Anchor, 1.f))
	{
		SearchCenters.Add(AgentLoc);
	}

	FVector TargetLocation = AgentLoc;
	bool bFoundRandom = false;

	if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		for (const FVector& Center : SearchCenters)
		{
			FNavLocation RandomPt;
			if (NavSys->GetRandomReachablePointInRadius(Center, RoamWanderRadius, RandomPt))
			{
				TargetLocation = RandomPt.Location;
				bFoundRandom = true;
				break;
			}

			FVector SnappedCenter = Center;
			if (UCayEnemyBehaviorBlueprintLibrary::SnapWorldLocationToNavMesh(this, Center, SnappedCenter, RoamWanderRadius * 0.35f, 1500.f)
				&& NavSys->GetRandomReachablePointInRadius(SnappedCenter, RoamWanderRadius, RandomPt))
			{
				TargetLocation = RandomPt.Location;
				bFoundRandom = true;
				break;
			}
		}

		if (!bFoundRandom)
		{
			for (const FVector& Center : SearchCenters)
			{
				if (UCayEnemyBehaviorBlueprintLibrary::SnapWorldLocationToNavMesh(this, Center, TargetLocation, 1200.f, 1500.f))
				{
					bFoundRandom = true;
					break;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAI: No NavigationSystem — RoamTargetLocation may be off-mesh."));
	}

	BB->SetValueAsVector(EnemyBlackboardKeys::KeyRoamTargetLocation(), TargetLocation);
	BB->SetValueAsEnum(EnemyBlackboardKeys::KeyBehaviorState(), static_cast<uint8>(EEnemyBehavior::Roam));
	CurrentState = EEnemyBehavior::Roam;
}

void AEnemyAIController::SetBehaviorState(EEnemyBehavior NewState)
{
	if (CurrentState == NewState)
	{
		SyncStateToBlackboard();
		return;
	}

	const EEnemyBehavior Previous = CurrentState;
	CurrentState = NewState;

	if (NewState == EEnemyBehavior::Chase && Previous == EEnemyBehavior::Attack)
	{
		bPlayerCurrentlyPerceived = true;
	}

	if (NewState == EEnemyBehavior::Attack)
	{
		AttackPlaceholderRemain = bEndAttackWhenAttackMontageCompletes ? 0.f : AttackPlaceholderDuration;
	}
	else
	{
		AttackPlaceholderRemain = 0.f;
	}

	if (NewState == EEnemyBehavior::Chase && FocusTarget)
	{
		ApplyLastKnownFromActor(FocusTarget);
	}

	if (NewState == EEnemyBehavior::Roam)
	{
		FocusTarget = nullptr;
		bPlayerCurrentlyPerceived = false;
		Suspicion = 0.f;
		GetWorldTimerManager().ClearTimer(RetryMoveTimerHandle);
		StopMovement();
		if (BehaviorTree)
		{
			SeedRoamBlackboardTarget();
		}
		else if (bPrototypeRandomRoam)
		{
			RequestRandomMove();
		}
	}
	else if (Previous == EEnemyBehavior::Roam)
	{
		GetWorldTimerManager().ClearTimer(RetryMoveTimerHandle);
		StopMovement();
	}

	if (Previous == EEnemyBehavior::Chase && NewState != EEnemyBehavior::Chase && bDriveChaseFromController)
	{
		StopMovement();
	}

	if (Previous == EEnemyBehavior::Attack && NewState != EEnemyBehavior::Attack)
	{
		AttackCooldownRemain = AttackIntervalSeconds;
	}

	SyncStateToBlackboard();

	if (AEnemyCharacter* const En = Cast<AEnemyCharacter>(GetPawn()))
	{
		En->NotifyAIBehaviorChanged(Previous, NewState);
	}

	if (NewState == EEnemyBehavior::Chase && bDriveChaseFromController && !bSuppressChaseMoveForScream && !bSuppressChaseMoveForHitReact)
	{
		DriveChaseMove();
	}
}

void AEnemyAIController::NotifyAttackMontageFinished(bool bInterruptedMontage)
{
	(void)bInterruptedMontage;
	if (const AEnemyCharacter* const En = Cast<AEnemyCharacter>(GetPawn()))
	{
		if (En->IsDead())
		{
			return;
		}
	}
	if (CurrentState != EEnemyBehavior::Attack)
	{
		return;
	}
	SetBehaviorState(EEnemyBehavior::Chase);
	ChaseBlackboardRefreshAccumulator = 0.f;
}

void AEnemyAIController::SetChaseMoveSuppressedForScream(const bool bSuppress)
{
	bSuppressChaseMoveForScream = bSuppress;
	if (!bSuppress && CurrentState == EEnemyBehavior::Chase && bDriveChaseFromController && !bSuppressChaseMoveForHitReact)
	{
		DriveChaseMove();
	}
}

void AEnemyAIController::NotifyDamagedBy(AActor* const DamageCauser)
{
	StopMovement();
	bSuppressChaseMoveForHitReact = true;
	if (AEnemyCharacter* const En = Cast<AEnemyCharacter>(GetPawn()))
	{
		En->SetHitReactMoveLocked(true);
	}

	if (AAlex_PlayerCharacter* const AlexFromCauser = Cast<AAlex_PlayerCharacter>(DamageCauser))
	{
		FocusTarget = AlexFromCauser;
	}
	else if (DamageCauser)
	{
		if (AActor* const Inst = DamageCauser->GetInstigator())
		{
			if (AAlex_PlayerCharacter* const A = Cast<AAlex_PlayerCharacter>(Inst))
			{
				FocusTarget = A;
			}
		}
	}
	if (!FocusTarget && GetWorld())
	{
		if (APlayerController* const PC = GetWorld()->GetFirstPlayerController())
		{
			FocusTarget = Cast<AAlex_PlayerCharacter>(PC->GetPawn());
		}
	}
	bPlayerCurrentlyPerceived = FocusTarget != nullptr;

	if (FocusTarget)
	{
		// 受击瞬间把黑板 LastKnown 拉到玩家当前位置（不依赖 Chase Tick 间隔），并锁定 AI 注视目标。
		ApplyLastKnownFromActor(FocusTarget);
		SetFocus(FocusTarget, EAIFocusPriority::Gameplay);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(HitReactStunTimerHandle);
		GetWorldTimerManager().SetTimer(HitReactStunTimerHandle, this, &AEnemyAIController::OnHitReactStunTimer, HitReactStunSeconds, false);
	}
	SyncStateToBlackboard();
}

void AEnemyAIController::OnHitReactStunTimer()
{
	bSuppressChaseMoveForHitReact = false;
	if (AEnemyCharacter* const En = Cast<AEnemyCharacter>(GetPawn()))
	{
		En->SetHitReactMoveLocked(false);
	}

	if (CurrentState == EEnemyBehavior::Roam)
	{
		SyncStateToBlackboard();
		return;
	}

	if (CurrentState == EEnemyBehavior::Chase && bDriveChaseFromController && !bSuppressChaseMoveForScream)
	{
		SyncStateToBlackboard();
		DriveChaseMove();
		return;
	}

	SetBehaviorState(EEnemyBehavior::Chase);
}

void AEnemyAIController::NotifyDeathBegin()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(HitReactStunTimerHandle);
	}
	bSuppressChaseMoveForHitReact = false;
	bSuppressChaseMoveForScream = false;
	if (AEnemyCharacter* const En = Cast<AEnemyCharacter>(GetPawn()))
	{
		En->SetHitReactMoveLocked(false);
	}
	if (UBrainComponent* const Brain = GetBrainComponent())
	{
		Brain->StopLogic(TEXT("Dead"));
	}
	StopMovement();
}

void AEnemyAIController::RequestRandomMove()
{
	if (!bPrototypeRandomRoam || CurrentState != EEnemyBehavior::Roam || BehaviorTree)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(RetryMoveTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(RetryMoveTimerHandle);
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		GetWorldTimerManager().SetTimer(RetryMoveTimerHandle, this, &AEnemyAIController::RequestRandomMove, RetryDelaySeconds, false);
		return;
	}

	FNavLocation RandomPt;
	const FVector Origin = ControlledPawn->GetActorLocation();
	if (NavSys->GetRandomReachablePointInRadius(Origin, RandomMoveRadius, RandomPt))
	{
		MoveToLocation(RandomPt.Location);
	}
	else
	{
		GetWorldTimerManager().SetTimer(RetryMoveTimerHandle, this, &AEnemyAIController::RequestRandomMove, RetryDelaySeconds, false);
	}
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	AAlex_PlayerCharacter* const Alex = Cast<AAlex_PlayerCharacter>(Actor);
	if (!Alex)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		bPlayerCurrentlyPerceived = true;
		FocusTarget = Alex;
		ApplyLastKnownFromActor(Alex);
		if (CurrentState == EEnemyBehavior::Chase)
		{
			SyncStateToBlackboard();
			return;
		}
		SetBehaviorState(EEnemyBehavior::Alert);
		return;
	}

	bPlayerCurrentlyPerceived = false;

	if (FocusTarget != Alex)
	{
		SyncStateToBlackboard();
		return;
	}

	if (CurrentState == EEnemyBehavior::Chase || CurrentState == EEnemyBehavior::Attack)
	{
		ApplyLastKnownFromActor(Alex);
		Suspicion = FMath::Clamp(SuspicionMax * AlertSuspicionFractionAfterChaseLoss, 0.f, SuspicionMax);
		SetBehaviorState(EEnemyBehavior::Alert);
	}

	SyncStateToBlackboard();
}
