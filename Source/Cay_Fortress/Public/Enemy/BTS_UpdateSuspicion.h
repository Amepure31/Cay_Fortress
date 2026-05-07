// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_UpdateSuspicion.generated.h"

/**
 * 挂在行为树「Alert」分支：按视线增减黑板 Suspicion，满则切 Chase。
 * 使用 FName 键名（不用 FBlackboardKeySelector），避免 BT 实例化时 StaticDuplicateObject 与 FString 复制崩溃。
 */
UCLASS()
class CAY_FORTRESS_API UBTS_UpdateSuspicion : public UBTService
{
	GENERATED_BODY()

public:
	UBTS_UpdateSuspicion();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKeyName = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName SuspicionKeyName = TEXT("Suspicion");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName SuspicionMaxKeyName = TEXT("SuspicionMax");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName LastKnownLocationKeyName = TEXT("LastKnownLocation");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName HasLastKnownKeyName = TEXT("HasLastKnown");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName BehaviorStateKeyName = TEXT("BehaviorState");

	/** 与玩家距离 ≤ 此值（cm）时，使用 SuspicionGainPerSecondAtClose。 */
	UPROPERTY(EditAnywhere, Category = "Suspicion", meta = (ClampMin = "50"))
	float SuspicionNearDistance = 400.f;

	/** 与玩家距离 ≥ 此值（cm）时，使用 SuspicionGainPerSecondAtFar（中间线性插值）。 */
	UPROPERTY(EditAnywhere, Category = "Suspicion", meta = (ClampMin = "100"))
	float SuspicionFarDistance = 2200.f;

	UPROPERTY(EditAnywhere, Category = "Suspicion", meta = (ClampMin = "0"))
	float SuspicionGainPerSecondAtClose = 85.f;

	UPROPERTY(EditAnywhere, Category = "Suspicion", meta = (ClampMin = "0"))
	float SuspicionGainPerSecondAtFar = 12.f;

	UPROPERTY(EditAnywhere, Category = "Suspicion", meta = (ClampMin = "0"))
	float SuspicionLossPerSecond = 18.f;
};
