 #include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/FInventoryGridCell.h"
#include "Inventory/FItemShapeMask.h"

namespace
{
static FFItemShapeMask MakeFullMask(int32 Width, int32 Height)
{
	FFItemShapeMask Mask;
	Mask.Width = FMath::Max(1, Width);
	Mask.Height = FMath::Max(1, Height);
	Mask.ShapeMaskData.Init(1, Mask.Width * Mask.Height);
	return Mask;
}

static FFItemShapeMask BuildMaskFromItemData(const FInventoryItemData& Data)
{
	const int32 SafeWidth = FMath::Max(1, Data.Width);
	const int32 SafeHeight = FMath::Max(1, Data.Height);
	if (Data.ShapeMaskData.Num() < SafeWidth * SafeHeight)
	{
		return MakeFullMask(SafeWidth, SafeHeight);
	}

	FFItemShapeMask Mask;
	Mask.Width = SafeWidth;
	Mask.Height = SafeHeight;
	Mask.ShapeMaskData = Data.ShapeMaskData;
	return Mask;
}

static FFItemShapeMask RotateMaskClockwise(const FFItemShapeMask& InMask)
{
	const int32 OldWidth = FMath::Max(1, InMask.Width);
	const int32 OldHeight = FMath::Max(1, InMask.Height);
	const int32 NewWidth = OldHeight;
	const int32 NewHeight = OldWidth;

	FFItemShapeMask Rotated;
	Rotated.Width = NewWidth;
	Rotated.Height = NewHeight;
	Rotated.ShapeMaskData.Init(0, NewWidth * NewHeight);

	for (int32 Y = 0; Y < OldHeight; ++Y)
	{
		for (int32 X = 0; X < OldWidth; ++X)
		{
			if (!InMask.IsOccupied(X, Y))
			{
				continue;
			}

			const int32 RotatedX = OldHeight - 1 - Y;
			const int32 RotatedY = X;
			Rotated.ShapeMaskData[RotatedY * NewWidth + RotatedX] = 1;
		}
	}

	return Rotated;
}

static bool IsFullRectangularMask(const FFItemShapeMask& Mask)
{
	const int32 Width = FMath::Max(1, Mask.Width);
	const int32 Height = FMath::Max(1, Mask.Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (!Mask.IsOccupied(X, Y))
			{
				return false;
			}
		}
	}
	return true;
}

static int32 GetRotationAttemptCount(const FFItemShapeMask& Mask)
{
	const int32 Width = FMath::Max(1, Mask.Width);
	const int32 Height = FMath::Max(1, Mask.Height);
	const int32 OccupiedCount = Mask.GetOccupiedCellCount();
	if (Width == 1 && Height == 1 && OccupiedCount <= 1)
	{
		return 1;
	}
	return IsFullRectangularMask(Mask) ? 2 : 4;
}
}

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

	FFItemShapeMask TryMask = BuildMaskFromItemData(Data);
	int32 TryWidth = FMath::Max(1, TryMask.Width);
	int32 TryHeight = FMath::Max(1, TryMask.Height);
	const int32 RotationAttempts = GetRotationAttemptCount(TryMask);

	for (int32 RotationStep = 0; RotationStep < RotationAttempts; ++RotationStep)
	{
		int32 SlotX = -1;
		int32 SlotY = -1;
		if (FindSpaceForItem(TryWidth, TryHeight, TryMask, SlotX, SlotY))
		{
			return AddItemWithShapeAtPosition(
				ItemData,
				SlotX,
				SlotY,
				StackSize,
				TryWidth,
				TryHeight,
				TryMask,
				RotationStep) != nullptr;
		}

		if (RotationStep < RotationAttempts - 1)
		{
			TryMask = RotateMaskClockwise(TryMask);
			TryWidth = FMath::Max(1, TryMask.Width);
			TryHeight = FMath::Max(1, TryMask.Height);
		}
	}

	return false;
}

bool UInventoryComponent::AddItemAtPosition(class UInventoryItemDataAsset* ItemData, int32 SlotX, int32 SlotY, int32 StackSize)
{
	if (!ItemData)
	{
		return false;
	}

	const FInventoryItemData& Data = ItemData->ItemData;
	FFItemShapeMask BaseMask = BuildMaskFromItemData(Data);
	return AddItemWithShapeAtPosition(
		ItemData,
		SlotX,
		SlotY,
		StackSize,
		BaseMask.Width,
		BaseMask.Height,
		BaseMask,
		0) != nullptr;
}

class UInventoryItemInstance* UInventoryComponent::AddItemWithShapeAtPosition(
	class UInventoryItemDataAsset* ItemData,
	int32 SlotX,
	int32 SlotY,
	int32 StackSize,
	int32 Width,
	int32 Height,
	const FFItemShapeMask& ShapeMask,
	int32 RotationQuarterTurns,
	int32 Durability,
	int32 MaxDurability,
	bool bIsBound,
	FDateTime BindTime)
{
	if (!ItemData)
	{
		return nullptr;
	}

	const int32 SafeWidth = FMath::Max(1, Width);
	const int32 SafeHeight = FMath::Max(1, Height);
	if (!IsSpaceAvailable(SafeWidth, SafeHeight, ShapeMask, SlotX, SlotY))
	{
		return nullptr;
	}

	UInventoryItemInstance* NewItem = NewObject<UInventoryItemInstance>(this);
	if (!NewItem)
	{
		return nullptr;
	}

	NewItem->SetItemData(ItemData);
	NewItem->StackSize = FMath::Max(1, StackSize);
	NewItem->Width = SafeWidth;
	NewItem->Height = SafeHeight;
	NewItem->ShapeMask = ShapeMask;
	NewItem->RotationQuarterTurns = ((RotationQuarterTurns % 4) + 4) % 4;
	NewItem->SlotX = SlotX;
	NewItem->SlotY = SlotY;
	NewItem->bIsBound = bIsBound;
	NewItem->BindTime = BindTime;

	if (MaxDurability >= 0)
	{
		NewItem->MaxDurability = MaxDurability;
	}
	if (Durability >= 0)
	{
		NewItem->Durability = Durability;
	}

	Items.Add(NewItem);
	OccupyGrid(NewItem, SlotX, SlotY);
	NotifyInventoryChanged();
	return NewItem;
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