#include "Alex_PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
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
#include "UI/UI_AimPoint.h"

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
	bIsToggling = false;
	LastToggleTime = 0.0f;
	bToggleCooldown = false;
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
}

void AAlex_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AttachToPawnInventoryAndEquipment(InPawn);
	EnsureAimPointWidgetCreated();
	EnsureAmmoHudWidgetCreated();
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
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAlex_PlayerController::Interact);
		if (InventoryAction)
			EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ToggleInventory);
		if (ReloadAction)
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ReloadPressed);
	}
}

void AAlex_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawDebugAccumulatedHitCount();

	if (UUI_AimPoint* AimPoint = Cast<UUI_AimPoint>(AimPointWidget.Get()))
		AimPoint->RefreshAimDisplay();

	if (UUI_AmmoHUD* AmmoHud = Cast<UUI_AmmoHUD>(AmmoHudWidget))
		AmmoHud->RefreshAmmoDisplay();

	if (!bLootInteractionActive) return;

	if (!PlayerInteractComponent)
	{
		if (APawn* ControlledPawn = GetPawn())
			PlayerInteractComponent = ControlledPawn->FindComponentByClass<UPlayerInteractComponent>();
	}
	if (!PlayerInteractComponent) { CloseLootContainerUI(true); return; }

	AActor* CurrentTarget = PlayerInteractComponent->GetCurrentInteractable();
	if (Cast<ALootContainerActor>(CurrentTarget) != ActiveLootContainer.Get())
		CloseLootContainerUI(true);
}

void AAlex_PlayerController::Move(const FInputActionValue& Value)
{
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
	if (GetCharacter()) GetCharacter()->Jump();
}

void AAlex_PlayerController::Run(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) return;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetCharacter()))
	{
		PlayerCharacter->SetRunInputHeld(true);
		if (PlayerCharacter->IsAiming()) return;
		PlayerCharacter->SetTargetMoveSpeed(PlayerCharacter->GetRunSpeed());
	}
}

void AAlex_PlayerController::StopRun()
{
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetCharacter()))
	{
		PlayerCharacter->SetRunInputHeld(false);
		PlayerCharacter->SetTargetMoveSpeed(PlayerCharacter->GetMoveSpeed());
	}
}

void AAlex_PlayerController::AimStarted(const FInputActionValue& Value)
{
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
	if (bShowMouseCursor) return;
	if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetPawn()))
		PlayerCharacter->TryReload();
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

void AAlex_PlayerController::DrawDebugAccumulatedHitCount() const
{
	if (!GEngine || !IsLocalPlayerController()) return;
	static constexpr int32 HitCounterOnScreenKey = 871042;
	static constexpr int32 LastDamageOnScreenKey = 871043;

	if (bShowAccumulatedHitCounterOnScreen)
	{
		const FString Line = FString::Printf(TEXT("Hits: %d"), AccumulatedHitCount);
		GEngine->AddOnScreenDebugMessage(HitCounterOnScreenKey, 0.f, FColor(220, 255, 180), Line, false, FVector2D(HitCounterOnScreenTextScale, HitCounterOnScreenTextScale));
	}
	else GEngine->RemoveOnScreenDebugMessage(HitCounterOnScreenKey);

	if (bShowLastRangedHitDamageOnScreen)
	{
		const FString DamageLine = FString::Printf(TEXT("Last damage: %.1f"), LastRangedHitDamage);
		GEngine->AddOnScreenDebugMessage(LastDamageOnScreenKey, 0.f, FColor(180, 220, 255), DamageLine, false, FVector2D(LastHitDamageOnScreenTextScale, LastHitDamageOnScreenTextScale));
	}
	else GEngine->RemoveOnScreenDebugMessage(LastDamageOnScreenKey);
}

void AAlex_PlayerController::Interact()
{
	if (bLootInteractionActive) { CloseLootContainerUI(true); return; }
	if (bShowMouseCursor) return;

	if (!PlayerInteractComponent)
	{
		if (APawn* ControlledPawn = GetPawn())
			PlayerInteractComponent = ControlledPawn->FindComponentByClass<UPlayerInteractComponent>();
	}
	if (PlayerInteractComponent && PlayerInteractComponent->TryInteract())
	{
		AActor* Target = PlayerInteractComponent->GetCurrentInteractable();
		if (ALootContainerActor* LootContainer = Cast<ALootContainerActor>(Target))
			OpenLootContainerUI(LootContainer);
	}
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
	SetIgnoreLookInput(true);
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
		SetIgnoreLookInput(false);
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
		SetIgnoreLookInput(true);
		EnsureEquipmentUIVisibleWithInventory();
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	SetIgnoreLookInput(false);
}

void AAlex_PlayerController::ToggleInventory()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bToggleCooldown && CurrentTime - LastToggleTime < 0.5f) return;
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
		if (InventoryWidget) { InventoryWidget->RemoveFromParent(); InventoryWidget = nullptr; }
		if (EquipmentWidget) { EquipmentWidget->RemoveFromParent(); EquipmentWidget = nullptr; }
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		SetIgnoreLookInput(false);
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
