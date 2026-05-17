// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Inventory.h"
#include "UI_LootContainer.generated.h"

class UInventoryComponent;
class UButton;
class UComboBoxString;
class UTextBlock;
class ALootContainerActor;

/**
 * 容器库存UI
 * 复用 UUI_Inventory 的网格渲染逻辑，只额外记录容器来源
 */
UCLASS()
class CAY_FORTRESS_API UUI_LootContainer : public UUI_Inventory
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LootContainer")
	void BindLootContainer(ALootContainerActor* InLootContainer);

	UFUNCTION(BlueprintCallable, Category = "LootContainer")
	void BindContainerInventory(UInventoryComponent* InContainerInventory);

	UFUNCTION(BlueprintCallable, Category = "LootContainer")
	UInventoryComponent* GetContainerInventory() const { return ContainerInventory; }

	virtual void UpdateDebugButtonVisibility() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "LootContainer|Refresh")
	void OnRefreshButtonClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LootContainer|Refresh")
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LootContainer|Refresh")
	TObjectPtr<UComboBoxString> RefreshTypeComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LootContainer")
	TObjectPtr<UTextBlock> ContainerTypeText;

	UPROPERTY(BlueprintReadOnly, Category = "LootContainer")
	TWeakObjectPtr<ALootContainerActor> BoundLootContainer;

	UPROPERTY(BlueprintReadOnly, Category = "LootContainer")
	TObjectPtr<UInventoryComponent> ContainerInventory;

private:
	void UpdateContainerTypeText();
};
