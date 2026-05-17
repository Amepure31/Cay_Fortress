// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_LootContainer.h"
#include "Inventory/InventoryComponent.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "LootContainer/LootContainerActor.h"
#include "LootContainer/ContainerType.h"
#include "Alex_PlayerController.h"

void UUI_LootContainer::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &UUI_LootContainer::OnRefreshButtonClicked);
	}

	if (RefreshTypeComboBox)
	{
		// No longer used. Refresh type is now determined by container type directly.
		RefreshTypeComboBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateContainerTypeText();
	UpdateDebugButtonVisibility();
}

void UUI_LootContainer::NativeDestruct()
{
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &UUI_LootContainer::OnRefreshButtonClicked);
	}

	Super::NativeDestruct();
}

void UUI_LootContainer::BindLootContainer(ALootContainerActor* InLootContainer)
{
	BoundLootContainer = InLootContainer;
	BindContainerInventory(InLootContainer ? InLootContainer->GetInventoryComponent() : nullptr);
	UpdateContainerTypeText();
}

void UUI_LootContainer::BindContainerInventory(UInventoryComponent* InContainerInventory)
{
	ContainerInventory = InContainerInventory;
	BindInventory(InContainerInventory);
}

void UUI_LootContainer::UpdateDebugButtonVisibility()
{
	Super::UpdateDebugButtonVisibility();

	bool bDebug = false;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AAlex_PlayerController* APC = Cast<AAlex_PlayerController>(PC))
			bDebug = APC->IsDebugUIVisible();
	}
	ESlateVisibility Vis = bDebug ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (RefreshButton)
		RefreshButton->SetVisibility(Vis);
}

void UUI_LootContainer::OnRefreshButtonClicked()
{
	if (ALootContainerActor* LootContainer = BoundLootContainer.Get())
	{
		LootContainer->RefreshItemsByRoom();
		UpdateInventory();
	}
}

void UUI_LootContainer::UpdateContainerTypeText()
{
	if (!ContainerTypeText)
	{
		return;
	}

	if (ALootContainerActor* LootContainer = BoundLootContainer.Get())
	{
		if (const UEnum* ContainerTypeEnum = StaticEnum<EContainerType>())
		{
			ContainerTypeText->SetText(ContainerTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(LootContainer->ContainerType)));
			return;
		}
	}

	ContainerTypeText->SetText(FText::FromString(TEXT("未知")));
}

