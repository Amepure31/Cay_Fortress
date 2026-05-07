// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyBehavior.generated.h"

/**
 * 敌人行为树 / AI 状态。
 * 曾用名 E_EnemyBehavior 与旧版错误 UCLASS 在 UObject 路径上冲突，故改为 EEnemyBehavior。
 */
UENUM(BlueprintType)
enum class EEnemyBehavior : uint8
{
	Roam UMETA(DisplayName = "Roam"),
	Alert UMETA(DisplayName = "Alert"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack")
};
