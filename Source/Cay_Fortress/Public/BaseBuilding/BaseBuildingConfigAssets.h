#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BaseBuilding/TrainingMachineTypes.h"
#include "BaseBuilding/WeaponWorkbenchTypes.h"
#include "BaseBuilding/StorageCabinetTypes.h"
#include "BaseBuildingConfigAssets.generated.h"

/** 训练机配方表 DataAsset — 一份资产 = 一套训练配方，可被多台训练机共享 */
UCLASS(BlueprintType)
class CAY_FORTRESS_API UTrainingMachineConfigAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Training")
	TArray<FTrainingUpgradeConfig> UpgradeConfigs;
};

/** 武器工作台配方表 DataAsset — 一份资产 = 一套改装配方，可被多台工作台共享 */
UCLASS(BlueprintType)
class CAY_FORTRESS_API UWeaponWorkbenchConfigAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Workbench")
	TArray<FWeaponModConfig> ModConfigs;
};

/** 储物柜配置 DataAsset — 一份资产 = 一套柜子参数，可被多台储物柜共享 */
UCLASS(BlueprintType)
class CAY_FORTRESS_API UStorageCabinetConfigAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "初始列数", ClampMin = "1"))
	int32 InitialGridWidth = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "初始行数", ClampMin = "1"))
	int32 InitialGridHeight = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "最大列数", ClampMin = "1"))
	int32 MaxGridWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "最大行数", ClampMin = "1"))
	int32 MaxGridHeight = 10;

	/** 每级升级：增加的列/行数和消耗物品 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Upgrade", meta = (DisplayName = "升级等级"))
	TArray<FStorageCabinetUpgradeLevel> UpgradeLevels;
};
