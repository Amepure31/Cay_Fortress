#include "UI/UI_Inventory.h"
#include "UI/UI_ItemSlot.h"
#include "UI/UI_ItemTooltip.h"
#include "UI/UI_ItemWidget.h"
#include "Components/UniformGridPanel.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "SlateBasics.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"
#include "Misc/PackageName.h"
#include "Blueprint/DragDropOperation.h"

namespace
{
static TSubclassOf<UUI_ItemWidget> ResolveDefaultItemWidgetClass()
{
	static const FName ItemWidgetFolderPath(TEXT("/Game/UI/WB_UI_Item"));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(ItemWidgetFolderPath, AssetDataList, true);
	if (AssetDataList.Num() == 0)
	{
		return nullptr;
	}

	for (const FAssetData& AssetData : AssetDataList)
	{
		FString GeneratedClassPath;
		if (!AssetData.GetTagValue(TEXT("GeneratedClass"), GeneratedClassPath))
		{
			continue;
		}

		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPath);
		if (ClassObjectPath.IsEmpty())
		{
			continue;
		}

		UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassObjectPath);
		if (!LoadedClass)
		{
			continue;
		}

		if (LoadedClass->IsChildOf(UUI_ItemWidget::StaticClass()))
		{
			return LoadedClass;
		}
	}

	return nullptr;
}

static bool ExtractDragPayload(UDragDropOperation* InOperation, UUI_ItemWidget*& OutSourceWidget, UInventoryItemInstance*& OutItemInstance)
{
	OutSourceWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	OutItemInstance = OutSourceWidget ? OutSourceWidget->GetItemInstance() : nullptr;
	return OutSourceWidget && OutItemInstance;
}
}

void UUI_Inventory::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);

	if (AddItemButton)
	{
		AddItemButton->OnClicked.RemoveDynamic(this, &UUI_Inventory::OnAddItemButtonClicked);
		AddItemButton->OnClicked.AddDynamic(this, &UUI_Inventory::OnAddItemButtonClicked);
	}
	if (AddItemComboBox)
	{
		AddItemComboBox->OnSelectionChanged.RemoveDynamic(this, &UUI_Inventory::OnAddItemSelectionChanged);
		AddItemComboBox->OnSelectionChanged.AddDynamic(this, &UUI_Inventory::OnAddItemSelectionChanged);
		SetAddItemListVisible(false);
	}
	
	if (GridWidth <= 0)
	{
		GridWidth = 6;
	}
	if (GridHeight <= 0)
	{
		GridHeight = 10;
	}
	if (SlotSize <= 0)
	{
		SlotSize = 64;
	}
	if (SlotSpacing < 0)
	{
		SlotSpacing = 0;
	}
	if (!ItemWidgetClass)
	{
		ItemWidgetClass = ResolveDefaultItemWidgetClass();
	}

	if (ItemWidgetClass)
	{
		for (UUI_ItemSlot* GridSlot : ItemSlots)
		{
			if (GridSlot)
			{
				GridSlot->SetItemWidgetClass(ItemWidgetClass);
			}
		}
	}
	
	SetSlotSize();

	if (BoundInventory && ItemSlots.Num() > 0)
	{
		UpdateInventory();
	}
}

FReply UUI_Inventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUI_Inventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DraggedItemWidget && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		UInventoryItemInstance* ItemInstance = DraggedItemWidget->GetItemInstance();
		UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(InMouseEvent.GetScreenSpacePosition());
		bool bPlaced = false;
		if (ItemInstance && HoveredSlot)
		{
			const int32 TargetOriginX = HoveredSlot->GetGridX() - DraggedItemWidget->GetDragGrabCellX();
			const int32 TargetOriginY = HoveredSlot->GetGridY() - DraggedItemWidget->GetDragGrabCellY();
			bPlaced = TryPlaceDraggedItem(ItemInstance, TargetOriginX, TargetOriginY);
		}
		if (!bPlaced)
		{
			UpdateInventory();
			ClearPlacementPreview();
		}
		DraggedItemWidget = nullptr;
		return FReply::Handled();
	}

	ClearPlacementPreview();
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UUI_Inventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!DraggedItemWidget)
	{
		return;
	}

	UInventoryItemInstance* ItemInstance = DraggedItemWidget->GetItemInstance();
	if (!ItemInstance)
	{
		DraggedItemWidget = nullptr;
		ClearPlacementPreview();
		return;
	}

	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
	UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(CursorPos);
	if (!HoveredSlot)
	{
		ClearPlacementPreview();
		return;
	}

	const int32 TargetOriginX = HoveredSlot->GetGridX() - DraggedItemWidget->GetDragGrabCellX();
	const int32 TargetOriginY = HoveredSlot->GetGridY() - DraggedItemWidget->GetDragGrabCellY();
	UpdatePlacementPreview(ItemInstance, TargetOriginX, TargetOriginY);
}

bool UUI_Inventory::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UUI_ItemWidget* SourceItemWidget = nullptr;
	UInventoryItemInstance* ItemInstance = nullptr;
	if (!ExtractDragPayload(InOperation, SourceItemWidget, ItemInstance))
	{
		return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
	}

	UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(InDragDropEvent.GetScreenSpacePosition());
	if (!HoveredSlot)
	{
		ClearPlacementPreview();
		return true;
	}

	const int32 TargetOriginX = HoveredSlot->GetGridX() - SourceItemWidget->GetDragGrabCellX();
	const int32 TargetOriginY = HoveredSlot->GetGridY() - SourceItemWidget->GetDragGrabCellY();
	UpdatePlacementPreview(ItemInstance, TargetOriginX, TargetOriginY);
	return true;
}

void UUI_Inventory::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	ClearPlacementPreview();
}

bool UUI_Inventory::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UUI_ItemWidget* SourceItemWidget = nullptr;
	UInventoryItemInstance* ItemInstance = nullptr;
	if (!ExtractDragPayload(InOperation, SourceItemWidget, ItemInstance))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(InDragDropEvent.GetScreenSpacePosition());
	if (!HoveredSlot)
	{
		ClearPlacementPreview();
		UpdateInventory();
		return false;
	}

	const int32 TargetOriginX = HoveredSlot->GetGridX() - SourceItemWidget->GetDragGrabCellX();
	const int32 TargetOriginY = HoveredSlot->GetGridY() - SourceItemWidget->GetDragGrabCellY();
	const bool bPlaced = TryPlaceDraggedItem(ItemInstance, TargetOriginX, TargetOriginY);
	DraggedItemWidget = nullptr;
	return bPlaced;
}

void UUI_Inventory::NativeDestruct()
{
	if (AddItemButton)
	{
		AddItemButton->OnClicked.RemoveDynamic(this, &UUI_Inventory::OnAddItemButtonClicked);
	}
	if (AddItemComboBox)
	{
		AddItemComboBox->OnSelectionChanged.RemoveDynamic(this, &UUI_Inventory::OnAddItemSelectionChanged);
	}

	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::UpdateInventory);
	}
	
	BoundInventory = nullptr;
	ItemSlots.Empty();
	ActiveTooltip = nullptr;
	DraggedItemWidget = nullptr;
	HoveredItemInstance = nullptr;
	Super::NativeDestruct();
}

void UUI_Inventory::BindInventory(UInventoryComponent* InInventory)
{
	BoundInventory = InInventory;
	
	if (BoundInventory)
	{
		// Keep UI grid dimensions aligned with inventory component settings.
		GridWidth = BoundInventory->GridWidth;
		GridHeight = BoundInventory->GridHeight;

		BoundInventory->OnInventoryChanged.AddDynamic(this, &UUI_Inventory::UpdateInventory);
	}
	
	CreateGrid();
	UpdateInventory();
}

void UUI_Inventory::UpdateInventory()
{
	if (!BoundInventory || !GridPanel)
	{
		return;
	}

	ClearItemHoverPreview();

	for (UUI_ItemSlot* GridSlot : ItemSlots)
	{
		if (GridSlot)
		{
			GridSlot->UnbindItem();
		}
	}

	if (BoundInventory->Items.Num() > 0)
	{
		for (UInventoryItemInstance* Item : BoundInventory->Items)
		{
			if (!Item || Item->SlotX < 0 || Item->SlotY < 0)
			{
				continue;
			}

			int32 Width = Item->Width;
			int32 Height = Item->Height;
			bool bItemWidgetBound = false;

			for (int32 Y = 0; Y < Height; Y++)
			{
				for (int32 X = 0; X < Width; X++)
				{
					bool bIsOccupied = true;
					if (Item->ShapeMask.Width > 0 && Item->ShapeMask.Height > 0)
					{
						bIsOccupied = Item->ShapeMask.IsOccupied(X, Y);
					}

					if (bIsOccupied)
					{
						int32 GridX = Item->SlotX + X;
						int32 GridY = Item->SlotY + Y;

						if (GridX >= 0 && GridX < BoundInventory->GridWidth &&
							GridY >= 0 && GridY < BoundInventory->GridHeight)
						{
							int32 Index = GridY * BoundInventory->GridWidth + GridX;
							if (Index >= 0 && Index < ItemSlots.Num())
							{
								if (!bItemWidgetBound)
								{
									ItemSlots[Index]->BindItem(Item);
									bItemWidgetBound = true;
								}
								else
								{
									ItemSlots[Index]->SetOccupied(true, Item->ItemData->ItemData.Rarity);
								}
							}
						}
					}
				}
			}

		}
	}
}

void UUI_Inventory::CreateGrid()
{
	if (!GridPanel)
	{
		return;
	}
	
	if (!ItemSlotClass)
	{
		return;
	}

	// BindInventory may call CreateGrid before NativeConstruct runs,
	// so make sure ItemWidgetClass is valid here as well.
	if (!ItemWidgetClass)
	{
		ItemWidgetClass = ResolveDefaultItemWidgetClass();
	}

	GridPanel->ClearChildren();
	ItemSlots.Empty();

	int32 GridWidthToUse = GridWidth > 0 ? GridWidth : (BoundInventory ? BoundInventory->GridWidth : 6);
	int32 GridHeightToUse = GridHeight > 0 ? GridHeight : (BoundInventory ? BoundInventory->GridHeight : 10);
	int32 SlotSizeToUse = SlotSize > 0 ? SlotSize : 64;
	int32 SlotSpacingToUse = SlotSpacing >= 0 ? SlotSpacing : 0;

	for (int32 Y = 0; Y < GridHeightToUse; Y++)
	{
		for (int32 X = 0; X < GridWidthToUse; X++)
		{
			UUI_ItemSlot* GridSlot = CreateWidget<UUI_ItemSlot>(GetWorld(), ItemSlotClass);
			if (GridSlot)
			{
				GridSlot->SetSlotPosition(X, Y);
				GridSlot->SetItemWidgetClass(ItemWidgetClass);
				GridSlot->OnSlotClicked.AddDynamic(this, &UUI_Inventory::OnItemSlotClickedInternal);
				GridSlot->OnSlotHovered.AddDynamic(this, &UUI_Inventory::ShowTooltip);
				ItemSlots.Add(GridSlot);
				GridPanel->AddChildToUniformGrid(GridSlot, Y, X);
				GridSlot->SetOwningInventory(this);
			}
		}
	}
	
	SetSlotSize();
}

void UUI_Inventory::OnItemSlotClickedInternal(UUI_ItemSlot* GridSlot)
{
	OnItemSlotClicked.Broadcast(GridSlot);
}

void UUI_Inventory::SetSlotSize()
{
	if (!GridPanel)
	{
		return;
	}
	
	int32 SizeToUse = SlotSize > 0 ? SlotSize : 64;
	int32 SpacingToUse = SlotSpacing >= 0 ? SlotSpacing : 0;
	
	GridPanel->SetMinDesiredSlotWidth(SizeToUse + SpacingToUse);
	GridPanel->SetMinDesiredSlotHeight(SizeToUse + SpacingToUse);
}

void UUI_Inventory::UpdateSlotCanPlacePreviews(UUI_ItemWidget* ItemWidget)
{
	if (!ItemWidget || !BoundInventory || !GridPanel)
	{
		return;
	}

	DraggedItemWidget = ItemWidget;

	for (UUI_ItemSlot* GridSlot : ItemSlots)
	{
		if (GridSlot)
		{
			GridSlot->UpdateCanPlaceFromItem(ItemWidget);
		}
	}
}

void UUI_Inventory::ShowTooltip(UUI_ItemSlot* GridSlot)
{
	if (!GridSlot || !TooltipClass)
	{
		return;
	}

	UInventoryItemInstance* Item = GridSlot->GetBoundItem();
	if (!Item)
	{
		HideTooltip();
		return;
	}

	if (!ActiveTooltip)
	{
		ActiveTooltip = CreateWidget<UUI_ItemTooltip>(GetWorld(), TooltipClass);
	}

	if (ActiveTooltip)
	{
		ActiveTooltip->SetItem(Item);
	}
}

void UUI_Inventory::HideTooltip()
{
	if (ActiveTooltip)
	{
		ActiveTooltip->HideTooltip();
	}
}

void UUI_Inventory::SetDraggedItemWidget(UUI_ItemWidget* InWidget)
{
	DraggedItemWidget = InWidget;
}

UUI_ItemWidget* UUI_Inventory::GetDraggedItemWidget() const
{
	return DraggedItemWidget;
}

UUI_ItemSlot* UUI_Inventory::GetSlotAtGrid(int32 GridX, int32 GridY) const
{
	if (GridX < 0 || GridY < 0)
	{
		return nullptr;
	}

	const int32 EffectiveWidth = BoundInventory ? BoundInventory->GridWidth : GridWidth;
	if (EffectiveWidth <= 0)
	{
		return nullptr;
	}

	const int32 Index = GridY * EffectiveWidth + GridX;
	if (!ItemSlots.IsValidIndex(Index))
	{
		return nullptr;
	}

	return ItemSlots[Index];
}

void UUI_Inventory::UpdatePlacementPreview(UInventoryItemInstance* ItemInstance, int32 OriginSlotX, int32 OriginSlotY)
{
	ClearPlacementPreview();

	if (!BoundInventory || !ItemInstance)
	{
		return;
	}

	const int32 Width = FMath::Max(1, ItemInstance->Width);
	const int32 Height = FMath::Max(1, ItemInstance->Height);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			bool bShouldOccupy = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
			{
				bShouldOccupy = ItemInstance->ShapeMask.IsOccupied(X, Y);
			}
			if (!bShouldOccupy)
			{
				continue;
			}

			const int32 GridX = OriginSlotX + X;
			const int32 GridY = OriginSlotY + Y;
			if (!BoundInventory->IsValidPosition(GridX, GridY))
			{
				continue;
			}

			const int32 Index = GridY * BoundInventory->GridWidth + GridX;
			if (!ItemSlots.IsValidIndex(Index))
			{
				continue;
			}

			const UInventoryItemInstance* ExistingItem = BoundInventory->GetItemAtPosition(GridX, GridY);
			const bool bCellCanPlace = !ExistingItem || ExistingItem == ItemInstance;
			ItemSlots[Index]->SetCanPlace(bCellCanPlace);
		}
	}
}

void UUI_Inventory::ClearPlacementPreview()
{
	for (UUI_ItemSlot* GridSlot : ItemSlots)
	{
		if (GridSlot)
		{
			GridSlot->ClearCanPlacePreview();
		}
	}
}

void UUI_Inventory::SetItemHoverPreview(UInventoryItemInstance* ItemInstance)
{
	if (!BoundInventory || !ItemInstance)
	{
		ClearItemHoverPreview();
		return;
	}

	if (HoveredItemInstance == ItemInstance)
	{
		return;
	}

	ClearItemHoverPreview();
	HoveredItemInstance = ItemInstance;

	const int32 Width = FMath::Max(1, ItemInstance->Width);
	const int32 Height = FMath::Max(1, ItemInstance->Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			bool bShouldHighlight = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
			{
				bShouldHighlight = ItemInstance->ShapeMask.IsOccupied(X, Y);
			}
			if (!bShouldHighlight)
			{
				continue;
			}

			const int32 GridX = ItemInstance->SlotX + X;
			const int32 GridY = ItemInstance->SlotY + Y;
			UUI_ItemSlot* TargetSlot = GetSlotAtGrid(GridX, GridY);
			if (TargetSlot)
			{
				TargetSlot->SetHoverActive(true);
			}
		}
	}
}

void UUI_Inventory::ClearItemHoverPreview(UInventoryItemInstance* ItemInstance)
{
	if (ItemInstance && HoveredItemInstance != ItemInstance)
	{
		return;
	}

	for (UUI_ItemSlot* GridSlot : ItemSlots)
	{
		if (GridSlot)
		{
			GridSlot->SetHoverActive(false);
		}
	}

	HoveredItemInstance = nullptr;
}

bool UUI_Inventory::TryPlaceDraggedItem(UInventoryItemInstance* ItemInstance, int32 OriginSlotX, int32 OriginSlotY)
{
	if (!BoundInventory || !ItemInstance)
	{
		ClearPlacementPreview();
		return false;
	}

	const bool bCanPlace = BoundInventory->IsSpaceAvailable(
		ItemInstance->Width,
		ItemInstance->Height,
		ItemInstance->ShapeMask,
		OriginSlotX,
		OriginSlotY,
		ItemInstance
	);

	bool bMoved = false;
	if (bCanPlace)
	{
		bMoved = BoundInventory->MoveItem(ItemInstance, OriginSlotX, OriginSlotY);
	}

	ClearPlacementPreview();
	if (!bMoved)
	{
		UpdateInventory();
	}
	return bMoved;
}

void UUI_Inventory::AddItemFromDataAsset(class UInventoryItemDataAsset* ItemData, int32 StackSize)
{
	if (!BoundInventory || !ItemData)
	{
		return;
	}

	BoundInventory->AddItem(ItemData, StackSize);
}

TArray<UInventoryItemDataAsset*> UUI_Inventory::GetAllItemDataAssets() const
{
	TArray<UInventoryItemDataAsset*> Result;

	static const FName ItemDataFolderPath(TEXT("/Game/Inventory/InventoryItemDataAsset"));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(ItemDataFolderPath, AssetDataList, true);

	TSet<const UInventoryItemDataAsset*> UniqueAssets;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* RawAsset = AssetData.GetAsset();
		UInventoryItemDataAsset* Asset = Cast<UInventoryItemDataAsset>(RawAsset);

		if (!Asset)
		{
			if (const UBlueprint* BlueprintAsset = Cast<UBlueprint>(RawAsset))
			{
				if (BlueprintAsset->GeneratedClass && BlueprintAsset->GeneratedClass->IsChildOf(UInventoryItemDataAsset::StaticClass()))
				{
					Asset = Cast<UInventoryItemDataAsset>(BlueprintAsset->GeneratedClass->GetDefaultObject());
				}
			}
		}

		if (Asset)
		{
			if (!UniqueAssets.Contains(Asset))
			{
				UniqueAssets.Add(Asset);
				Result.Add(Asset);
			}
		}
		else
		{
			// Kept silent to reduce log noise outside bind probe.
		}
	}

	return Result;
}

TArray<UInventoryItemDataAsset*> UUI_Inventory::GetAvailableItemDataAssets() const
{
	return GetAllItemDataAssets();
}

void UUI_Inventory::CloseInventory()
{
	if (BoundInventory)
	{
		BoundInventory->CloseInventory();
	}
}

void UUI_Inventory::OnAddItemButtonClicked()
{
	if (!AddItemComboBox)
	{
		return;
	}

	RefreshAddItemOptions();
	if (AddItemOptionMap.Num() == 0)
	{
		SetAddItemListVisible(false);
		return;
	}

	const bool bShouldShow = AddItemComboBox->GetVisibility() != ESlateVisibility::Visible;
	SetAddItemListVisible(bShouldShow);
}

void UUI_Inventory::OnAddItemSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	if (TObjectPtr<UInventoryItemDataAsset>* FoundAsset = AddItemOptionMap.Find(SelectedItem))
	{
		if (FoundAsset && *FoundAsset)
		{
			AddItemFromDataAsset(*FoundAsset, 1);
		}
	}

	SetAddItemListVisible(false);
}

void UUI_Inventory::RefreshAddItemOptions()
{
	if (!AddItemComboBox)
	{
		return;
	}

	AddItemOptionMap.Empty();
	AddItemComboBox->ClearOptions();

	TArray<UInventoryItemDataAsset*> ItemAssets = GetAvailableItemDataAssets();
	for (UInventoryItemDataAsset* Asset : ItemAssets)
	{
		if (!Asset)
		{
			continue;
		}

		FString BaseLabel = Asset->ItemData.ItemName.IsEmpty()
			? Asset->GetName()
			: Asset->ItemData.ItemName.ToString();

		FString UniqueLabel = BaseLabel;
		int32 Suffix = 2;
		while (AddItemOptionMap.Contains(UniqueLabel))
		{
			UniqueLabel = FString::Printf(TEXT("%s (%d)"), *BaseLabel, Suffix);
			++Suffix;
		}

		AddItemOptionMap.Add(UniqueLabel, Asset);
		AddItemComboBox->AddOption(UniqueLabel);
	}

}

void UUI_Inventory::SetAddItemListVisible(bool bVisible)
{
	if (!AddItemComboBox)
	{
		return;
	}

	AddItemComboBox->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bVisible)
	{
		AddItemComboBox->ClearSelection();
	}
}

UUI_ItemSlot* UUI_Inventory::FindSlotAtScreenPosition(const FVector2D& ScreenPosition) const
{
	for (UUI_ItemSlot* GridSlot : ItemSlots)
	{
		if (!GridSlot)
		{
			continue;
		}

		if (GridSlot->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			return GridSlot;
		}
	}

	return nullptr;
}