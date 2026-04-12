#include "UI/UI_Inventory.h"
#include "UI/UI_ItemSlot.h"
#include "UI/UI_ItemTooltip.h"
#include "UI/UI_ItemWidget.h"
#include "Components/UniformGridPanel.h"
#include "Inventory/InventoryComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "SlateBasics.h"

void UUI_Inventory::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (GridWidth <= 0)
	{
		GridWidth = 10;
	}
	if (GridHeight <= 0)
	{
		GridHeight = 6;
	}
	if (SlotSize <= 0)
	{
		SlotSize = 64;
	}
	if (SlotSpacing < 0)
	{
		SlotSpacing = 0;
	}
	
	SetSlotSize();
}

FReply UUI_Inventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUI_Inventory::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DraggedItemWidget && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		FVector2D MousePosition = InMouseEvent.GetScreenSpacePosition();
		
		for (UUI_ItemSlot* GridSlot : ItemSlots)
		{
			if (GridSlot)
			{
				GridSlot->SetCanPlace(false);
			}
		}
		
		DraggedItemWidget->RemoveFromParent();
		DraggedItemWidget = nullptr;
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UUI_Inventory::NativeDestruct()
{
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::UpdateInventory);
	}
	
	BoundInventory = nullptr;
	ItemSlots.Empty();
	ActiveTooltip = nullptr;
	DraggedItemWidget = nullptr;
	Super::NativeDestruct();
}

void UUI_Inventory::BindInventory(UInventoryComponent* InInventory)
{
	BoundInventory = InInventory;
	
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.AddDynamic(this, &UUI_Inventory::UpdateInventory);
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] Bound to component - Grid: %d x %d"), BoundInventory->GridWidth, BoundInventory->GridHeight);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] Failed to bind - InInventory is null"));
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

	UE_LOG(LogTemp, Warning, TEXT("[Inventory] Updating inventory with %d items"), BoundInventory->Items.Num());
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] Total slots: %d"), ItemSlots.Num());

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
								if (X == 0 && Y == 0)
								{
									ItemSlots[Index]->BindItem(Item);
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

	GridPanel->ClearChildren();
	ItemSlots.Empty();

	int32 GridWidthToUse = GridWidth > 0 ? GridWidth : (BoundInventory ? BoundInventory->GridWidth : 10);
	int32 GridHeightToUse = GridHeight > 0 ? GridHeight : (BoundInventory ? BoundInventory->GridHeight : 6);
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
				GridSlot->OnSlotClicked.AddDynamic(this, &UUI_Inventory::OnItemSlotClickedInternal);
				GridSlot->OnSlotHovered.AddDynamic(this, &UUI_Inventory::ShowTooltip);
				ItemSlots.Add(GridSlot);
				GridPanel->AddChildToUniformGrid(GridSlot, Y, X);
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

void UUI_Inventory::CloseInventory()
{
	UE_LOG(LogTemp, Warning, TEXT("=== UUI_Inventory::CloseInventory called ==="));
	if (BoundInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("Calling BoundInventory->CloseInventory()"));
		BoundInventory->CloseInventory();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BoundInventory is null!"));
	}
}