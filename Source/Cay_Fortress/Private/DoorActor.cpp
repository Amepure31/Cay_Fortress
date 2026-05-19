// Fill out your copyright notice in the Description page of Project Settings.

#include "DoorActor.h"
#include "CayFortressCollisionChannels.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"

ADoorActor::ADoorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	SetRootComponent(DoorMesh);

	// AI 视野穿透门（不阻挡 Visibility 射线）
	DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	// 武器射线不能穿透门
	DoorMesh->SetCollisionResponseToChannel(CayFortressCollision::WeaponTrace, ECR_Block);

	DoorDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DoorDetectionSphere"));
	DoorDetectionSphere->SetupAttachment(DoorMesh);
	DoorDetectionSphere->SetSphereRadius(120.f);
	DoorDetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorDetectionSphere->SetCollisionObjectType(CayFortressCollision::DestructibleObstacle);
	DoorDetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DoorDetectionSphere->SetCollisionResponseToChannel(CayFortressCollision::DestructibleObstacle, ECR_Block);
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = FMath::Clamp(MaxHealth, 1.f, MAX_flt);
}

bool ADoorActor::CanBeDestroyedByAI_Implementation() const
{
	return !bDestroyed;
}

void ADoorActor::TakeAIAttackDamage_Implementation(const float DamageAmount)
{
	if (bDestroyed || DamageAmount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - DamageAmount);

	OnDoorTakeDamage(DamageAmount, CurrentHealth / FMath::Max(1.f, MaxHealth));

	if (CurrentHealth <= KINDA_SMALL_NUMBER)
	{
		DestroyDoor();
	}
}

bool ADoorActor::IsObstacleDestroyed_Implementation() const
{
	return bDestroyed;
}

FVector ADoorActor::GetObstacleLocation_Implementation() const
{
	return GetActorLocation();
}

void ADoorActor::DestroyDoor()
{
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	OnDoorDestroyed();

	// 触发导航重建：标记该 Actor 的 Octree 边界已脏，配合 Dynamic RuntimeGeneration 自动重建 Tile
	if (UWorld* const World = GetWorld())
	{
		if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			NavSys->UpdateNavOctreeBounds(this);
		}
	}
}
