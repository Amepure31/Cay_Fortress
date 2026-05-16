#pragma once

#include "CoreMinimal.h"
#include "UI/UI_LootContainer.h"
#include "UI_StorageCabinet.generated.h"

class AStorageCabinetActor;

UCLASS()
class CAY_FORTRESS_API UUI_StorageCabinet : public UUI_LootContainer
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "StorageCabinet")
	void BindStorageCabinet(AStorageCabinetActor* InCabinet);

	UFUNCTION(BlueprintCallable, Category = "StorageCabinet")
	bool TryUpgradeCabinet();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StorageCabinet")
	bool CanUpgradeCabinet() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StorageCabinet")
	int32 GetUpgradeLevel() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StorageCabinet")
	int32 GetMaxUpgradeLevel() const;

	UFUNCTION(BlueprintCallable, Category = "StorageCabinet")
	void RequestClose();

	UFUNCTION(BlueprintImplementableEvent, Category = "StorageCabinet")
	void BP_OnCabinetBound();

	UFUNCTION(BlueprintImplementableEvent, Category = "StorageCabinet")
	void BP_OnCabinetUpgraded(int32 NewLevel);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "StorageCabinet")
	TObjectPtr<AStorageCabinetActor> BoundCabinet;
};
