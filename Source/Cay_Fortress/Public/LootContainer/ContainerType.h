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

	/** 马桶 */
	Toilet = 3 UMETA(DisplayName = "马桶"),

	/** 能源箱 */
	EnergyCrate = 4 UMETA(DisplayName = "能源箱"),

	/** 材料箱 */
	MaterialCrate = 5 UMETA(DisplayName = "材料箱"),

	/** 弹药箱 */
	AmmoCrate = 6 UMETA(DisplayName = "弹药箱"),

	/** 护甲箱 */
	ArmorCrate = 7 UMETA(DisplayName = "护甲箱"),

	/** 装备箱 */
	EquipmentCrate = 8 UMETA(DisplayName = "装备箱"),

	/** 枚举上限（隐藏） */
	MAX = 9 UMETA(Hidden)
};
