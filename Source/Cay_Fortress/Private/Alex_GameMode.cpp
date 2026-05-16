// Fill out your copyright notice in the Description page of Project Settings.


#include "Alex_GameMode.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"
#include "UObject/SoftObjectPtr.h"
#include "Teleport/TeleportNode.h"
#include "Persistence/InventoryPersistenceSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

extern FName GPendingTeleportNodeID;

AAlex_GameMode::AAlex_GameMode()
{
	static ConstructorHelpers::FClassFinder<AAlex_PlayerCharacter> PlayerPawnBPClass(TEXT("/Game/Characters/Model/BP_Alex_PlayerCharacter.BP_Alex_PlayerCharacter_C"));
	if (PlayerPawnBPClass.Succeeded())
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	
	static ConstructorHelpers::FClassFinder<AAlex_PlayerController> PlayerControllerBPClass(TEXT("/Game/Input/BP_Alex_PlayerController.BP_Alex_PlayerController_C"));
	if (PlayerControllerBPClass.Succeeded())
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
	
}

void AAlex_GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GPendingTeleportNodeID != NAME_None)
	{
		const FName CachedNodeID = GPendingTeleportNodeID;
		GPendingTeleportNodeID = NAME_None;

		FTimerHandle Handle;
		GetWorldTimerManager().SetTimerForNextTick([this, CachedNodeID]()
		{
			HandleTeleportArrival(CachedNodeID);
		});
	}
}

void AAlex_GameMode::HandleTeleportArrival(FName NodeID)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TeleportArrival] 未找到玩家 Pawn，无法移动到节点 '%s'。"), *NodeID.ToString());
		return;
	}

	for (TActorIterator<ATeleportNode> It(GetWorld()); It; ++It)
	{
		ATeleportNode* Node = *It;
		if (Node && Node->TeleportNodeID == NodeID)
		{
			const FVector ArrivalLoc = Node->ArrivalPoint->GetComponentLocation();
			const FRotator ArrivalRot = Node->ArrivalPoint->GetComponentRotation();
			Pawn->SetActorLocationAndRotation(ArrivalLoc, ArrivalRot);

			if (ACharacter* Char = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
				{
					CMC->Velocity = FVector::ZeroVector;
				}
			}

			// Restore inventory & equipment from before the travel
			if (AAlex_PlayerCharacter* PlayerChar = Cast<AAlex_PlayerCharacter>(Pawn))
			{
				if (UGameInstance* GI = GetGameInstance())
				{
					if (UInventoryPersistenceSubsystem* Persist = GI->GetSubsystem<UInventoryPersistenceSubsystem>())
					{
						if (Persist->HasCapturedData())
						{
							Persist->RestoreToPlayer(PlayerChar);
						}
					}
				}
			}

			UE_LOG(LogTemp, Log, TEXT("[TeleportArrival] 玩家已到达节点 '%s'（%s）。"), *NodeID.ToString(), *ArrivalLoc.ToString());
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[TeleportArrival] 未找到 ID 为 '%s' 的传送节点，使用默认出生点。"), *NodeID.ToString());
}
