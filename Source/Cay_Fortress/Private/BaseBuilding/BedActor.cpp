#include "BaseBuilding/BedActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Alex_PlayerCharacter.h"

ABedActor::ABedActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh"));
	BedMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetSphereRadius(120.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InteractText = FText::FromString(TEXT("按E休息"));
}

bool ABedActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor);
}

void ABedActor::Interact_Implementation(AActor* Interactor)
{
	AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor);
	if (!Player) return;

	Player->SetHealth(Player->MaxHealth);
	Player->SetHunger(Player->MaxHunger);
	Player->SetHydration(Player->MaxHydration);

	BP_OnBedUsed(Interactor);
}

FText ABedActor::GetInteractText_Implementation() const
{
	return InteractText;
}
