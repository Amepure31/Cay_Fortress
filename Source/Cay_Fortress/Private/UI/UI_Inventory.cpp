#include "UI/UI_Inventory.h"
#include "UI/UI_ItemSlot.h"
#include "UI/UI_ItemTooltip.h"
#include "UI/UI_ItemWidget.h"
#include "UI/UI_ItemContextMenu.h"
#include "Components/UniformGridPanel.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Equipment/EquipmentComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "Alex_PlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "SlateBasics.h"
#include "Kismet/GameplayStatics.h"
#include "Alex_PlayerController.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"
#include "Misc/PackageName.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetLayoutLibrary.h"

namespace
{
static constexpr int32 MinInventorySlotSize = 64;

static int32 ClampSlotSizeToMinimum(int32 InSlotSize)
{
	return FMath::Max(MinInventorySlotSize, InSlotSize);
}

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

	// Prefer the shared generic widget (WB_UI_Item) first.
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetName != TEXT("WB_UI_Item"))
		{
			continue;
		}

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
		if (LoadedClass && LoadedClass->IsChildOf(UUI_ItemWidget::StaticClass()))
		{
			return LoadedClass;
		}
	}

	// Fallback: any child class in folder.
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

static TSubclassOf<UUI_ItemTooltip> ResolveDefaultTooltipClass()
{
	static const FName TooltipFolderPath(TEXT("/Game/UI/WB_UI_ItemTooltip"));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(TooltipFolderPath, AssetDataList, true);
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
		if (LoadedClass && LoadedClass->IsChildOf(UUI_ItemTooltip::StaticClass()))
		{
			return LoadedClass;
		}
	}

	return nullptr;
}

static bool ExtractDragPayload(UDragDropOperation* InOperation, UUI_ItemWidget*& OutSourceWidget)
{
	OutSourceWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	return OutSourceWidget && OutSourceWidget->GetItemInstance();
}
}

void UUI_Inventory::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);
	SetIsFocusable(true);

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
		SlotSize = MinInventorySlotSize;
	}
	SlotSize = ClampSlotSizeToMinimum(SlotSize);
	if (SlotSpacing < 0)
	{
		SlotSpacing = 0;
	}
	if (!ItemWidgetClass)
	{
		ItemWidgetClass = ResolveDefaultItemWidgetClass();
	}
	if (!TooltipClass)
	{
		TooltipClass = ResolveDefaultTooltipClass();
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
		UpdateDebugButtonVisibility();
	}
}

FReply UUI_Inventory::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::R)
	{
		// Dragging: rotate the dragged item
		if (DraggedItemWidget)
		{
			DraggedItemWidget->RotateDragFootprintClockwise();

			const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
			if (UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(CursorPos))
			{
				const int32 TargetOriginX = HoveredSlot->GetGridX() - DraggedItemWidget->GetDragGrabCellX();
				const int32 TargetOriginY = HoveredSlot->GetGridY() - DraggedItemWidget->GetDragGrabCellY();
				UpdatePlacementPreview(DraggedItemWidget, TargetOriginX, TargetOriginY);
			}
			else
			{
				ClearPlacementPreview();
			}
			return FReply::Handled();
		}

	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UUI_Inventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUI_Inventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DraggedItemWidget && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (TryDiscardDraggedItem(DraggedItemWidget, InGeometry, InMouseEvent.GetScreenSpacePosition()))
		{
			SetDraggedItemWidget(nullptr);
			return FReply::Handled();
		}

		UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(InMouseEvent.GetScreenSpacePosition());
		bool bPlaced = false;
		if (HoveredSlot)
		{
			const int32 TargetOriginX = HoveredSlot->GetGridX() - DraggedItemWidget->GetDragGrabCellX();
			const int32 TargetOriginY = HoveredSlot->GetGridY() - DraggedItemWidget->GetDragGrabCellY();
			bPlaced = TryPlaceDraggedItem(DraggedItemWidget, TargetOriginX, TargetOriginY);
		}
		if (!bPlaced)
		{
			UpdateInventory();
			ClearPlacementPreview();
		}
		SetDraggedItemWidget(nullptr);
		return FReply::Handled();
	}

	ClearPlacementPreview();
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UUI_Inventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DraggedItemWidget)
	{
		// Dragging phase only renders placement preview, tooltip is suppressed.
		HideTooltip();
		ClearItemHoverPreview();

		if (!DraggedItemWidget->GetItemInstance())
		{
			SetDraggedItemWidget(nullptr);
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
		UpdatePlacementPreview(DraggedItemWidget, TargetOriginX, TargetOriginY);
		return;
	}

	// Non-drag phase uses cursor-based hit test every tick for reliable hover.
	UpdateHoverStateFromCursor();

	if (!DraggedItemWidget && BoundInventory)
	{
		APlayerController* PC = GetOwningPlayer();
		const bool bKeyDown = PC && PC->IsInputKeyDown(WeaponUnloadMagazineKey);
		if (bKeyDown)
		{
			if (!bWeaponUnloadKeyHeld)
			{
				bWeaponUnloadKeyHeld = true;
				WeaponUnloadKeyHoldSeconds = 0.f;
				bTriggeredWeaponUnloadThisHold = false;
			}
			WeaponUnloadKeyHoldSeconds += InDeltaTime;
			if (!bTriggeredWeaponUnloadThisHold && WeaponUnloadKeyHoldSeconds >= WeaponUnloadMagazineHoldSeconds)
			{
				if (HoveredItemInstance && HoveredItemInstance->ItemData
					&& HoveredItemInstance->ItemData->ItemData.ItemType == EInventoryItemType::Weapon
					&& HoveredItemInstance->WeaponMagazineAmmo > 0)
				{
					if (BoundInventory->TryUnloadWeaponMagazineToLooseAmmo(HoveredItemInstance))
					{
						UpdateInventory();
					}
				}
				bTriggeredWeaponUnloadThisHold = true;
			}
		}
		else
		{
			bWeaponUnloadKeyHeld = false;
			WeaponUnloadKeyHoldSeconds = 0.f;
			bTriggeredWeaponUnloadThisHold = false;
		}
	}
}

bool UUI_Inventory::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UUI_ItemWidget* SourceItemWidget = nullptr;
	if (!ExtractDragPayload(InOperation, SourceItemWidget))
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
	UpdatePlacementPreview(SourceItemWidget, TargetOriginX, TargetOriginY);
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
	if (!ExtractDragPayload(InOperation, SourceItemWidget))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(InDragDropEvent.GetScreenSpacePosition());
	if (!HoveredSlot)
	{
		if (TryDiscardDraggedItem(SourceItemWidget, InGeometry, InDragDropEvent.GetScreenSpacePosition()))
		{
			SetDraggedItemWidget(nullptr);
			return true;
		}

		ClearPlacementPreview();
		UpdateInventory();
		return false;
	}

	const int32 TargetOriginX = HoveredSlot->GetGridX() - SourceItemWidget->GetDragGrabCellX();
	const int32 TargetOriginY = HoveredSlot->GetGridY() - SourceItemWidget->GetDragGrabCellY();
	const bool bPlaced = TryPlaceDraggedItem(SourceItemWidget, TargetOriginX, TargetOriginY);
	SetDraggedItemWidget(nullptr);
	return bPlaced;
}

void UUI_Inventory::NativeDestruct()
{
	UUI_ItemContextMenu::CloseActiveMenu();

	// Tooltip is added directly to viewport, so it must be explicitly removed
	// when inventory UI is closed/destroyed.
	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}
	ClearItemHoverPreview();
	HideTooltip();

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
	SetDraggedItemWidget(nullptr);
	HoveredItemInstance = nullptr;
	bWeaponUnloadKeyHeld = false;
	WeaponUnloadKeyHoldSeconds = 0.f;
	bTriggeredWeaponUnloadThisHold = false;
	Super::NativeDestruct();
}

void UUI_Inventory::BindInventory(UInventoryComponent* InInventory)
{
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::UpdateInventory);
	}

	BoundInventory = InInventory;
	bHasLastHoverCursorPosition = false;
	
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

			const int32 Width = Item->Width;
			const int32 Height = Item->Height;

			// Always bind the visual widget to the logical item origin.
			// For irregular shapes, (0,0) can be empty after rotation; binding to "first occupied"
			// causes one-cell drift between visual, occupancy and interaction.
			const UInventoryItemInstance* const ItemConst = Item;
			const bool bOriginOccupied =
				(ItemConst->ShapeMask.Width <= 0 || ItemConst->ShapeMask.Height <= 0) ||
				ItemConst->ShapeMask.IsOccupied(0, 0);

			const int32 OriginGridX = Item->SlotX;
			const int32 OriginGridY = Item->SlotY;
			if (OriginGridX >= 0 && OriginGridX < BoundInventory->GridWidth &&
				OriginGridY >= 0 && OriginGridY < BoundInventory->GridHeight)
			{
				const int32 OriginIndex = OriginGridY * BoundInventory->GridWidth + OriginGridX;
				if (OriginIndex >= 0 && OriginIndex < ItemSlots.Num())
				{
					ItemSlots[OriginIndex]->BindItem(Item);
				}
			}

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
								const bool bIsOriginCell = (GridX == OriginGridX && GridY == OriginGridY);
								if (!bIsOriginCell || !bOriginOccupied)
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
	int32 SlotSizeToUse = SlotSize > 0 ? SlotSize : MinInventorySlotSize;
	SlotSizeToUse = ClampSlotSizeToMinimum(SlotSizeToUse);
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
				GridSlot->OnSlotRightClicked.AddDynamic(this, &UUI_Inventory::OnItemSlotRightClickedInternal);
				GridSlot->OnSlotHovered.AddDynamic(this, &UUI_Inventory::ShowTooltip);
				ItemSlots.Add(GridSlot);
				GridPanel->AddChildToUniformGrid(GridSlot, Y, X);
				GridSlot->SetOwningInventory(this);
			}
		}
	}
	
	SetSlotSize();
}

void UUI_Inventory::OnItemSlotClickedInternal(UUI_ItemSlot* ClickedSlot)
{
	OnItemSlotClicked.Broadcast(ClickedSlot);
}

void UUI_Inventory::OnItemSlotRightClickedInternal(UUI_ItemSlot* ClickedSlot, FVector2D ScreenPosition)
{
	if (!ContextMenuClass || !ClickedSlot) return;
	UInventoryItemInstance* Item = ClickedSlot->GetBoundItem();
	if (!Item) return;

	HideTooltip();

	UUI_ItemContextMenu* Menu = CreateWidget<UUI_ItemContextMenu>(this, ContextMenuClass);
	if (Menu)
		Menu->Show(this, Item, ScreenPosition);
}

void UUI_Inventory::RequestEquipItem(UInventoryItemInstance* Item)
{
	if (!BoundInventory || !Item || !Item->ItemData) return;

	const EInventoryItemType ItemType = Item->ItemData->ItemData.ItemType;
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;
	UEquipmentComponent* EquipComp = Pawn->FindComponentByClass<UEquipmentComponent>();
	if (!EquipComp) return;

	if (ItemType == EInventoryItemType::Weapon)
	{
		EEquipmentSlotType TargetSlot = EquipComp->IsSlotOccupied(EEquipmentSlotType::Weapon1)
			? EEquipmentSlotType::Weapon2 : EEquipmentSlotType::Weapon1;
		EquipComp->EquipItemFromInventory(BoundInventory, Item, TargetSlot);
	}
	else if (ItemType == EInventoryItemType::Armor)
	{
		const EArmorEquipSlot AE = Item->ItemData->ItemData.ArmorStats.EquipSlot;
		EEquipmentSlotType ArmorSlot;
		switch (AE)
		{
		case EArmorEquipSlot::Head:     ArmorSlot = EEquipmentSlotType::Head; break;
		case EArmorEquipSlot::Chest:    ArmorSlot = EEquipmentSlotType::Chest; break;
		case EArmorEquipSlot::Backpack: ArmorSlot = EEquipmentSlotType::Backpack; break;
		default: ArmorSlot = EEquipmentSlotType::Chest; break;
		}
		EquipComp->EquipItemFromInventory(BoundInventory, Item, ArmorSlot);
	}

	UpdateInventory();
}

void UUI_Inventory::RequestConsumeFood(UInventoryItemInstance* Item)
{
	if (!BoundInventory || !Item || !Item->ItemData) return;
	if (Item->ItemData->ItemData.ItemType != EInventoryItemType::Food) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(PC->GetPawn());
	if (!Player) return;

	const FFoodItemStats& Food = Item->ItemData->ItemData.FoodStats;
	if (Food.HungerRestore > 0.0f)
		Player->SetHunger(Player->GetHunger() + Food.HungerRestore);
	if (Food.HydrationRestore > 0.0f)
		Player->SetHydration(Player->GetHydration() + Food.HydrationRestore);

	if (Item->StackSize > 1)
	{
		Item->StackSize--;
	}
	else
	{
		BoundInventory->RemoveItem(Item);
	}

	UpdateInventory();
}

void UUI_Inventory::RequestConsumeMedical(UInventoryItemInstance* Item)
{
	if (!BoundInventory || !Item || !Item->ItemData) return;
	if (Item->ItemData->ItemData.ItemType != EInventoryItemType::Medical) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(PC->GetPawn());
	if (!Player) return;

	const FMedicalItemStats& Med = Item->ItemData->ItemData.MedicalStats;
	if (Med.TotalRecoveryAmount > 0.0f)
		Player->SetHealth(Player->GetHealth() + Med.TotalRecoveryAmount);

	if (Item->StackSize > 1)
	{
		Item->StackSize--;
	}
	else
	{
		BoundInventory->RemoveItem(Item);
	}

	UpdateInventory();
}

void UUI_Inventory::RequestDiscardItem(UInventoryItemInstance* Item)
{
	if (!BoundInventory || !Item) return;
	BoundInventory->RemoveItem(Item);
	UpdateInventory();
}

void UUI_Inventory::SetSlotSize()
{
	if (!GridPanel)
	{
		return;
	}
	
	int32 SizeToUse = SlotSize > 0 ? SlotSize : MinInventorySlotSize;
	SizeToUse = ClampSlotSizeToMinimum(SizeToUse);
	int32 SpacingToUse = SlotSpacing >= 0 ? SlotSpacing : 0;
	
	GridPanel->SetMinDesiredSlotWidth(SizeToUse + SpacingToUse);
	GridPanel->SetMinDesiredSlotHeight(SizeToUse + SpacingToUse);
	UpdateGridPanelSize();
}

void UUI_Inventory::UpdateGridPanelSize()
{
	if (!GridPanel)
	{
		return;
	}

	const int32 WidthToUse = FMath::Max(1, BoundInventory ? BoundInventory->GridWidth : GridWidth);
	const int32 HeightToUse = FMath::Max(1, BoundInventory ? BoundInventory->GridHeight : GridHeight);
	const int32 SizeToUse = ClampSlotSizeToMinimum(SlotSize > 0 ? SlotSize : MinInventorySlotSize);
	const int32 SpacingToUse = FMath::Max(0, SlotSpacing);
	const int32 CellPixelSize = SizeToUse + SpacingToUse;

	const FVector2D PanelPixelSize(
		static_cast<float>(WidthToUse * CellPixelSize),
		static_cast<float>(HeightToUse * CellPixelSize));

	// If GridPanel is inside a SizeBox, drive exact panel footprint through overrides.
	if (USizeBox* ParentSizeBox = Cast<USizeBox>(GridPanel->GetParent()))
	{
		ParentSizeBox->SetWidthOverride(PanelPixelSize.X);
		ParentSizeBox->SetHeightOverride(PanelPixelSize.Y);
	}

	// If GridPanel uses a Canvas slot, force explicit pixel size.
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GridPanel->Slot))
	{
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(PanelPixelSize);
	}
}

int32 UUI_Inventory::GetEffectiveSlotSize() const
{
	const int32 SizeToUse = SlotSize > 0 ? SlotSize : MinInventorySlotSize;
	return ClampSlotSizeToMinimum(SizeToUse);
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

void UUI_Inventory::ShowTooltip(UUI_ItemSlot* HoveredSlot)
{
	if (!HoveredSlot)
	{
		return;
	}

	UInventoryItemInstance* ItemToShow = HoveredSlot->GetBoundItem();
	if (!ItemToShow && BoundInventory)
	{
		ItemToShow = BoundInventory->GetItemAtPosition(HoveredSlot->GetGridX(), HoveredSlot->GetGridY());
	}

	ShowTooltipForItem(ItemToShow);
}

void UUI_Inventory::HideTooltip()
{
	if (ActiveTooltip)
	{
		ActiveTooltip->HideTooltip();
	}
}

void UUI_Inventory::UpdateTooltipPosition()
{
	if (!ActiveTooltip)
	{
		return;
	}

	FVector2D MousePosition = FVector2D::ZeroVector;
	if (!UWidgetLayoutLibrary::GetMousePositionScaledByDPI(GetOwningPlayer(), MousePosition.X, MousePosition.Y))
	{
		return;
	}

	MousePosition += FVector2D(20.0f, 20.0f);
	ActiveTooltip->SetPositionInViewport(MousePosition, false);
}

bool UUI_Inventory::IsDropInDiscardZone(const FGeometry& InventoryGeometry, const FVector2D& ScreenPosition) const
{
	const FVector2D LocalPos = InventoryGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D LocalSize = InventoryGeometry.GetLocalSize();
	const bool bWithinX = LocalPos.X >= 0.0f && LocalPos.X <= LocalSize.X;
	const bool bBelow = LocalPos.Y > LocalSize.Y;
	return bWithinX && bBelow;
}

bool UUI_Inventory::TryDiscardDraggedItem(UUI_ItemWidget* ItemWidget, const FGeometry& InventoryGeometry, const FVector2D& ScreenPosition)
{
	if (!ItemWidget || !IsDropInDiscardZone(InventoryGeometry, ScreenPosition))
	{
		return false;
	}

	UInventoryItemInstance* ItemInstance = ItemWidget->GetItemInstance();
	if (!ItemInstance)
	{
		return false;
	}

	UUI_Inventory* SourceInventoryWidget = ItemWidget->GetOwningInventory();
	UInventoryComponent* SourceInventory = SourceInventoryWidget ? SourceInventoryWidget->GetBoundInventory() : BoundInventory;
	if (!SourceInventory)
	{
		return false;
	}

	const bool bRemoved = SourceInventory->RemoveItem(ItemInstance);
	if (!bRemoved)
	{
		return false;
	}

	ClearPlacementPreview();
	UpdateInventory();
	if (SourceInventoryWidget && SourceInventoryWidget != this)
	{
		SourceInventoryWidget->ClearPlacementPreview();
		SourceInventoryWidget->SetDraggedItemWidget(nullptr);
		SourceInventoryWidget->UpdateInventory();
	}

	return true;
}

void UUI_Inventory::SetDraggedItemWidget(UUI_ItemWidget* InWidget)
{
	if (DraggedItemWidget && DraggedItemWidget != InWidget)
	{
		DraggedItemWidget->EndDragSession();
	}

	DraggedItemWidget = InWidget;
	if (DraggedItemWidget)
	{
		bWeaponUnloadKeyHeld = false;
		WeaponUnloadKeyHoldSeconds = 0.f;
		bTriggeredWeaponUnloadThisHold = false;
		SetKeyboardFocus();
		SuppressDraggedItemSlotVisuals(DraggedItemWidget->GetItemInstance());
	}
	else
	{
		ClearPlacementPreview();
	}
}

UUI_ItemWidget* UUI_Inventory::GetDraggedItemWidget() const
{
	return DraggedItemWidget;
}

void UUI_Inventory::SuppressDraggedItemSlotVisuals(UInventoryItemInstance* ItemInstance)
{
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
			bool bShouldAffect = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
			{
				bShouldAffect = ItemInstance->ShapeMask.IsOccupied(X, Y);
			}
			if (!bShouldAffect)
			{
				continue;
			}

			const int32 GridX = ItemInstance->SlotX + X;
			const int32 GridY = ItemInstance->SlotY + Y;
			UUI_ItemSlot* GridSlot = GetSlotAtGrid(GridX, GridY);
			if (!GridSlot)
			{
				continue;
			}

			// During drag, hide source occupancy tint so only drag visual remains.
			GridSlot->SetOccupied(false);
		}
	}
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

void UUI_Inventory::UpdatePlacementPreview(UUI_ItemWidget* ItemWidget, int32 OriginSlotX, int32 OriginSlotY)
{
	ClearPlacementPreview();

	if (!BoundInventory || !ItemWidget)
	{
		return;
	}

	UInventoryItemInstance* ItemInstance = ItemWidget->GetItemInstance();
	if (!ItemInstance)
	{
		return;
	}

	const int32 Width = FMath::Max(1, ItemWidget->GetCurrentDragWidth());
	const int32 Height = FMath::Max(1, ItemWidget->GetCurrentDragHeight());
	const FFItemShapeMask& ShapeMask = ItemWidget->GetCurrentDragShapeMask();

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			bool bShouldOccupy = true;
			if (ShapeMask.Width > 0 && ShapeMask.Height > 0)
			{
				bShouldOccupy = ShapeMask.IsOccupied(X, Y);
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
		UpdateTooltipPosition();
		return;
	}

	ClearItemHoverPreview();
	HoveredItemInstance = ItemInstance;
	ShowTooltipForItem(ItemInstance);

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
	HideTooltip();
}

void UUI_Inventory::ShowTooltipForItem(UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		HideTooltip();
		return;
	}

	if (!TooltipClass)
	{
		TooltipClass = ResolveDefaultTooltipClass();
	}

	if (!TooltipClass)
	{
		return;
	}

	if (!ActiveTooltip)
	{
		ActiveTooltip = CreateWidget<UUI_ItemTooltip>(GetWorld(), TooltipClass);
		if (ActiveTooltip)
		{
			ActiveTooltip->AddToViewport(1000);
		}
	}

	if (ActiveTooltip)
	{
		ActiveTooltip->SetItem(ItemInstance);
		UpdateTooltipPosition();
	}
}

void UUI_Inventory::UpdateHoverStateFromCursor()
{
	if (!BoundInventory)
	{
		ClearItemHoverPreview();
		HideTooltip();
		bHasLastHoverCursorPosition = false;
		return;
	}

	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
	if (bHasLastHoverCursorPosition && CursorPos.Equals(LastHoverCursorPosition, 0.1f))
	{
		if (ActiveTooltip && ActiveTooltip->GetVisibility() == ESlateVisibility::Visible)
		{
			UpdateTooltipPosition();
		}
		return;
	}

	LastHoverCursorPosition = CursorPos;
	bHasLastHoverCursorPosition = true;

	UUI_ItemSlot* HoveredSlot = FindSlotAtScreenPosition(CursorPos);
	if (!HoveredSlot)
	{
		ClearItemHoverPreview();
		HideTooltip();
		return;
	}

	UInventoryItemInstance* HoveredItem = HoveredSlot->GetBoundItem();
	if (!HoveredItem)
	{
		HoveredItem = BoundInventory->GetItemAtPosition(HoveredSlot->GetGridX(), HoveredSlot->GetGridY());
	}

	if (!HoveredItem)
	{
		ClearItemHoverPreview();
		HideTooltip();
		return;
	}

	SetItemHoverPreview(HoveredItem);
	ShowTooltipForItem(HoveredItem);
	UpdateTooltipPosition();
}

bool UUI_Inventory::TryPlaceDraggedItem(UUI_ItemWidget* ItemWidget, int32 OriginSlotX, int32 OriginSlotY)
{
	if (!BoundInventory || !ItemWidget)
	{
		ClearPlacementPreview();
		return false;
	}

	UInventoryItemInstance* ItemInstance = ItemWidget->GetItemInstance();
	if (!ItemInstance)
	{
		ClearPlacementPreview();
		return false;
	}

	const int32 NewWidth = FMath::Max(1, ItemWidget->GetCurrentDragWidth());
	const int32 NewHeight = FMath::Max(1, ItemWidget->GetCurrentDragHeight());
	const FFItemShapeMask& NewShapeMask = ItemWidget->GetCurrentDragShapeMask();
	const int32 NewRotationQuarterTurns = ItemWidget->GetCurrentDragRotationQuarterTurns();

	UUI_Inventory* const SourceInventoryWidget = ItemWidget->GetOwningInventory();
	UInventoryComponent* const SourceInventory = SourceInventoryWidget ? SourceInventoryWidget->GetBoundInventory() : nullptr;

	// Cross-inventory transfer: remove from source inventory, add to target inventory, then apply drag footprint.
	if (SourceInventory && SourceInventory != BoundInventory)
	{
		if (!ItemInstance->ItemData)
		{
			ClearPlacementPreview();
			return false;
		}

		const TObjectPtr<UInventoryItemDataAsset> ItemDataAsset = ItemInstance->ItemData;
		const int32 StackSize = FMath::Max(1, ItemInstance->StackSize);
		UInventoryItemInstance* AddedItem = BoundInventory->AddItemWithShapeAtPosition(
			ItemDataAsset,
			OriginSlotX,
			OriginSlotY,
			StackSize,
			NewWidth,
			NewHeight,
			NewShapeMask,
			NewRotationQuarterTurns,
			ItemInstance->Durability,
			ItemInstance->MaxDurability,
			ItemInstance->bIsBound,
			ItemInstance->BindTime);
		if (!AddedItem)
		{
			ClearPlacementPreview();
			if (SourceInventoryWidget)
			{
				SourceInventoryWidget->ClearPlacementPreview();
				SourceInventoryWidget->UpdateInventory();
			}
			UpdateInventory();
			return false;
		}

		if (!SourceInventory->RemoveItem(ItemInstance))
		{
			BoundInventory->RemoveItem(AddedItem);
			ClearPlacementPreview();
			if (SourceInventoryWidget)
			{
				SourceInventoryWidget->ClearPlacementPreview();
				SourceInventoryWidget->SetDraggedItemWidget(nullptr);
				SourceInventoryWidget->UpdateInventory();
			}
			UpdateInventory();
			return false;
		}

		ItemWidget->SetOwningInventory(this);
		ClearPlacementPreview();
		if (SourceInventoryWidget)
		{
			SourceInventoryWidget->ClearPlacementPreview();
			SourceInventoryWidget->SetDraggedItemWidget(nullptr);
			SourceInventoryWidget->UpdateInventory();
		}
		UpdateInventory();
		return true;
	}

	const bool bMoved = BoundInventory->MoveItemWithShape(
		ItemInstance,
		OriginSlotX,
		OriginSlotY,
		NewWidth,
		NewHeight,
		NewShapeMask,
		NewRotationQuarterTurns
	);

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

	BoundInventory->AddItemFromInventorySpawnerButton(ItemData, StackSize);
}

TArray<UInventoryItemDataAsset*> UUI_Inventory::GetAllItemDataAssets() const
{
	TArray<UInventoryItemDataAsset*> Result;

	static const FName ItemDataFolderPath_UIInventory(TEXT("/Game/Inventory/InventoryItemDataAsset"));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(ItemDataFolderPath_UIInventory, AssetDataList, true);

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
	bWeaponUnloadKeyHeld = false;
	WeaponUnloadKeyHoldSeconds = 0.f;
	bTriggeredWeaponUnloadThisHold = false;

	// Close context menu if open
	UUI_ItemContextMenu::CloseActiveMenu();

	// Ensure hover preview/tooltip never lingers after closing inventory.
	ClearItemHoverPreview();
	HideTooltip();
	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}

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
}

void UUI_Inventory::UpdateDebugButtonVisibility()
{
	bool bDebug = false;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AAlex_PlayerController* APC = Cast<AAlex_PlayerController>(PC))
			bDebug = APC->IsDebugUIVisible();
	}
	ESlateVisibility Vis = bDebug ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (AddItemButton)
		AddItemButton->SetVisibility(Vis);
	if (AddItemComboBox && !bDebug)
		AddItemComboBox->SetVisibility(ESlateVisibility::Collapsed);
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