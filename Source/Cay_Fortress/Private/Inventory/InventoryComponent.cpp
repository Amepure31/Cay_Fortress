 #include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/FInventoryGridCell.h"
#include "Inventory/FItemShapeMask.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	GridWidth = 6;
	GridHeight = 10;
	bIsInventoryOpen = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (GridWidth <= 0)
	{
		GridWidth = 6;
	}
	if (GridHeight <= 0)
	{
		GridHeight = 10;
	}
	
	InitializeGrid(GridWidth, GridHeight);
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInventoryComponent::InitializeGrid(int32 InWidth, int32 InHeight)
{
	GridWidth = InWidth;
	GridHeight = InHeight;

	Grid.Empty();
	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		for (int32 X = 0; X < GridWidth; X++)
		{
			FFInventoryGridCell Cell;
			Grid.Add(Cell);
		}
	}
}

void UInventoryComponent::Clear()
{
	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		for (int32 X = 0; X < GridWidth; X++)
		{
			int32 Index = GetGridIndex(X, Y);
			if (Index >= 0 && Index < Grid.Num())
			{
				Grid[Index].Clear();
			}
		}
	}

	Items.Empty();
	NotifyInventoryChanged();
}

void UInventoryComponent::OpenInventory()
{
	if (!bIsInventoryOpen)
	{
		bIsInventoryOpen = true;
		OnInventoryToggled.Broadcast(bIsInventoryOpen);
	}
}

void UInventoryComponent::CloseInventory()
{
	if (bIsInventoryOpen)
	{
		bIsInventoryOpen = false;
		OnInventoryToggled.Broadcast(bIsInventoryOpen);
	}
}

void UInventoryComponent::ToggleInventory()
{
	bIsInventoryOpen = !bIsInventoryOpen;
	OnInventoryToggled.Broadcast(bIsInventoryOpen);
}

bool UInventoryComponent::AddItem(class UInventoryItemDataAsset* ItemData, int32 StackSize)
{
	if (!ItemData)
	{
		return false;
	}

	const FInventoryItemData& Data = ItemData->ItemData;

	if (Data.bCanStack)
	{
		for (UInventoryItemInstance* ExistingItem : Items)
		{
			if (ExistingItem && ExistingItem->ItemData == ItemData && ExistingItem->StackSize < Data.MaxStackSize)
			{
				int32 Space = Data.MaxStackSize - ExistingItem->StackSize;
				int32 AddAmount = FMath::Min(StackSize, Space);
				ExistingItem->StackSize += AddAmount;
				NotifyInventoryChanged();
				return true;
			}
		}
	}

	int32 SlotX = -1;
	int32 SlotY = -1;

	if (!FindSpaceForItem(Data.Width, Data.Height, FFItemShapeMask(), SlotX, SlotY))
	{
		return false;
	}

	return AddItemAtPosition(ItemData, SlotX, SlotY, StackSize);
}

bool UInventoryComponent::AddItemAtPosition(class UInventoryItemDataAsset* ItemData, int32 SlotX, int32 SlotY, int32 StackSize)
{
	if (!ItemData)
	{
		return false;
	}

	const FInventoryItemData& Data = ItemData->ItemData;

	if (!IsSpaceAvailable(Data.Width, Data.Height, FFItemShapeMask(), SlotX, SlotY))
	{
		return false;
	}

	UInventoryItemInstance* NewItem = NewObject<UInventoryItemInstance>(this);
	if (!NewItem)
	{
		return false;
	}

	NewItem->SetItemData(ItemData);
	NewItem->StackSize = StackSize;
	NewItem->SlotX = SlotX;
	NewItem->SlotY = SlotY;

	Items.Add(NewItem);
	OccupyGrid(NewItem, SlotX, SlotY);
	NotifyInventoryChanged();

	return true;
}

bool UInventoryComponent::RemoveItem(class UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		return false;
	}

	ReleaseGrid(ItemInstance);
	Items.Remove(ItemInstance);
	ItemInstance->MarkAsGarbage();
	NotifyInventoryChanged();

	return true;
}

class UInventoryItemInstance* UInventoryComponent::RemoveItemAtPosition(int32 SlotX, int32 SlotY)
{
	UInventoryItemInstance* Item = GetItemAtPosition(SlotX, SlotY);
	if (Item)
	{
		RemoveItem(Item);
	}
	return Item;
}

bool UInventoryComponent::MoveItem(class UInventoryItemInstance* ItemInstance, int32 NewSlotX, int32 NewSlotY)
{
	if (!ItemInstance)
	{
		return false;
	}

	ReleaseGrid(ItemInstance);

	if (!IsSpaceAvailable(ItemInstance->Width, ItemInstance->Height, ItemInstance->ShapeMask, NewSlotX, NewSlotY, ItemInstance))
	{
		OccupyGrid(ItemInstance, ItemInstance->SlotX, ItemInstance->SlotY);
		return false;
	}

	ItemInstance->SlotX = NewSlotX;
	ItemInstance->SlotY = NewSlotY;
	OccupyGrid(ItemInstance, NewSlotX, NewSlotY);
	NotifyInventoryChanged();

	return true;
}

bool UInventoryComponent::MoveItemWithShape(
	class UInventoryItemInstance* ItemInstance,
	int32 NewSlotX,
	int32 NewSlotY,
	int32 NewWidth,
	int32 NewHeight,
	const FFItemShapeMask& NewShapeMask,
	int32 NewRotationQuarterTurns)
{
	if (!ItemInstance)
	{
		return false;
	}

	const int32 OldSlotX = ItemInstance->SlotX;
	const int32 OldSlotY = ItemInstance->SlotY;

	ReleaseGrid(ItemInstance);

	const int32 SafeWidth = FMath::Max(1, NewWidth);
	const int32 SafeHeight = FMath::Max(1, NewHeight);
	if (!IsSpaceAvailable(SafeWidth, SafeHeight, NewShapeMask, NewSlotX, NewSlotY, ItemInstance))
	{
		OccupyGrid(ItemInstance, OldSlotX, OldSlotY);
		return false;
	}

	ItemInstance->Width = SafeWidth;
	ItemInstance->Height = SafeHeight;
	ItemInstance->ShapeMask = NewShapeMask;
	ItemInstance->RotationQuarterTurns = ((NewRotationQuarterTurns % 4) + 4) % 4;
	ItemInstance->SlotX = NewSlotX;
	ItemInstance->SlotY = NewSlotY;

	OccupyGrid(ItemInstance, NewSlotX, NewSlotY);
	NotifyInventoryChanged();
	return true;
}

bool UInventoryComponent::IsSpaceAvailable(int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32 SlotX, int32 SlotY, class UInventoryItemInstance* IgnoreItem) const
{
	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			int32 GridX = SlotX + X;
			int32 GridY = SlotY + Y;

			if (!IsValidPosition(GridX, GridY))
			{
				return false;
			}

			bool bShouldOccupy = true;
			if (ShapeMask.Width > 0 && ShapeMask.Height > 0)
			{
				bShouldOccupy = ShapeMask.IsOccupied(X, Y);
			}

			if (bShouldOccupy)
			{
				int32 Index = GetGridIndex(GridX, GridY);
				if (Index >= 0 && Index < Grid.Num())
				{
					const FFInventoryGridCell& Cell = Grid[Index];
					if (Cell.bOccupied && Cell.ItemInstance != IgnoreItem)
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool UInventoryComponent::FindSpaceForItem(int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32& OutSlotX, int32& OutSlotY) const
{
	for (int32 Y = 0; Y <= GridHeight - Height; Y++)
	{
		for (int32 X = 0; X <= GridWidth - Width; X++)
		{
			if (IsSpaceAvailable(Width, Height, ShapeMask, X, Y))
			{
				OutSlotX = X;
				OutSlotY = Y;
				return true;
			}
		}
	}

	return false;
}

class UInventoryItemInstance* UInventoryComponent::GetItemAtPosition(int32 SlotX, int32 SlotY) const
{
	if (!IsValidPosition(SlotX, SlotY))
	{
		return nullptr;
	}

	int32 Index = GetGridIndex(SlotX, SlotY);
	if (Index >= 0 && Index < Grid.Num())
	{
		return Grid[Index].ItemInstance;
	}

	return nullptr;
}

bool UInventoryComponent::IsValidPosition(int32 SlotX, int32 SlotY) const
{
	return SlotX >= 0 && SlotX < GridWidth && SlotY >= 0 && SlotY < GridHeight;
}

void UInventoryComponent::OccupyGrid(class UInventoryItemInstance* ItemInstance, int32 SlotX, int32 SlotY)
{
	if (!ItemInstance)
	{
		return;
	}

	int32 Width = ItemInstance->Width;
	int32 Height = ItemInstance->Height;

	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			bool bShouldOccupy = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
			{
				bShouldOccupy = ItemInstance->ShapeMask.IsOccupied(X, Y);
			}

			if (bShouldOccupy)
			{
				int32 GridX = SlotX + X;
				int32 GridY = SlotY + Y;
				int32 Index = GetGridIndex(GridX, GridY);
				if (Index >= 0 && Index < Grid.Num())
				{
					Grid[Index].SetOccupied(ItemInstance, Width, Height, SlotX, SlotY);
				}
			}
		}
	}
}

void UInventoryComponent::ReleaseGrid(class UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		return;
	}

	int32 Width = ItemInstance->Width;
	int32 Height = ItemInstance->Height;
	int32 SlotX = ItemInstance->SlotX;
	int32 SlotY = ItemInstance->SlotY;

	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			bool bShouldRelease = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
			{
				bShouldRelease = ItemInstance->ShapeMask.IsOccupied(X, Y);
			}

			if (bShouldRelease)
			{
				int32 GridX = SlotX + X;
				int32 GridY = SlotY + Y;
				int32 Index = GetGridIndex(GridX, GridY);
				if (Index >= 0 && Index < Grid.Num() && Grid[Index].ItemInstance == ItemInstance)
				{
					Grid[Index].Clear();
				}
			}
		}
	}
}

int32 UInventoryComponent::GetGridIndex(int32 X, int32 Y) const
{
	return Y * GridWidth + X;
}

void UInventoryComponent::NotifyInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}