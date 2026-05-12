// Fill out your copyright notice in the Description page of Project Settings.


#include "LootContainer/RoomActor.h"
#include "LootContainer/LootContainerActor.h"
#include "LootContainer/ContainerGenerationTag.h"
#include "Inventory/InventoryComponent.h"
#include "TimerManager.h"

ARoomActor::ARoomActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoCollectAttachedContainers = true;
	bRefreshContainersOnBeginPlay = true;
	DefaultContainerRefreshChance = 1.0f;
	bAllContainersOpened = false;
	bAllContainersLooted = false;

	bAutoCollectAttachedTags_DEPRECATED = true;
	bGenerateContainersOnBeginPlay_DEPRECATED = true;
	bRefreshContainerItemsOnBeginPlay_DEPRECATED = false;
}

void ARoomActor::BeginPlay()
{
	Super::BeginPlay();

	CollectManagedContainers();

	for (ALootContainerActor* Container : CollectedAttachedContainers)
	{
		RegisterContainerForStateTracking(Container);
	}

	if (bRefreshContainersOnBeginPlay)
	{
		RefreshContainerPresence();
	}
	else
	{
		ActiveContainers.Reset();
		for (ALootContainerActor* Container : CollectedAttachedContainers)
		{
			if (IsValid(Container))
			{
				ActiveContainers.Add(Container);
			}
		}
		EvaluateRoomState();
	}
}

void ARoomActor::PostLoad()
{
	Super::PostLoad();

	// Migrate old serialized flags from previous RoomActor versions.
	bAutoCollectAttachedContainers = bAutoCollectAttachedContainers || bAutoCollectAttachedTags_DEPRECATED;
	bRefreshContainersOnBeginPlay = bRefreshContainersOnBeginPlay || bGenerateContainersOnBeginPlay_DEPRECATED;
}

void ARoomActor::CollectManagedContainers()
{
	CollectedAttachedContainers.Reset();

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (ALootContainerActor* Container = Cast<ALootContainerActor>(AttachedActor))
		{
			CollectedAttachedContainers.Add(Container);
		}
	}

}

void ARoomActor::RefreshContainerPresence()
{
	ActiveContainers.Reset();
	EverOpenedContainers.Reset();

	for (ALootContainerActor* Container : CollectedAttachedContainers)
	{
		if (!IsValid(Container))
		{
			continue;
		}

		const float RefreshChance = GetRefreshChanceForContainerType(Container->ContainerType);
		const bool bActive = FMath::FRand() <= FMath::Clamp(RefreshChance, 0.0f, 1.0f);
		SetContainerActiveState(Container, bActive);

		if (bActive)
		{
			ActiveContainers.Add(Container);
		}
	}

	EvaluateRoomState();
	TriggerSecondRefreshAfterFirstPhase();
}

void ARoomActor::CollectContainerGenerationTags()
{
	CollectManagedContainers();
}

void ARoomActor::GenerateContainers()
{
	RefreshContainerPresence();
}

void ARoomActor::RefreshItemsInActiveContainers()
{
	for (const TWeakObjectPtr<ALootContainerActor>& ContainerPtr : ActiveContainers)
	{
		ALootContainerActor* Container = ContainerPtr.Get();
		if (!IsValid(Container))
		{
			continue;
		}

		Container->RefreshItemsByRoom();
		RequestContainerItemRefresh(Container);
	}
}

void ARoomActor::EvaluateRoomState()
{
	bool bPreviousAllOpened = bAllContainersOpened;
	bool bPreviousAllLooted = bAllContainersLooted;

	TArray<TWeakObjectPtr<ALootContainerActor>> ValidContainers;
	ValidContainers.Reserve(ActiveContainers.Num());

	int32 LootedCount = 0;
	for (const TWeakObjectPtr<ALootContainerActor>& ContainerPtr : ActiveContainers)
	{
		ALootContainerActor* Container = ContainerPtr.Get();
		if (!IsValid(Container))
		{
			continue;
		}

		ValidContainers.Add(Container);

		if (Container->IsContainerOpen())
		{
			EverOpenedContainers.Add(Container);
		}

		if (IsContainerInventoryEmpty(Container))
		{
			++LootedCount;
		}
	}

	ActiveContainers = ValidContainers;

	// 清理失效引用，防止集合长期积累垃圾弱引用。
	TSet<TWeakObjectPtr<ALootContainerActor>> ValidOpenedSet;
	for (const TWeakObjectPtr<ALootContainerActor>& OpenedPtr : EverOpenedContainers)
	{
		if (IsValid(OpenedPtr.Get()))
		{
			ValidOpenedSet.Add(OpenedPtr);
		}
	}
	EverOpenedContainers = MoveTemp(ValidOpenedSet);

	const int32 TotalCount = ActiveContainers.Num();
	if (TotalCount <= 0)
	{
		bAllContainersOpened = false;
		bAllContainersLooted = false;
	}
	else
	{
		bAllContainersOpened = (EverOpenedContainers.Num() >= TotalCount);
		bAllContainersLooted = (LootedCount >= TotalCount);
	}

	if (bPreviousAllOpened != bAllContainersOpened)
	{
		OnAllContainersOpenedStateChanged.Broadcast(bAllContainersOpened);
	}

	if (bPreviousAllLooted != bAllContainersLooted)
	{
		OnAllContainersLootedStateChanged.Broadcast(bAllContainersLooted);
	}
}

void ARoomActor::OnTrackedContainerInventoryChanged()
{
	EvaluateRoomState();
}

void ARoomActor::OnTrackedContainerInventoryToggled(bool bIsOpen)
{
	EvaluateRoomState();
}

bool ARoomActor::IsContainerInventoryEmpty(const ALootContainerActor* Container) const
{
	if (!IsValid(Container))
	{
		return true;
	}

	const UInventoryComponent* Inventory = Container->GetInventoryComponent();
	if (!IsValid(Inventory))
	{
		return true;
	}

	const int32 Width = FMath::Max(1, Inventory->GridWidth);
	const int32 Height = FMath::Max(1, Inventory->GridHeight);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (Inventory->GetItemAtPosition(X, Y) != nullptr)
			{
				return false;
			}
		}
	}

	return true;
}

void ARoomActor::RegisterContainerForStateTracking(ALootContainerActor* Container)
{
	if (!IsValid(Container))
	{
		return;
	}

	if (UInventoryComponent* Inventory = Container->GetInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &ARoomActor::OnTrackedContainerInventoryChanged);
		Inventory->OnInventoryToggled.AddUniqueDynamic(this, &ARoomActor::OnTrackedContainerInventoryToggled);
	}
}

void ARoomActor::SetContainerActiveState(ALootContainerActor* Container, bool bActive)
{
	if (!IsValid(Container))
	{
		return;
	}

	if (!bActive)
	{
		Container->CloseContainer();
	}

	Container->SetActorHiddenInGame(!bActive);
	Container->SetActorEnableCollision(bActive);
	Container->SetActorTickEnabled(bActive);
}

float ARoomActor::GetRefreshChanceForContainerType(EContainerType ContainerType) const
{
	if (const float* OverrideChance = ContainerRefreshChanceByType.Find(ContainerType))
	{
		return FMath::Clamp(*OverrideChance, 0.0f, 1.0f);
	}

	return FMath::Clamp(DefaultContainerRefreshChance, 0.0f, 1.0f);
}

void ARoomActor::RequestContainerItemRefresh(ALootContainerActor* Container)
{
	if (!IsValid(Container))
	{
		return;
	}

	// Room only decides "container exists or not".
	// The actual item refresh logic is external and should be bound in Blueprint/C++.
	OnContainerItemRefreshRequested.Broadcast(Container);
}

void ARoomActor::TriggerSecondRefreshAfterFirstPhase()
{
	// Second phase only runs after first phase succeeds (at least one active container).
	if (ActiveContainers.Num() <= 0)
	{
		return;
	}

	// Defer to next tick to avoid BeginPlay order issues between Room and Containers.
	// Some container-side refresh logic may assume container initialization has completed.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ARoomActor::RefreshItemsInActiveContainers);
	}
}
