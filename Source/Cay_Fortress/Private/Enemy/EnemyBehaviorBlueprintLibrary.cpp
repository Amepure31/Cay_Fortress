// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyBehaviorBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"

EEnemyBehavior UCayEnemyBehaviorBlueprintLibrary::EnemyBehaviorFromIndex(int32 Index)
{
	switch (Index)
	{
	case 0:
		return EEnemyBehavior::Roam;
	case 1:
		return EEnemyBehavior::Alert;
	case 2:
		return EEnemyBehavior::Chase;
	case 3:
		return EEnemyBehavior::Attack;
	default:
		return EEnemyBehavior::Roam;
	}
}

int32 UCayEnemyBehaviorBlueprintLibrary::EnemyBehaviorToIndex(EEnemyBehavior Value)
{
	return static_cast<int32>(Value);
}

FVector UCayEnemyBehaviorBlueprintLibrary::GetActorNavSampleLocation(const AActor* Actor)
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}
	if (const ACharacter* Ch = Cast<ACharacter>(Actor))
	{
		return Ch->GetNavAgentLocation();
	}
	return Actor->GetActorLocation();
}

bool UCayEnemyBehaviorBlueprintLibrary::SnapWorldLocationToNavMesh(
	UObject* const WorldContextObject,
	const FVector WorldLocation,
	FVector& OutNavLocation,
	const float HorizontalQueryHalfExtent,
	const float VerticalQueryHalfExtent)
{
	OutNavLocation = WorldLocation;
	UWorld* const World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return false;
	}

	FNavLocation NavLoc;
	const FVector QueryExtent(HorizontalQueryHalfExtent, HorizontalQueryHalfExtent, VerticalQueryHalfExtent);
	// UE5.7: (Point, OutLocation, Extent, NavData?, Filter?) — Extent 为第 3 参，勿再插入 nullptr。
	if (NavSys->ProjectPointToNavigation(WorldLocation, NavLoc, QueryExtent))
	{
		OutNavLocation = NavLoc.Location;
		return true;
	}

	if (NavSys->GetRandomReachablePointInRadius(WorldLocation, HorizontalQueryHalfExtent * 2.f, NavLoc))
	{
		OutNavLocation = NavLoc.Location;
		return true;
	}

	return false;
}

