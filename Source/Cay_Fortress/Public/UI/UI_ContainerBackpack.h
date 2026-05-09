#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Inventory.h"
#include "UI_ContainerBackpack.generated.h"

class UInventoryComponent;
class UInventoryItemInstance;

/**
 * 容器背包 Widget：继承 UUI_Inventory，绑定到护甲物品实例上的 ContainerInventory。
 * 在 UMG 中创建子类时须包含 GridPanel 等 UUI_Inventory 所需的绑定控件。
 */
UCLASS()
class CAY_FORTRESS_API UUI_ContainerBackpack : public UUI_Inventory
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ContainerBackpack")
	void BindContainerBackpack(UInventoryItemInstance* InContainerItem);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ContainerBackpack")
	UInventoryItemInstance* GetContainerItem() const { return ContainerItem.Get(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	TWeakObjectPtr<UInventoryItemInstance> ContainerItem;
	TWeakObjectPtr<UInventoryComponent> ContainerInventory;
};
