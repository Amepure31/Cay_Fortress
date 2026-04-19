// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "ContainerType.generated.h"

/**
 * 容器类型枚举
 */
UENUM(BlueprintType)
enum class EContainerType : uint8
{
	/** 食物箱 */
	FoodCrate   = 0 UMETA(DisplayName = "食物箱"),

	/** 武器箱 */
	WeaponCrate = 1 UMETA(DisplayName = "武器箱"),

	/** 医疗箱 */
	MedicalCrate = 2 UMETA(DisplayName = "医疗箱"),

	/** 枚举上限（隐藏） */
	MAX = 3 UMETA(Hidden)
};
