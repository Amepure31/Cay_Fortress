// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DestructibleObstacleInterface.generated.h"

UINTERFACE(BlueprintType)
class CAY_FORTRESS_API UDestructibleObstacle : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可被 AI 破坏的障碍物接口 —— 门、路障等实现此接口后，敌人受阻时可检测并攻击。
 */
class CAY_FORTRESS_API IDestructibleObstacle
{
	GENERATED_BODY()

public:
	/** AI 是否可将此障碍物作为目标（未破坏、未锁死等）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A | AI")
	bool CanBeDestroyedByAI() const;

	/** AI 近战命中此障碍物时造成伤害。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A | AI")
	void TakeAIAttackDamage(float DamageAmount);

	/** 障碍物是否已被破坏（AI 恢复寻路）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A | AI")
	bool IsObstacleDestroyed() const;

	/** 障碍物世界位置（供 AI MoveTo 目标）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A | AI")
	FVector GetObstacleLocation() const;
};
