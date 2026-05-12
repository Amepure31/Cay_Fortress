#include "UI/UI_Equipment.h"
#include "UI/UI_EquipmentSlot.h"
#include "UI/UI_ItemWidget.h"
#include "UI/UI_Inventory.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Logging/LogMacros.h"

namespace
{
static constexpr int32 MinEquipmentCellPixelSize = 64;

static void CollectEquipmentSlotsUnderPanel(UPanelWidget* Panel, TArray<UUI_EquipmentSlot*>& OutSlots)
{
	if (!Panel)
	{
		return;
	}
	for (UPanelSlot* PanelSlot : Panel->GetSlots())
	{
		if (!PanelSlot)
		{
			continue;
		}
		UWidget* Content = PanelSlot->GetContent();
		if (!Content)
		{
			continue;
		}
		if (UUI_EquipmentSlot* EquipSlot = Cast<UUI_EquipmentSlot>(Content))
		{
			OutSlots.Add(EquipSlot);
			continue;
		}
		if (UPanelWidget* ChildPanel = Cast<UPanelWidget>(Content))
		{
			CollectEquipmentSlotsUnderPanel(ChildPanel, OutSlots);
		}
	}
}
}

UUI_Equipment::UUI_Equipment()
	: BackpackSlotHeightCells(6)
	, WeaponSlotMaxWidthCells(8)
	, FallbackEquipmentCellPixelSize(0)
{
}

void UUI_Equipment::NativeConstruct()
{
	Super::NativeConstruct();

	if (BoundEquipment && SlotWidgets.Num() > 0)
	{
		RefreshAllSlots();
	}
}

void UUI_Equipment::NativeDestruct()
{
	if (BoundEquipment)
	{
		BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &UUI_Equipment::OnEquipmentChanged);
	}

	HoveredEquipmentSlot.Reset();
	BoundEquipment = nullptr;
	PlayerInventory = nullptr;
	SyncedInventoryWidget = nullptr;
	SlotWidgets.Empty();
	Super::NativeDestruct();
}

void UUI_Equipment::BindEquipment(UEquipmentComponent* InEquipment, UInventoryComponent* InPlayerInventory, UUI_Inventory* InInventoryUI)
{
	if (BoundEquipment)
	{
		BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &UUI_Equipment::OnEquipmentChanged);
	}

	HoveredEquipmentSlot = nullptr;
	BoundEquipment = InEquipment;
	PlayerInventory = InPlayerInventory;
	SyncedInventoryWidget = InInventoryUI;

	if (BoundEquipment)
	{
		BoundEquipment->OnEquipmentChanged.AddDynamic(this, &UUI_Equipment::OnEquipmentChanged);
	}

	CreateSlots();
	RefreshAllSlots();
}

void UUI_Equipment::SetInventoryUIForCellSync(UUI_Inventory* InInventoryUI)
{
	SyncedInventoryWidget = InInventoryUI;
	RefreshAllSlots();
}

int32 UUI_Equipment::GetEffectiveEquipmentCellPixelSize() const
{
	if (SyncedInventoryWidget.IsValid())
	{
		return FMath::Max(MinEquipmentCellPixelSize, SyncedInventoryWidget->GetEffectiveSlotSize());
	}
	if (FallbackEquipmentCellPixelSize > 0)
	{
		return FMath::Max(MinEquipmentCellPixelSize, FallbackEquipmentCellPixelSize);
	}
	return MinEquipmentCellPixelSize;
}

void UUI_Equipment::GetSlotGridSize(EEquipmentSlotType SlotType, int32& OutWidthCells, int32& OutHeightCells) const
{
	GetEquipmentSlotGridCells(SlotType, BackpackSlotHeightCells, WeaponSlotMaxWidthCells, OutWidthCells, OutHeightCells);
}

void UUI_Equipment::CreateSlots()
{
	SlotWidgets.Empty();

	struct FSlotPair
	{
		UUI_EquipmentSlot* Widget;
		EEquipmentSlotType Type;
	};

	const FSlotPair Pairs[] = {
		{ EquipmentSlot_Head, EEquipmentSlotType::Head },
		{ EquipmentSlot_Chest, EEquipmentSlotType::Chest },
		{ EquipmentSlot_Backpack, EEquipmentSlotType::Backpack },
		{ EquipmentSlot_Weapon1, EEquipmentSlotType::Weapon1 },
		{ EquipmentSlot_Weapon2, EEquipmentSlotType::Weapon2 },
	};

	int32 BoundCount = 0;
	for (const FSlotPair& Pair : Pairs)
	{
		if (!Pair.Widget)
		{
			continue;
		}
		++BoundCount;
		Pair.Widget->SlotType = Pair.Type;
		Pair.Widget->SetOwningEquipmentPanel(this);
		SlotWidgets.Add(Pair.Widget);
		Pair.Widget->ApplySlotChromeAndSize();
		Pair.Widget->OnContainerBackpackRequested.AddDynamic(this, &UUI_Equipment::OnContainerBackpackSlotRequested);
	}

	if (BoundCount == 0)
	{
		if (UPanelWidget* RootPanel = Cast<UPanelWidget>(GetRootWidget()))
		{
			TArray<UUI_EquipmentSlot*> Found;
			CollectEquipmentSlotsUnderPanel(RootPanel, Found);
			TSet<EEquipmentSlotType> SeenTypes;
			for (UUI_EquipmentSlot* SlotWidget : Found)
			{
				if (!SlotWidget)
				{
					continue;
				}
				if (SeenTypes.Contains(SlotWidget->SlotType))
				{
					UE_LOG(LogTemp, Warning, TEXT("UUI_Equipment: duplicate SlotType %d in hierarchy ignored."),
						static_cast<int32>(SlotWidget->SlotType));
					continue;
				}
				SeenTypes.Add(SlotWidget->SlotType);
				SlotWidget->SetOwningEquipmentPanel(this);
				SlotWidgets.Add(SlotWidget);
				SlotWidget->ApplySlotChromeAndSize();
				SlotWidget->OnContainerBackpackRequested.AddDynamic(this, &UUI_Equipment::OnContainerBackpackSlotRequested);
				++BoundCount;
			}
		}

		if (BoundCount == 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UUI_Equipment: bind EquipmentSlot_Head / _Chest / _Backpack / _Weapon1 / _Weapon2, or place UUI_EquipmentSlot under root with SlotType set."));
		}
	}
}

void UUI_Equipment::RefreshAllSlots()
{
	for (UUI_EquipmentSlot* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ApplySlotChromeAndSize();
			SlotWidget->RefreshSlotVisual();
		}
	}
}

void UUI_Equipment::UnequipSlot(EEquipmentSlotType TargetEquipmentSlot)
{
	if (!BoundEquipment || !PlayerInventory)
	{
		return;
	}

	BoundEquipment->UnequipItemToInventory(TargetEquipmentSlot, PlayerInventory.Get());
}

bool UUI_Equipment::TryEquipFromDrag(UUI_ItemWidget* SourceWidget, EEquipmentSlotType TargetEquipmentSlot)
{
	if (!BoundEquipment || !PlayerInventory || !SourceWidget)
	{
		return false;
	}

	UInventoryItemInstance* ItemInstance = SourceWidget->GetItemInstance();
	if (!ItemInstance || !ItemInstance->ItemData)
	{
		return false;
	}

	UUI_Inventory* SourceInventoryWidget = SourceWidget->GetOwningInventory();
	UInventoryComponent* SourceInventory = SourceInventoryWidget
		? SourceInventoryWidget->GetBoundInventory()
		: PlayerInventory.Get();

	if (!SourceInventory)
	{
		return false;
	}

	const bool bEquipped = BoundEquipment->EquipItemFromInventory(SourceInventory, ItemInstance, TargetEquipmentSlot);
	if (bEquipped)
	{
		if (SourceInventoryWidget)
		{
			SourceInventoryWidget->ClearPlacementPreview();
			SourceInventoryWidget->SetDraggedItemWidget(nullptr);
			SourceInventoryWidget->UpdateInventory();
		}
	}

	return bEquipped;
}

void UUI_Equipment::OnEquipmentChanged(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem)
{
	RefreshAllSlots();
}

void UUI_Equipment::OnContainerBackpackSlotRequested(UInventoryItemInstance* ContainerItem)
{
	OnContainerBackpackRequested.Broadcast(ContainerItem);
}

void UUI_Equipment::NotifyEquipmentSlotHover(UUI_EquipmentSlot* EquipmentSlot, bool bHovered)
{
	if (bHovered)
	{
		HoveredEquipmentSlot = EquipmentSlot;
	}
	else if (HoveredEquipmentSlot.Get() == EquipmentSlot)
	{
		HoveredEquipmentSlot = nullptr;
	}
}

UInventoryItemInstance* UUI_Equipment::GetHoveredEquippedContainerItemInstance() const
{
	UUI_EquipmentSlot* HoveredSlotWidget = HoveredEquipmentSlot.Get();
	if (!HoveredSlotWidget || !BoundEquipment)
	{
		return nullptr;
	}

	UInventoryItemInstance* Eq = BoundEquipment->GetEquippedItem(HoveredSlotWidget->SlotType);
	if (Eq && Eq->ItemData && Eq->ItemData->ItemData.ArmorStats.bIsContainer)
	{
		return Eq;
	}
	return nullptr;
}
