// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/PlayerInteractComponent.h"
#include "Interaction/InteractableInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"

UPlayerInteractComponent::UPlayerInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	InteractDistance = 250.0f;
	TraceRadius = 20.0f;
	bDebugInteraction = true;
}


void UPlayerInteractComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UPlayerInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateInteractableTarget();
}

bool UPlayerInteractComponent::TryInteract()
{
	UpdateInteractableTarget();

	AActor* Target = CurrentInteractable.Get();
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Target)
	{
		ShowDebugMessage(TEXT("[Interact] 未检测到可交互目标"), FColor::Red);
		return false;
	}

	if (!Target->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		ShowDebugMessage(TEXT("[Interact] 目标未实现交互接口"), FColor::Red);
		return false;
	}

	if (!IInteractableInterface::Execute_CanInteract(Target, OwnerActor))
	{
		ShowDebugMessage(FString::Printf(TEXT("[Interact] 目标不可交互: %s"), *Target->GetName()), FColor::Orange);
		return false;
	}

	IInteractableInterface::Execute_Interact(Target, OwnerActor);
	ShowDebugMessage(FString::Printf(TEXT("[Interact] 交互成功: %s"), *Target->GetName()), FColor::Green, 2.0f);
	return true;
}

void UPlayerInteractComponent::UpdateInteractableTarget()
{
	const AActor* PreviousTarget = CurrentInteractable.Get();
	CurrentInteractable = FindBestInteractable();
	const AActor* NewTarget = CurrentInteractable.Get();

	if (PreviousTarget != NewTarget)
	{
		if (NewTarget)
		{
			ShowDebugMessage(FString::Printf(TEXT("[Interact] 检测到目标: %s"), *NewTarget->GetName()), FColor::Cyan);
		}
		else if (PreviousTarget)
		{
			ShowDebugMessage(TEXT("[Interact] 离开交互目标"), FColor::Silver, 1.0f);
		}
	}
}

AActor* UPlayerInteractComponent::FindBestInteractable() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	FVector Start = OwnerPawn->GetActorLocation();
	FRotator ViewRotation = OwnerPawn->GetActorRotation();
	OwnerPawn->GetActorEyesViewPoint(Start, ViewRotation);
	const FVector Forward = ViewRotation.Vector();
	const FVector End = Start + Forward * InteractDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerInteractTrace), false, OwnerPawn);
	TArray<FHitResult> Hits;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);

	if (!bHit)
	{
		return nullptr;
	}

	AActor* BestActor = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || !HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		{
			continue;
		}

		if (!IInteractableInterface::Execute_CanInteract(HitActor, const_cast<APawn*>(OwnerPawn)))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), HitActor->GetActorLocation());
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestActor = HitActor;
		}
	}

	return BestActor;
}

void UPlayerInteractComponent::ShowDebugMessage(const FString& Message, FColor Color, float Duration) const
{
	if (!bDebugInteraction || !GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
}
