#include "Teleport/TeleportNode.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Persistence/InventoryPersistenceSubsystem.h"
#include "Alex_PlayerCharacter.h"

/** 跨关卡静态变量：目标传送节点ID。DLL 常驻，关卡切换不销毁。 */
FName GPendingTeleportNodeID = NAME_None;

ATeleportNode::ATeleportNode()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	NodeMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetSphereRadius(100.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ArrivalPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ArrivalPoint"));
	ArrivalPoint->SetupAttachment(Root);

	TeleportVisuals = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TeleportVisuals"));
	TeleportVisuals->SetupAttachment(Root);
	TeleportVisuals->bAutoActivate = true;

	InteractText = FText::FromString(TEXT("按E传送"));
}

bool ATeleportNode::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && !TargetLevelName.IsNone() && !TargetTeleportNodeID.IsNone();
}

void ATeleportNode::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	GPendingTeleportNodeID = TargetTeleportNodeID;

	// Capture player inventory before world is destroyed
	if (AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor))
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UInventoryPersistenceSubsystem* Persist = GI->GetSubsystem<UInventoryPersistenceSubsystem>())
			{
				Persist->CaptureFromPlayer(Player);
			}
		}
	}

	//UE_LOG(LogTemp, Log, TEXT("[TeleportNode] 传送: TargetLevel=%s, TargetNode=%s"),
		//*TargetLevelName.ToString(), *TargetTeleportNodeID.ToString());

	const FString MapPath = FString::Printf(TEXT("/Game/Map/%s"), *TargetLevelName.ToString());
	UGameplayStatics::OpenLevel(World, FName(*MapPath));
}

FText ATeleportNode::GetInteractText_Implementation() const
{
	return InteractText;
}
