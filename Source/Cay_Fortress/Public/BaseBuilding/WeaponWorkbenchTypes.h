#pragma once

#include "CoreMinimal.h"
#include "WeaponWorkbenchTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponModType : uint8
{
	Damage           UMETA(DisplayName = "伤害"),
	FireRate         UMETA(DisplayName = "射速"),
	MagazineCapacity UMETA(DisplayName = "弹匣容量"),
	Recoil           UMETA(DisplayName = "后坐力(水平+垂直)"),
	Range            UMETA(DisplayName = "射程"),
	ReloadTime       UMETA(DisplayName = "换弹速度"),
	HipFireAccuracy  UMETA(DisplayName = "腰射精度"),
	ADSAccuracy      UMETA(DisplayName = "开镜精度"),
	Handling         UMETA(DisplayName = "操控性"),
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FWeaponModConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "改装类型"))
	EWeaponModType ModType = EWeaponModType::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "最大等级", ClampMin = "1"))
	int32 MaxUpgradeLevels = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "每级增量"))
	float ModifierPerLevel = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "是否乘法叠加"))
	bool bIsMultiplicative = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "消耗物品"))
	FSoftObjectPath CostItemPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "每级消耗数量", ClampMin = "1"))
	int32 CostAmountPerLevel = 3;
};
