#include "Alex_PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Alex_PlayerCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Interaction/PlayerInteractComponent.h"
#include "LootContainer/LootContainerActor.h"
#include "UI/UI_Inventory.h"
#include "UI/UI_LootContainer.h"
#include "UI/UI_Equipment.h"
#include "UI/UI_AmmoHUD.h"
#include "Equipment/EquipmentComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/EnemyCharacter.h"
#include "UI/UI_AimPoint.h"
#include "UI/UI_CharacterProperty.h"
#include "UI/UI_DamageNumber.h"
#include "UI/UI_ContainerBackpack.h"
#include "UI/UI_Minimap.h"
#include "MinimapCaptureActor.h"
#include "UI/UI_LootProgressBar.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemRarity.h"
#include "BaseBuilding/StorageCabinetActor.h"
#include "BaseBuilding/TrainingMachineActor.h"
#include "BaseBuilding/WeaponWorkbenchActor.h"
#include "UI/UI_TrainingMachine.h"
#include "UI/UI_WeaponWorkbench.h"
#include "UI/UI_StorageCabinet.h"

AAlex_PlayerController::AAlex_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	InventoryWidget = nullptr;
	EquipmentWidget = nullptr;
	LootContainerWidget = nullptr;
	AimPointWidget = nullptr;
	AmmoHudWidget = nullptr;
	InventoryComponent = nullptr;
	EquipmentComponent = nullptr;
	PlayerInteractComponent = nullptr;
	bLootInteractionActive = false;
	bInventoryOpenedByLootInteraction = false;
	LootContainerOpenTime = 0.0f;
	bIsLootProgressActive = false;
	bLootRunSpeedUp = false;
	LootProgressElapsed = 0.0f;
	LootProgressWidget = nullptr;
	bIsToggling = false;
	LastToggleTime = 0.0f;
	ContainerBackpackWidget = nullptr;
	bContainerBackpackActive = false;
	bInventoryOpenedByContainerBackpack = false;
	UIUpdateThrottle = 0.f;
	bToggleCooldown = false;
	StorageCabinetWidget = nullptr;
	TrainingMachineWidget = nullptr;
	WeaponWorkbenchWidget = nullptr;
	bStorageCabinetActive = false;
	bInventoryOpenedByCabinet = false;
	bTrainingMachineActive = false;
	bInventoryOpenedByTrainingMachine = false;
	bWeaponWorkbenchActive = false;
	bInventoryOpenedByWeaponWorkbench = false;
}

void AAlex_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	AttachToPawnInventoryAndEquipment(GetPawn());
	EnsureAimPointWidgetCreated();
	EnsureAmmoHudWidgetCreated();
	EnsureCharacterPropertyWidgetCreated();
	EnsureMinimapWidgetCreated();
}

void AAlex_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AttachToPawnInventoryAndEquipment(InPawn);
	EnsureAimPointWidgetCreated();
	EnsureAmmoHudWidgetCreated();
	EnsureCharacterPropertyWidgetCreated();
	EnsureMinimapWidgetCreated();
}

void AAlex_PlayerController::OnUnPossess()
{
	AttachToPawnInventoryAndEquipment(nullptr);
	Super::OnUnPossess();
}

void AAlex_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAlex_PlayerController::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAlex_PlayerController::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AAlex_PlayerController::Jump);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Started, this, &AAlex_PlayerController::Run);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &AAlex_PlayerController::StopRun);
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AAlex_PlayerController::AimStarted);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AAlex_PlayerController::AimStopped);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &AAlex_PlayerController::AimStopped);
		}
		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AAlex_PlayerController::AttackPressed);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AAlex_PlayerController::AttackReleased);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, this, &AAlex_PlayerController::AttackReleased);
		}
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAlex_PlayerController::InteractStarted);
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AAlex_PlayerController::InteractCompleted);
		}
		if (InventoryAction)
			EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ToggleInventory);
		if (ReloadAction)
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ReloadPressed);
		if (DodgeAction)
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AAlex_PlayerController::Dodge);
		if (MinimapAction)
			EnhancedInputComponent->BindAction(MinimapAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ToggleMinimap);
		if (DebugUIAction)
			EnhancedInputComponent->BindAction(DebugUIAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ToggleDebugUI);
	}
	}

void AAlex_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Minimap: hide when any UI is open
	if (MinimapWidget)
	{
		ESlateVisibility MapVis = bShowMouseCursor ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible;
		if (MinimapWidget->GetVisibility() != MapVis)
			MinimapWidget->SetVisibility(MapVis);
	}

	// Minimap: player arrow at full rate, enemy scan at 10 Hz
	MinimapRefreshThrottle += DeltaSeconds;
	if (bShowMouseCursor) { /* skip when UI open */ }
	else if (UUI_Minimap* MM = Cast<UUI_Minimap>(MinimapWidget.Get()))
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			MM->SetPlayerYaw(PlayerPawn->GetActorRotation().Yaw);

			if (MinimapRefreshThrottle >= 0.1f)
			{
				MinimapRefreshThrottle = 0.f;

				const FVector PlayerLoc = PlayerPawn->GetActorLocation();
				const float Scale = MM->WorldToPixelScale;
				const float HalfRange = MM->MapRadius / Scale;

				TArray<FVector2D> Offsets;
				TArray<float> Yaws;

				TArray<AActor*> Enemies;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), Enemies);
				for (AActor* Enemy : Enemies)
				{
					if (!Enemy || Enemy->IsActorBeingDestroyed()) continue;
					const FVector Rel = Enemy->GetActorLocation() - PlayerLoc;

					if (FMath::Abs(Rel.X) > HalfRange || FMath::Abs(Rel.Y) > HalfRange) continue;

					FCollisionQueryParams CeilingQuery;
					CeilingQuery.AddIgnoredActor(Enemy);
					FHitResult CeilingHit;
					const FVector TraceStart = Enemy->GetActorLocation() + FVector(0, 0, 50.f);
					const FVector TraceEnd = TraceStart + FVector(0, 0, 800.f);
					if (GetWorld()->LineTraceSingleByChannel(CeilingHit, TraceStart, TraceEnd, ECC_Visibility, CeilingQuery))
						continue;

					Offsets.Add(FVector2D(Rel.Y * Scale, -Rel.X * Scale));
					Yaws.Add(Enemy->GetActorRotation().Yaw);
				}
				MM->UpdateEnemyMarkers(Offsets, Yaws);
			}
		}
	}

	// Throttle UI refresh to ~10 Hz
	UIUpdateThrottle += DeltaSeconds;
	constexpr float UIUpdateInterval = 0.1f;
	if (UIUpdateThrottle >= UIUpdateInterval)
	{
		UIUpdateThrottle = 0.f;

		if (UUI_AimPoint* AimPoint = Cast<UUI_AimPoint>(AimPointWidget.Get()))
			AimPoint->RefreshAimDisplay();

		if (UUI_AmmoHUD* AmmoHud = Cast<UUI_AmmoHUD>(AmmoHudWidget))
			AmmoHud->RefreshAmmoDisplay();

		if (UUI_CharacterProperty* CharProp = Cast<UUI_CharacterProperty>(CharacterPropertyWidget.Get()))
			CharProp->RefreshStatusDisplay();

	}

	// --- Loot progress (hold-to-interact) ---
	if (bIsLootProgressActive)
	{
		if (!LootProgressTarget.IsValid())
		{
			CancelLootProgress();
		}
		else
		{
			if (!PlayerInteractComponent)
			{
				if (APawn* ControlledPawn = GetPawn())
					PlayerInteractComponent = ControlledPawn->FindComponentByClass<UPlayerInteractComponent>();
			}
			AActor* CurrentTarget = PlayerInteractComponent ? PlayerInteractComponent->GetCurrentInteractable() : nullptr;
			if (!CurrentTarget || Cast<ALootContainerActor>(CurrentTarget) != LootProgressTarget.Get())
			{
				CancelLootProgress();
			}
			else
			{
				const float SpeedMultiplier = bLootRunSpeedUp ? LootSpeedUpMultiplier : 1.0f;
				LootProgressElapsed += DeltaSeconds * SpeedMultiplier;

				const float Remaining = LootHoldDuration - LootProgressElapsed;
				const float Progress = FMath::Clamp(LootProgressElapsed / FMath::Max(LootHoldDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);

				if (UUI_LootProgressBar* Bar = Cast<UUI_LootProgressBar>(LootProgressWidget))
				{
					Bar->SetProgress(Progress);
					Bar->SetCountdownValue(Remaining);
				}

				if (LootProgressElapsed >= LootHoldDuration)
				{
					CompleteLootProgress();
					return;
				}
			}
		}
	}

	if (!bLootInteractionActive) return;

	if (!PlayerInteractComponent)
	{
		if (APawn* ControlledPawn = GetPawn())
			PlayerInteractComponent = ControlledPawn->FindComponentByClass<UPlayerInteractComponent>();
	}
	if (!PlayerInteractComponent) { CloseLootContainerUI(true); return; }

	AActor* CurrentTarget = PlayerInteractComponent->GetCurrentInteractable();
	if (Cast<ALootContainerActor>(CurrentTarget) != ActiveLootContainer.Get())
	{
		const float TimeSinceOpen = GetWorld() ? GetWorld()->GetTimeSeconds() - LootContainerOpenTime : 1.0f;
		if (ActiveLootContainer.IsValid() && ActiveLootContainer->GetInventoryComponent() && ActiveLootContainer->GetInventoryComponent()->Items.Num() > 0 && TimeSinceOpen < 0.5f)
			return;
		CloseLootContainerUI(true);
	}

	// --- Furniture walk-away ---
	if (bStorageCabinetActive || bTrainingMachineActive || bWeaponWorkbenchActive)
	{
		if (!PlayerInteractComponent)
		{
			if (APawn* ControlledPawn = GetPawn())
				PlayerInteractComponent = ControlledPawn->FindComponentByClass<UPlayerInteractComponent>();
		}
		if (!PlayerInteractComponent)
		{
			CloseAllFurnitureUI();
		}
		else
		{
			AActor* FurnitureTarget = PlayerInteractComponent->GetCurrentInteractable();
			if (bStorageCabinetActive && Cast<AStorageCabinetActor>(FurnitureTarget) != ActiveStorageCabinet.Get())
				CloseStorageCabinetUI(true);
			if (bTrainingMachineActive && Cast<ATrainingMachineActor>(FurnitureTarget) != ActiveTrainingMachine.Get())
				CloseTrainingMachineUI(true);
			if (bWeaponWorkbenchActive && Cast<AWeaponWorkbenchActor>(FurnitureTarget) != ActiveWeaponWorkbench.Get())
				CloseWeaponWorkbenchUI(true);
		}
	}
}

void AAlex_PlayerController::Move(const FInputActionValue& Value)
{
	if (bIsLootProgressActive || bLootInteractionActive) return;

	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		if (PlayerCharacter->IsDodgeMontagePlaying()) return;
	FVector2D MovementVector = Value.Get<FVector2D>();
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		FRotator Rotation = GetControlRotation();
		FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AAlex_PlayerController::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	float Scale = 1.f;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		if (PlayerCharacter->IsAiming()) Scale = PlayerCharacter->AimLookSpeedScale;
	AddYawInput(LookVector.X * Scale);
	AddPitchInput(-LookVector.Y * Scale);
}

void AAlex_PlayerController::Jump()
{
	if (bIsLootProgressActive || bShowMouseCursor) return;

	if (bLootInteractionActive)
	{
		TransferAllItemsFromContainer();
		return;
	}

	if (GetCharacter()) GetCharacter()->Jump();
}

void AAlex_PlayerController::Run(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) return;
	if (bIsLootProgressActive) bLootRunSpeedUp = true;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetCharacter()))
	{
		PlayerCharacter->SetRunInputHeld(true);
		if (!bIsLootProgressActive && !PlayerCharacter->IsAiming())
			PlayerCharacter->SetTargetMoveSpeed(PlayerCharacter->GetRunSpeed());
	}
}

void AAlex_PlayerController::StopRun()
{
	bLootRunSpeedUp = false;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetCharacter()))
	{
		PlayerCharacter->SetRunInputHeld(false);
		PlayerCharacter->SetTargetMoveSpeed(PlayerCharacter->GetMoveSpeed());
	}
}

void AAlex_PlayerController::AimStarted(const FInputActionValue& Value)
{
	if (bIsLootProgressActive || bShowMouseCursor) return;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		PlayerCharacter->SetAiming(true);
}

void AAlex_PlayerController::AimStopped(const FInputActionValue& Value)
{
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		PlayerCharacter->SetAiming(false);
}

void AAlex_PlayerController::AttackPressed(const FInputActionValue& Value)
{
	if (bIsLootProgressActive || bShowMouseCursor) return;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->SetPrimaryFireHeld(true);
		PlayerCharacter->TryAttack();
	}
}

void AAlex_PlayerController::AttackReleased(const FInputActionValue& Value)
{
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		PlayerCharacter->SetPrimaryFireHeld(false);
}

void AAlex_PlayerController::ReloadPressed(const FInputActionValue& Value)
{
	if (bIsLootProgressActive || bShowMouseCursor) return;
	// OnRKeyPressed handles R when UI is open
	if (bShowMouseCursor) return;

	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		PlayerCharacter->TryReload();
}

void AAlex_PlayerController::Dodge()
{
	if (bIsLootProgressActive || bShowMouseCursor) return;
	if (bShowMouseCursor) return;

	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		PlayerCharacter->TryDodge();
}


void AAlex_PlayerController::AddAccumulatedHitCount(const int32 Delta)
{
	if (Delta <= 0) return;
	AccumulatedHitCount += Delta;
}

void AAlex_PlayerController::SetLastRangedHitDamageForDebug(const float Amount)
{
	LastRangedHitDamage = Amount;
}

void AAlex_PlayerController::ShowDamageNumberAtLocation(FVector WorldLocation, float Damage, bool bHeadshot)
{
	if (!DamageNumberWidgetClass) return;

	FVector2D ScreenPosition;
	if (!ProjectWorldLocationToScreen(WorldLocation, ScreenPosition, true)) return;

	UUI_DamageNumber* DamageWidget = CreateWidget<UUI_DamageNumber>(this, DamageNumberWidgetClass);
	if (!DamageWidget) return;

	DamageWidget->SetDamage(Damage, bHeadshot);
	DamageWidget->AddToViewport();
	DamageWidget->SetPositionInViewport(ScreenPosition, true);
}

void AAlex_PlayerController::DrawDebugAccumulatedHitCount() const
{
}

void AAlex_PlayerController::InteractStarted()
{
	if (bLootInteractionActive)
	{
		const float TimeSinceOpen = GetWorld() ? GetWorld()->GetTimeSeconds() - LootContainerOpenTime : 1.0f;
		if (ActiveLootContainer.IsValid() && ActiveLootContainer->GetInventoryComponent() && ActiveLootContainer->GetInventoryComponent()->Items.Num() > 0 && TimeSinceOpen < 0.5f)
			return;
		CloseLootContainerUI(true);
		return;
	}

	// When UI is open, Interact key opens/closes container backpacks
	if (bShowMouseCursor)
	{
		// Toggle furniture close on second interact
		if (bStorageCabinetActive)
		{
			CloseStorageCabinetUI(true);
			return;
		}
		if (bWeaponWorkbenchActive)
		{
			CloseWeaponWorkbenchUI(true);
			return;
		}
		if (bTrainingMachineActive)
		{
			CloseTrainingMachineUI(true);
			return;
		}

		// Close container if open
		if (bContainerBackpackActive)
		{
			CloseContainerBackpackUI(true);
			return;
		}

		// Try to find a container item to open
		UInventoryItemInstance* ContainerItem = nullptr;
		if (UUI_Inventory* InvUI = Cast<UUI_Inventory>(InventoryWidget))
			ContainerItem = InvUI->GetHoveredItemInstance();
		if (!ContainerItem)
		{
			if (UUI_ContainerBackpack* CUI = Cast<UUI_ContainerBackpack>(ContainerBackpackWidget))
				ContainerItem = CUI->GetHoveredItemInstance();
		}
		if (!ContainerItem)
		{
			if (UUI_Equipment* EqUI = Cast<UUI_Equipment>(EquipmentWidget))
				ContainerItem = EqUI->GetHoveredEquippedContainerItemInstance();
		}
		if (ContainerItem)
		{
			OnContainerOpenRequested(ContainerItem);
		}
		return;
	}

	if (!PlayerInteractComponent)
	{
		if (APawn* ControlledPawn = GetPawn())
			PlayerInteractComponent = ControlledPawn->FindComponentByClass<UPlayerInteractComponent>();
	}
	if (!PlayerInteractComponent) return;

	AActor* Target = PlayerInteractComponent->GetCurrentInteractable();
	if (ALootContainerActor* LootContainer = Cast<ALootContainerActor>(Target))
	{
		if (LootContainer->bHasBeenLooted)
			OpenLootContainerUI(LootContainer);
		else
			StartLootProgress(LootContainer);
		return;
	}

	if (AStorageCabinetActor* Cabinet = Cast<AStorageCabinetActor>(Target))
	{
		OpenStorageCabinetUI(Cabinet);
		return;
	}
	if (ATrainingMachineActor* Machine = Cast<ATrainingMachineActor>(Target))
	{
		OpenTrainingMachineUI(Machine);
		return;
	}
	if (AWeaponWorkbenchActor* Workbench = Cast<AWeaponWorkbenchActor>(Target))
	{
		OpenWeaponWorkbenchUI(Workbench);
		return;
	}

	// Non-container / non-furniture interactable: instant interaction (e.g. Bed)
	PlayerInteractComponent->TryInteract();
}

void AAlex_PlayerController::InteractCompleted()
{
	if (!bIsLootProgressActive) return;

	if (LootProgressElapsed >= LootHoldDuration)
		CompleteLootProgress();
	else
		CancelLootProgress();
}

void AAlex_PlayerController::StartLootProgress(ALootContainerActor* Container)
{
	if (!Container || !LootProgressBarWidgetClass) return;

	bIsLootProgressActive = true;
	LootProgressElapsed = 0.0f;
	LootProgressTarget = Container;
	bLootRunSpeedUp = false;

	// Calculate loot duration: 2.0 * searchMultiplier
	int32 HighestRarity = 0;
	if (UInventoryComponent* ContainerInv = Container->GetInventoryComponent())
	{
		for (const TObjectPtr<UInventoryItemInstance>& Item : ContainerInv->Items)
		{
			if (Item && Item->ItemData)
			{
				const int32 RarityVal = static_cast<int32>(Item->ItemData->ItemData.Rarity);
				if (RarityVal > HighestRarity) HighestRarity = RarityVal;
			}
		}
	}

	const float RarityInfluence = 1.0f + 1.5f * static_cast<float>(HighestRarity);
	float SearchMultiplier = RarityInfluence;
	if (AAlex_PlayerCharacter* PlayerChar = Cast<AAlex_PlayerCharacter>(GetPawn()))
		SearchMultiplier += PlayerChar->SearchAbilityCoefficient;

	LootHoldDuration = 2.0f * SearchMultiplier;

	// Stop character movement
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UCharacterMovementComponent* MoveComp = ControlledPawn->FindComponentByClass<UCharacterMovementComponent>())
			MoveComp->Velocity = FVector::ZeroVector;
	}

	// Check if run key is already held
	if (AAlex_PlayerCharacter* PlayerChar = Cast<AAlex_PlayerCharacter>(GetPawn()))
	{
		if (PlayerChar->IsRunInputHeld())
			bLootRunSpeedUp = true;
	}

	LootProgressWidget = CreateWidget<UUserWidget>(this, LootProgressBarWidgetClass);
	if (LootProgressWidget)
	{
		LootProgressWidget->AddToViewport(100);
		if (UUI_LootProgressBar* Bar = Cast<UUI_LootProgressBar>(LootProgressWidget))
		{
			Bar->SetProgress(0.0f);
			Bar->SetCountdownValue(LootHoldDuration);
		}
	}
}

void AAlex_PlayerController::CancelLootProgress()
{
	bIsLootProgressActive = false;
	bLootRunSpeedUp = false;
	LootProgressElapsed = 0.0f;
	LootProgressTarget = nullptr;

	if (LootProgressWidget)
	{
		LootProgressWidget->RemoveFromParent();
		LootProgressWidget = nullptr;
	}
}

void AAlex_PlayerController::CompleteLootProgress()
{
	ALootContainerActor* Container = LootProgressTarget.Get();

	bIsLootProgressActive = false;
	bLootRunSpeedUp = false;
	LootProgressElapsed = 0.0f;
	LootProgressTarget = nullptr;

	if (LootProgressWidget)
	{
		LootProgressWidget->RemoveFromParent();
		LootProgressWidget = nullptr;
	}

	if (Container)
	{
		Container->bHasBeenLooted = true;
		OpenLootContainerUI(Container);
	}
}

void AAlex_PlayerController::TransferAllItemsFromContainer()
{
	ALootContainerActor* Container = ActiveLootContainer.Get();
	if (!Container) return;

	UInventoryComponent* ContainerInv = Container->GetInventoryComponent();
	if (!ContainerInv || !InventoryComponent) return;

	TArray<UInventoryItemInstance*> ItemsToTransfer;
	for (const TObjectPtr<UInventoryItemInstance>& Item : ContainerInv->Items)
	{
		if (Item) ItemsToTransfer.Add(Item);
	}

	for (UInventoryItemInstance* Item : ItemsToTransfer)
	{
		ContainerInv->DetachItemInstance(Item);
		if (!InventoryComponent->AddExistingItemInstance(Item))
		{
			ContainerInv->AttachItemInstance(Item);
		}
	}

	if (UUI_LootContainer* LootUI = Cast<UUI_LootContainer>(LootContainerWidget))
		LootUI->UpdateInventory();
	if (UUI_Inventory* InvUI = Cast<UUI_Inventory>(InventoryWidget))
		InvUI->UpdateInventory();
}

bool AAlex_PlayerController::EnsureInventoryUIVisible(bool bShowEquipment)
{
	if (!InventoryComponent) return false;
	if (AAlex_PlayerCharacter* Alex = Cast<AAlex_PlayerCharacter>(GetPawn()))
		if (Alex->IsReloadMontagePlaying()) return false;
	if (!InventoryWidgetClass) return false;

	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
		if (!InventoryWidget) return false;
		if (UUI_Inventory* InventoryUI = Cast<UUI_Inventory>(InventoryWidget))
			InventoryUI->BindInventory(InventoryComponent);
	}
	if (!InventoryWidget->IsInViewport()) InventoryWidget->AddToViewport();
	InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!InventoryComponent->IsInventoryOpen()) InventoryComponent->OpenInventory();
	else if (UUI_Inventory* InventoryUI = Cast<UUI_Inventory>(InventoryWidget)) InventoryUI->UpdateInventory();

	if (bShowEquipment) EnsureEquipmentUIVisibleWithInventory();
	else if (EquipmentWidget) EquipmentWidget->SetVisibility(ESlateVisibility::Collapsed);

	return true;
}

void AAlex_PlayerController::EnsureEquipmentUIVisibleWithInventory()
{
	if (!EquipmentComponent || !EquipmentWidgetClass || !InventoryWidget) return;
	if (!EquipmentWidget)
	{
		EquipmentWidget = CreateWidget<UUserWidget>(this, EquipmentWidgetClass);
		if (UUI_Equipment* EquipmentUI = Cast<UUI_Equipment>(EquipmentWidget))
			EquipmentUI->BindEquipment(EquipmentComponent, InventoryComponent, Cast<UUI_Inventory>(InventoryWidget));
	}
	if (!EquipmentWidget) return;
	if (!EquipmentWidget->IsInViewport()) EquipmentWidget->AddToViewport();
	EquipmentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UUI_Equipment* EquipmentUI = Cast<UUI_Equipment>(EquipmentWidget))
		EquipmentUI->SetInventoryUIForCellSync(Cast<UUI_Inventory>(InventoryWidget));
}

void AAlex_PlayerController::OpenLootContainerUI(ALootContainerActor* LootContainer)
{
	if (!LootContainer || !LootContainerWidgetClass) return;
	const bool bInventoryWasOpenBeforeLootInteraction = InventoryComponent && InventoryComponent->IsInventoryOpen();
	bLootInteractionActive = true;
	ActiveLootContainer = LootContainer;
	bInventoryOpenedByLootInteraction = !bInventoryWasOpenBeforeLootInteraction;
	LootContainerOpenTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (!EnsureInventoryUIVisible(false)) { bLootInteractionActive = false; ActiveLootContainer = nullptr; bInventoryOpenedByLootInteraction = false; return; }

	if (!LootContainerWidget) LootContainerWidget = CreateWidget<UUserWidget>(this, LootContainerWidgetClass);
	if (!LootContainerWidget) { bLootInteractionActive = false; ActiveLootContainer = nullptr; bInventoryOpenedByLootInteraction = false; return; }

	if (UUI_LootContainer* LootContainerUI = Cast<UUI_LootContainer>(LootContainerWidget))
	{
		LootContainerUI->BindLootContainer(LootContainer);
		LootContainerUI->UpdateInventory();
	}
	else { bLootInteractionActive = false; ActiveLootContainer = nullptr; bInventoryOpenedByLootInteraction = false; return; }

	if (!LootContainerWidget->IsInViewport()) LootContainerWidget->AddToViewport();
	LootContainerWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LootContainer->OpenContainer();

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(LootContainerWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	// Look ignore is already applied in OnInventoryToggled(true) from EnsureInventoryUIVisible;
	// stacking SetIgnoreLookInput(true) again here breaks mouse look permanently after close.
}

void AAlex_PlayerController::CloseLootContainerUI(bool bCloseInventoryIfOpenedByLootInteraction)
{
	if (ALootContainerActor* LootContainer = ActiveLootContainer.Get()) LootContainer->CloseContainer();
	ActiveLootContainer = nullptr;
	bLootInteractionActive = false;
	if (LootContainerWidget) { LootContainerWidget->RemoveFromParent(); LootContainerWidget = nullptr; }

	const bool bShouldCloseInventory = bCloseInventoryIfOpenedByLootInteraction && bInventoryOpenedByLootInteraction && InventoryComponent && InventoryComponent->IsInventoryOpen();
	bInventoryOpenedByLootInteraction = false;

	if (bShouldCloseInventory)
	{
		InventoryComponent->CloseInventory();
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		ResetIgnoreLookInput();
		return;
	}

	if (InventoryComponent && InventoryComponent->IsInventoryOpen() && InventoryWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		EnsureEquipmentUIVisibleWithInventory();
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	ResetIgnoreLookInput();
}

void AAlex_PlayerController::ToggleInventory()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bToggleCooldown && CurrentTime - LastToggleTime < 0.5f) return;
	if (bContainerBackpackActive)
	{
		FSlateApplication::Get().ClearAllUserFocus();
		FSlateApplication::Get().ReleaseAllPointerCapture();
		if (ContainerBackpackWidget)
		{
			ContainerBackpackWidget->RemoveFromParent();
			ContainerBackpackWidget = nullptr;
		}
		ActiveContainerBackpackItem = nullptr;
		bContainerBackpackActive = false;
		bInventoryOpenedByContainerBackpack = false;
	}
	bToggleCooldown = false;
	if (bIsToggling) return;
	if (!InventoryComponent) return;

	if (bShowMouseCursor) { bIsToggling = true; LastToggleTime = CurrentTime; InventoryComponent->CloseInventory(); return; }

	if (AAlex_PlayerCharacter* Alex = Cast<AAlex_PlayerCharacter>(GetPawn()))
		if (Alex->IsReloadMontagePlaying()) return;

	if (!InventoryWidget)
	{
		if (InventoryWidgetClass)
		{
			InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			if (InventoryWidget && InventoryComponent) { if (UUI_Inventory* UI = Cast<UUI_Inventory>(InventoryWidget)) UI->BindInventory(InventoryComponent); InventoryWidget->AddToViewport(); }
			else return;
		}
		else return;
	}
	if (!InventoryWidget->IsInViewport()) InventoryWidget->AddToViewport();
	InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	EnsureEquipmentUIVisibleWithInventory();
	bIsToggling = true;
	LastToggleTime = CurrentTime;
	InventoryComponent->OpenInventory();
}

void AAlex_PlayerController::OnInventoryToggled(bool bIsOpen)
{
	if (bIsOpen)
	{
		if (InventoryWidget && Cast<UUI_Inventory>(InventoryWidget)) Cast<UUI_Inventory>(InventoryWidget)->UpdateInventory();
		if (!bLootInteractionActive) EnsureEquipmentUIVisibleWithInventory();

		FInputModeGameAndUI InputMode;
		if (InventoryWidget) InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		ResetIgnoreLookInput();
		SetIgnoreLookInput(true);
		if (GetCharacter() && GetCharacter()->GetCharacterMovement()) GetCharacter()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		bToggleCooldown = true;
		bIsToggling = false;
		if (AmmoHudWidget) { if (AmmoHudWidget->IsInViewport()) AmmoHudWidget->RemoveFromParent(); AmmoHudWidget->AddToViewport(500); }
		OnInventoryAmmoRefresh();
	}
	else
	{
		if (bLootInteractionActive) CloseLootContainerUI(false);
		CloseAllFurnitureUI();
		if (bContainerBackpackActive)
		{
			FSlateApplication::Get().ClearAllUserFocus();
			FSlateApplication::Get().ReleaseAllPointerCapture();
			if (ContainerBackpackWidget)
			{
				ContainerBackpackWidget->RemoveFromParent();
				ContainerBackpackWidget = nullptr;
			}
			ActiveContainerBackpackItem = nullptr;
			bContainerBackpackActive = false;
			bInventoryOpenedByContainerBackpack = false;
		}
		if (InventoryWidget) { InventoryWidget->RemoveFromParent(); InventoryWidget = nullptr; }
		if (EquipmentWidget) { EquipmentWidget->RemoveFromParent(); EquipmentWidget = nullptr; }
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		ResetIgnoreLookInput();
		bToggleCooldown = true;
		bIsToggling = false;
		if (AmmoHudWidget) { if (AmmoHudWidget->IsInViewport()) AmmoHudWidget->RemoveFromParent(); AmmoHudWidget->AddToViewport(5); }
		OnInventoryAmmoRefresh();
		return;
	}
	bIsToggling = false;
}

void AAlex_PlayerController::EnsureAimPointWidgetCreated()
{
	if (!AimPointWidgetClass || AimPointWidget) return;
	if (!AimPointWidgetClass->IsChildOf(UUI_AimPoint::StaticClass())) return;
	AimPointWidget = CreateWidget<UUserWidget>(this, AimPointWidgetClass);
	if (!AimPointWidget) return;
	if (!AimPointWidget->IsInViewport()) AimPointWidget->AddToViewport(10);
	AimPointWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AAlex_PlayerController::EnsureAmmoHudWidgetCreated()
{
	if (!AmmoHudWidgetClass || AmmoHudWidget) return;
	if (!AmmoHudWidgetClass->IsChildOf(UUI_AmmoHUD::StaticClass())) return;
	AmmoHudWidget = CreateWidget<UUserWidget>(this, AmmoHudWidgetClass);
	if (!AmmoHudWidget) return;
	if (!AmmoHudWidget->IsInViewport()) AmmoHudWidget->AddToViewport(5);
}

void AAlex_PlayerController::EnsureCharacterPropertyWidgetCreated()
{
	if (!CharacterPropertyWidgetClass || CharacterPropertyWidget) return;
	if (!CharacterPropertyWidgetClass->IsChildOf(UUI_CharacterProperty::StaticClass())) return;
	CharacterPropertyWidget = CreateWidget<UUserWidget>(this, CharacterPropertyWidgetClass);
	if (!CharacterPropertyWidget) return;
	if (!CharacterPropertyWidget->IsInViewport()) CharacterPropertyWidget->AddToViewport(3);
}

void AAlex_PlayerController::EnsureMinimapWidgetCreated()
{
	if (!MinimapWidgetClass || MinimapWidget) return;
	MinimapWidget = CreateWidget<UUserWidget>(this, MinimapWidgetClass);
	if (!MinimapWidget) return;
	if (!MinimapWidget->IsInViewport()) MinimapWidget->AddToViewport(1);
	MinimapWidget->SetVisibility(ESlateVisibility::Visible);

	// Spawn capture actor immediately (minimap is always visible)
	if (MinimapCaptureActorClass && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AMinimapCaptureActor* Cap = GetWorld()->SpawnActor<AMinimapCaptureActor>(MinimapCaptureActorClass, SpawnParams))
		{
			if (UUI_Minimap* MM = Cast<UUI_Minimap>(MinimapWidget))
				MM->SetRenderTarget(Cap->GetMinimapRenderTarget());
		}
	}
}

void AAlex_PlayerController::ToggleDebugUI()
{
	bDebugUIVisible = !bDebugUIVisible;
	// Refresh all open inventory/loot UIs to update debug button visibility
	if (UUI_Inventory* Inv = Cast<UUI_Inventory>(InventoryWidget))
		Inv->UpdateDebugButtonVisibility();
	if (UUI_LootContainer* Loot = Cast<UUI_LootContainer>(LootContainerWidget))
		Loot->UpdateDebugButtonVisibility();
	if (UUI_ContainerBackpack* CB = Cast<UUI_ContainerBackpack>(ContainerBackpackWidget))
		CB->UpdateDebugButtonVisibility();
	if (StorageCabinetWidget)
		if (UUI_Inventory* SC = Cast<UUI_Inventory>(StorageCabinetWidget))
			SC->UpdateDebugButtonVisibility();
}

void AAlex_PlayerController::ToggleMinimap()
{
	if (!MinimapWidget) return;

	if (UUI_Minimap* MM = Cast<UUI_Minimap>(MinimapWidget))
	{
		bMinimapVisible = !bMinimapVisible;
		MM->SetLargeMode(bMinimapVisible);
	}
}

void AAlex_PlayerController::AttachToPawnInventoryAndEquipment(APawn* InPawn)
{
	if (!InPawn)
	{
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryToggled.RemoveDynamic(this, &AAlex_PlayerController::OnInventoryToggled);
			InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &AAlex_PlayerController::OnInventoryAmmoRefresh);
		}
		if (EquipmentComponent)
			EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &AAlex_PlayerController::OnEquipmentAmmoRefresh);
		InventoryComponent = nullptr;
		EquipmentComponent = nullptr;
		PlayerInteractComponent = nullptr;
		return;
	}

	UInventoryComponent* NewInv = InPawn->FindComponentByClass<UInventoryComponent>();
	if (!NewInv)
	{
		if (AAlex_PlayerCharacter* Alex = Cast<AAlex_PlayerCharacter>(InPawn))
		{
			NewInv = Alex->GetInventory();
		}
	}
	UEquipmentComponent* const NewEq = InPawn->FindComponentByClass<UEquipmentComponent>();
	UPlayerInteractComponent* const NewInteract = InPawn->FindComponentByClass<UPlayerInteractComponent>();

	if (InventoryComponent != NewInv)
	{
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryToggled.RemoveDynamic(this, &AAlex_PlayerController::OnInventoryToggled);
			InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &AAlex_PlayerController::OnInventoryAmmoRefresh);
		}
		InventoryComponent = NewInv;
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryToggled.AddDynamic(this, &AAlex_PlayerController::OnInventoryToggled);
			InventoryComponent->OnInventoryChanged.AddDynamic(this, &AAlex_PlayerController::OnInventoryAmmoRefresh);
		}
	}

	if (EquipmentComponent != NewEq)
	{
		if (EquipmentComponent)
			EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &AAlex_PlayerController::OnEquipmentAmmoRefresh);
		EquipmentComponent = NewEq;
		if (EquipmentComponent)
			EquipmentComponent->OnEquipmentChanged.AddDynamic(this, &AAlex_PlayerController::OnEquipmentAmmoRefresh);
	}

	PlayerInteractComponent = NewInteract;
}

void AAlex_PlayerController::OnInventoryAmmoRefresh()
{
	if (UUI_AmmoHUD* AmmoHud = Cast<UUI_AmmoHUD>(AmmoHudWidget))
		AmmoHud->RefreshAmmoDisplay();
}

void AAlex_PlayerController::OnEquipmentAmmoRefresh(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem)
{
	OnInventoryAmmoRefresh();
}

void AAlex_PlayerController::OnContainerOpenRequested(UInventoryItemInstance* ContainerItem)
{
	if (!ContainerItem) return;
	if (bContainerBackpackActive && ActiveContainerBackpackItem.Get() == ContainerItem)
	{
		CloseContainerBackpackUI(true);
		return;
	}
	if (bContainerBackpackActive)
		CloseContainerBackpackUI(false);
	OpenContainerBackpackUI(ContainerItem);
}

void AAlex_PlayerController::OnContainerBackpackRequestedFromEquipment(UInventoryItemInstance* ContainerItem)
{
	OnContainerOpenRequested(ContainerItem);
}

void AAlex_PlayerController::OpenContainerBackpackUI(UInventoryItemInstance* ContainerItem)
{
	if (!ContainerItem || !ContainerBackpackWidgetClass || !ContainerItem->ItemData)
		return;
	const FArmorItemStats& ArmorStats = ContainerItem->ItemData->ItemData.ArmorStats;
	if (!ArmorStats.bIsContainer)
		return;

	ContainerItem->EnsureContainerInventory(ArmorStats.ContainerGridWidth, ArmorStats.ContainerGridHeight);

	const bool bInventoryWasOpen = InventoryComponent && InventoryComponent->IsInventoryOpen();
	bContainerBackpackActive = true;
	ActiveContainerBackpackItem = ContainerItem;
	bInventoryOpenedByContainerBackpack = !bInventoryWasOpen;

	if (!EnsureInventoryUIVisible(true))
	{
		bContainerBackpackActive = false;
		ActiveContainerBackpackItem = nullptr;
		bInventoryOpenedByContainerBackpack = false;
		return;
	}

	if (!ContainerBackpackWidget)
		ContainerBackpackWidget = CreateWidget<UUserWidget>(this, ContainerBackpackWidgetClass);
	if (!ContainerBackpackWidget)
	{
		bContainerBackpackActive = false;
		ActiveContainerBackpackItem = nullptr;
		bInventoryOpenedByContainerBackpack = false;
		return;
	}

	if (UUI_ContainerBackpack* ContainerUI = Cast<UUI_ContainerBackpack>(ContainerBackpackWidget))
	{
		ContainerUI->BindContainerBackpack(ContainerItem);
		ContainerUI->UpdateInventory();
	}

	if (!ContainerBackpackWidget->IsInViewport())
		ContainerBackpackWidget->AddToViewport();
	ContainerBackpackWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	FInputModeGameAndUI InputMode;
	// Keep focus on the primary bag widget. Focusing the nested container inventory root
	// can leave viewport mouse / look routing broken after the panel closes; drag handling
	// already retargets focus via SetDraggedItemWidget when needed.
	if (InventoryWidget)
	{
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	}
	else
	{
		InputMode.SetWidgetToFocus(ContainerBackpackWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	// Same as loot: OnInventoryToggled already pushed one ignore-look frame; do not stack again.
}

void AAlex_PlayerController::CloseContainerBackpackUI(bool bCloseInventoryIfOpenedByContainerBackpack)
{
	// Guard against recursive calls (e.g. from OnInventoryToggled)
	if (!bContainerBackpackActive) return;

	ActiveContainerBackpackItem = nullptr;
	bContainerBackpackActive = false;

	if (ContainerBackpackWidget)
	{
		ContainerBackpackWidget->RemoveFromParent();
		ContainerBackpackWidget = nullptr;
	}

	const bool bShouldCloseInventory = bCloseInventoryIfOpenedByContainerBackpack
		&& bInventoryOpenedByContainerBackpack
		&& InventoryComponent && InventoryComponent->IsInventoryOpen();
	bInventoryOpenedByContainerBackpack = false;

	if (bShouldCloseInventory)
	{
		InventoryComponent->CloseInventory();
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		ResetIgnoreLookInput();
		return;
	}

	if (InventoryComponent && InventoryComponent->IsInventoryOpen() && InventoryWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		EnsureEquipmentUIVisibleWithInventory();
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	ResetIgnoreLookInput();
}

void AAlex_PlayerController::CloseAllFurnitureUI()
{
	if (bStorageCabinetActive) CloseStorageCabinetUI(false);
	if (bTrainingMachineActive) CloseTrainingMachineUI(false);
	if (bWeaponWorkbenchActive) CloseWeaponWorkbenchUI(false);
}

void AAlex_PlayerController::RefreshPlayerInventoryUI()
{
	if (InventoryWidget)
		if (UUI_Inventory* InvUI = Cast<UUI_Inventory>(InventoryWidget))
			InvUI->UpdateInventory();
}

void AAlex_PlayerController::OpenStorageCabinetUI(AStorageCabinetActor* Cabinet)
{
	if (!Cabinet || !StorageCabinetWidgetClass) return;
	const bool bInventoryWasOpen = InventoryComponent && InventoryComponent->IsInventoryOpen();
	bStorageCabinetActive = true;
	ActiveStorageCabinet = Cabinet;
	bInventoryOpenedByCabinet = !bInventoryWasOpen;

	if (!EnsureInventoryUIVisible(false)) { bStorageCabinetActive = false; ActiveStorageCabinet = nullptr; bInventoryOpenedByCabinet = false; return; }

	if (!StorageCabinetWidget) StorageCabinetWidget = CreateWidget<UUserWidget>(this, StorageCabinetWidgetClass);
	if (!StorageCabinetWidget) { bStorageCabinetActive = false; ActiveStorageCabinet = nullptr; bInventoryOpenedByCabinet = false; return; }

	if (UUI_StorageCabinet* CabUI = Cast<UUI_StorageCabinet>(StorageCabinetWidget))
	{
		CabUI->BindStorageCabinet(Cabinet);
	}
	else if (UUI_LootContainer* LootUI = Cast<UUI_LootContainer>(StorageCabinetWidget))
	{
		// Fallback for generic LootContainer widgets
		LootUI->BindContainerInventory(Cabinet->GetInventoryComponent());
		LootUI->UpdateInventory();
	}
	else { bStorageCabinetActive = false; ActiveStorageCabinet = nullptr; bInventoryOpenedByCabinet = false; return; }

	if (!StorageCabinetWidget->IsInViewport()) StorageCabinetWidget->AddToViewport();
	StorageCabinetWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Cabinet->OpenCabinet();

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(StorageCabinetWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void AAlex_PlayerController::CloseStorageCabinetUI(bool bCloseInventoryIfOpenedByCabinet)
{
	if (AStorageCabinetActor* Cabinet = ActiveStorageCabinet.Get()) Cabinet->CloseCabinet();
	ActiveStorageCabinet = nullptr;
	bStorageCabinetActive = false;
	if (StorageCabinetWidget) { StorageCabinetWidget->RemoveFromParent(); StorageCabinetWidget = nullptr; }

	const bool bShouldCloseInventory = bCloseInventoryIfOpenedByCabinet && bInventoryOpenedByCabinet && InventoryComponent && InventoryComponent->IsInventoryOpen();
	bInventoryOpenedByCabinet = false;

	if (bShouldCloseInventory)
	{
		InventoryComponent->CloseInventory();
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		ResetIgnoreLookInput();
		return;
	}

	if (InventoryComponent && InventoryComponent->IsInventoryOpen() && InventoryWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		EnsureEquipmentUIVisibleWithInventory();
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	ResetIgnoreLookInput();
}

void AAlex_PlayerController::OpenTrainingMachineUI(ATrainingMachineActor* Machine)
{
	if (!Machine || !TrainingMachineWidgetClass) return;
	bTrainingMachineActive = true;
	ActiveTrainingMachine = Machine;

	TrainingMachineWidget = CreateWidget<UUserWidget>(this, TrainingMachineWidgetClass);
	if (!TrainingMachineWidget) { bTrainingMachineActive = false; ActiveTrainingMachine = nullptr; return; }

	if (UUI_TrainingMachine* UI = Cast<UUI_TrainingMachine>(TrainingMachineWidget))
		UI->BindTrainingMachine(Machine);

	if (!TrainingMachineWidget->IsInViewport()) TrainingMachineWidget->AddToViewport();
	TrainingMachineWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TrainingMachineWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	SetIgnoreLookInput(true);
}

void AAlex_PlayerController::CloseTrainingMachineUI(bool bCloseInventoryIfOpenedByMachine)
{
	ActiveTrainingMachine = nullptr;
	bTrainingMachineActive = false;
	if (TrainingMachineWidget) { TrainingMachineWidget->RemoveFromParent(); TrainingMachineWidget = nullptr; }

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	ResetIgnoreLookInput();
}

void AAlex_PlayerController::OpenWeaponWorkbenchUI(AWeaponWorkbenchActor* Workbench)
{
	if (!Workbench || !WeaponWorkbenchWidgetClass) return;
	const bool bInventoryWasOpen = InventoryComponent && InventoryComponent->IsInventoryOpen();
	bWeaponWorkbenchActive = true;
	ActiveWeaponWorkbench = Workbench;
	bInventoryOpenedByWeaponWorkbench = !bInventoryWasOpen;

	if (!EnsureInventoryUIVisible(false)) { bWeaponWorkbenchActive = false; ActiveWeaponWorkbench = nullptr; return; }

	WeaponWorkbenchWidget = CreateWidget<UUserWidget>(this, WeaponWorkbenchWidgetClass);
	if (!WeaponWorkbenchWidget) { bWeaponWorkbenchActive = false; ActiveWeaponWorkbench = nullptr; return; }

	if (UUI_WeaponWorkbench* UI = Cast<UUI_WeaponWorkbench>(WeaponWorkbenchWidget))
		UI->BindWeaponWorkbench(Workbench);

	if (!WeaponWorkbenchWidget->IsInViewport()) WeaponWorkbenchWidget->AddToViewport();
	WeaponWorkbenchWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(WeaponWorkbenchWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	SetIgnoreLookInput(true);
}

void AAlex_PlayerController::CloseWeaponWorkbenchUI(bool bCloseInventoryIfOpenedByWorkbench)
{
	ActiveWeaponWorkbench = nullptr;
	bWeaponWorkbenchActive = false;
	if (WeaponWorkbenchWidget) { WeaponWorkbenchWidget->RemoveFromParent(); WeaponWorkbenchWidget = nullptr; }

	const bool bShouldCloseInventory = bCloseInventoryIfOpenedByWorkbench && bInventoryOpenedByWeaponWorkbench && InventoryComponent && InventoryComponent->IsInventoryOpen();
	bInventoryOpenedByWeaponWorkbench = false;

	if (bShouldCloseInventory)
	{
		InventoryComponent->CloseInventory();
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		ResetIgnoreLookInput();
		return;
	}

	if (InventoryComponent && InventoryComponent->IsInventoryOpen() && InventoryWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	ResetIgnoreLookInput();
}
