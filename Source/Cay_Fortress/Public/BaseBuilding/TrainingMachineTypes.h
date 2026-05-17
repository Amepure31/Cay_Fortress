#pragma once

#include "CoreMinimal.h"
#include "TrainingMachineTypes.generated.h"

UENUM(BlueprintType)
enum class ETrainingStatType : uint8
{
	MaxHealth           UMETA(DisplayName = "最大生命值"),
	MaxStamina          UMETA(DisplayName = "最大体力"),
	StaminaRecovery     UMETA(DisplayName = "体力恢复速度"),
	MaxHunger           UMETA(DisplayName = "最大饱食度"),
	MaxHydration        UMETA(DisplayName = "最大水分"),
	MaxCarryWeight      UMETA(DisplayName = "最大负重"),
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FTrainingUpgradeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training", meta = (DisplayName = "属性类型"))
	ETrainingStatType StatType = ETrainingStatType::MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training", meta = (DisplayName = "每级增量"))
	float IncrementPerLevel = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training", meta = (DisplayName = "消耗物品"))
	FSoftObjectPath CostItemPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training", meta = (DisplayName = "每级消耗数量", ClampMin = "1"))
	int32 CostAmountPerLevel = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training", meta = (DisplayName = "最大等级", ClampMin = "1"))
	int32 MaxLevels = 10;
};
