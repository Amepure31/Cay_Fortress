#include "UI/UI_ItemSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Inventory/InventoryItemRarity.h"
#include "UI/UI_ItemWidget.h"
#include "UI/UI_Inventory.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Input/Reply.h"
#include "SlateBasics.h"
#include "Blueprint/UserWidget.h"

void UUI_ItemSlot::NativeConstruct()
{
	Super::NativeConstruct();
	GridX = -1;
	GridY = -1;
	bCanPlaceItem = false;
	bIsOccupied = false;

	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

FReply UUI_ItemSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	OnSlotClicked.Broadcast(this);
	
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && BoundItem && BoundItem->ItemData)
	{
		if (UUI_Inventory* InventoryWidget = Cast<UUI_Inventory>(GetRootWidget()))
		{
			if (UUI_ItemWidget* ItemWidget = CreateWidget<UUI_ItemWidget>(InventoryWidget->GetWorld(), UUI_ItemWidget::StaticClass()))
			{
				ItemWidget->SetItemInstance(BoundItem);
				InventoryWidget->SetDraggedItemWidget(ItemWidget);
				ItemWidget->AddToViewport();
				TSharedPtr<SWidget> WidgetPtr = ItemWidget->GetCachedWidget();
				if (WidgetPtr.IsValid())
				{
					TSharedRef<SWidget> WidgetRef = WidgetPtr.ToSharedRef();
					return FReply::Handled().DetectDrag(WidgetRef, EKeys::LeftMouseButton);
				}
			}
		}
	}
	
	return FReply::Handled();
}

void UUI_ItemSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(ESlateVisibility::Visible);
	}
	OnSlotHovered.Broadcast(this);
}

void UUI_ItemSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::SetSlotPosition(int32 InGridX, int32 InGridY)
{
	GridX = InGridX;
	GridY = InGridY;
}

void UUI_ItemSlot::GetSlotPosition(int32& OutGridX, int32& OutGridY) const
{
	OutGridX = GridX;
	OutGridY = GridY;
}

void UUI_ItemSlot::SetCanPlace(bool bCanPlace)
{
	bCanPlaceItem = bCanPlace;

	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(bCanPlace ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(!bCanPlace ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::UpdateCanPlaceFromItem(UUI_ItemWidget* ItemWidget)
{
	if (!ItemWidget)
	{
		SetCanPlace(false);
		return;
	}

	UInventoryItemInstance* DraggableItem = ItemWidget->GetItemInstance();
	if (!DraggableItem)
	{
		SetCanPlace(false);
		return;
	}

	if (!BoundItem)
	{
		SetCanPlace(true);
		return;
	}

	int32 SlotX = DraggableItem->SlotX;
	int32 SlotY = DraggableItem->SlotY;

	int32 DraggableWidth = DraggableItem->Width;
	int32 DraggableHeight = DraggableItem->Height;

	int32 ItemGridX = GridX - SlotX;
	int32 ItemGridY = GridY - SlotY;

	bool bCanPlace = true;

	if (ItemGridX < 0 || ItemGridX >= DraggableWidth ||
		ItemGridY < 0 || ItemGridY >= DraggableHeight)
	{
		bCanPlace = false;
	}

	if (bCanPlace && DraggableItem->ShapeMask.Width > 0 && DraggableItem->ShapeMask.Height > 0)
	{
		if (!DraggableItem->ShapeMask.IsOccupied(ItemGridX, ItemGridY))
		{
			bCanPlace = false;
		}
	}

	SetCanPlace(bCanPlace);
}

void UUI_ItemSlot::SetOccupied(bool bOccupied, EInventoryItemRarity Rarity)
{
	bIsOccupied = bOccupied;

	if (Background)
	{
		if (bOccupied)
		{
			Background->SetColorAndOpacity(GetRarityColor(Rarity));
		}
		else
		{
			Background->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f));
		}
	}

	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::BindItem(UInventoryItemInstance* InItemInstance)
{
	BoundItem = InItemInstance;
	if (InItemInstance && InItemInstance->ItemData)
	{
		SetOccupied(true, InItemInstance->ItemData->ItemData.Rarity);
	}
}

void UUI_ItemSlot::UnbindItem()
{
	BoundItem = nullptr;
	SetOccupied(false);
}

FLinearColor UUI_ItemSlot::GetRarityColor(EInventoryItemRarity Rarity) const
{
	switch (Rarity)
	{
	case EInventoryItemRarity::Normal:
		return FLinearColor(0.4f, 0.4f, 0.4f);
	case EInventoryItemRarity::Uncommon:
		return FLinearColor(0.0f, 0.6f, 0.0f);
	case EInventoryItemRarity::Rare:
		return FLinearColor(0.0f, 0.3f, 0.8f);
	case EInventoryItemRarity::Epic:
		return FLinearColor(0.5f, 0.2f, 0.8f);
	case EInventoryItemRarity::Legendary:
		return FLinearColor(0.9f, 0.5f, 0.0f);
	default:
		return FLinearColor(0.4f, 0.4f, 0.4f);
	}
}