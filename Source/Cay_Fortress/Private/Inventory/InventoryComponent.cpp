// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/FInventoryGridCell.h"
#include "Inventory/FItemShapeMask.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryItemRarity.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	GridWidth = 10;
	GridHeight = 6;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

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
			int32 Index = Y * GridWidth + X;
			if (Index >= 0 && Index < Grid.Num())
			{
				Grid[Index].Clear();
			}
		}
	}

	Items.Empty();
}
