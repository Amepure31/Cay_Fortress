#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/FInventoryItemInstance.h"
#include "UI_ItemTooltip.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class CAY_FORTRESS_API UUI_ItemTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* ItemName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* ItemDescription;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UProgressBar* DurabilityBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* StackSizeText;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItem(UInventoryItemInstance* InItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void HideTooltip();

protected:
	virtual void NativeConstruct() override;

private:
	FLinearColor GetRarityColor(EInventoryItemRarity Rarity) const;
};