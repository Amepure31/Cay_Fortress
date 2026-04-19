// Fill out your copyright notice in the Description page of Project Settings.

#include "LootContainer/LootContainerActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/InventoryComponent.h"

ALootContainerActor::ALootContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	ContainerMesh->SetupAttachment(Root);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	ContainerGridWidth = 6;
	ContainerGridHeight = 5;
	ContainerType = EContainerType::FoodCrate;
}

void ALootContainerActor::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent)
	{
		InventoryComponent->InitializeGrid(
			FMath::Max(1, ContainerGridWidth),
			FMath::Max(1, ContainerGridHeight));
		InventoryComponent->CloseInventory();
	}
}

void ALootContainerActor::OpenContainer()
{
	if (InventoryComponent)
	{
		InventoryComponent->OpenInventory();
	}
}

void ALootContainerActor::CloseContainer()
{
	if (InventoryComponent)
	{
		InventoryComponent->CloseInventory();
	}
}

bool ALootContainerActor::IsContainerOpen() const
{
	return InventoryComponent && InventoryComponent->IsInventoryOpen();
}

bool ALootContainerActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && InventoryComponent != nullptr;
}

void ALootContainerActor::Interact_Implementation(AActor* Interactor)
{
	if (CanInteract_Implementation(Interactor))
	{
		OpenContainer();
	}
}

FText ALootContainerActor::GetInteractText_Implementation() const
{
	return FText::FromString(TEXT("按E打开容器"));
}

