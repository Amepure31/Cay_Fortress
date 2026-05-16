#pragma once

#include "CoreMinimal.h"
#include "StorageCabinetTypes.generated.h"

/** 单条消耗：物品路径 + 数量 */
USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FStorageCabinetCostEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (DisplayName = "物品"))
	FSoftObjectPath ItemPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (DisplayName = "数量", ClampMin = "1"))
	int32 Amount = 1;
};

/** 每级升级配置：增加尺寸 + 消耗列表 */
USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FStorageCabinetUpgradeLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (DisplayName = "增加列数", ClampMin = "1"))
	int32 AddedColumns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (DisplayName = "增加行数", ClampMin = "1"))
	int32 AddedRows = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (DisplayName = "消耗物品"))
	TArray<FStorageCabinetCostEntry> CostEntries;
};
