#include "Alex_PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Alex_PlayerCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Inventory/InventoryComponent.h"
#include "UI/UI_Inventory.h"
#include "Kismet/GameplayStatics.h"

AAlex_PlayerController::AAlex_PlayerController()
{
	InventoryWidget = nullptr;
	InventoryComponent = nullptr;
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
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			UE_LOG(LogTemp, Warning, TEXT("Mapping context added successfully"));
		}
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		InventoryComponent = ControlledPawn->FindComponentByClass<UInventoryComponent>();
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryToggled.AddDynamic(this, &AAlex_PlayerController::OnInventoryToggled);
			UE_LOG(LogTemp, Warning, TEXT("InventoryComponent found and delegate bound"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InventoryComponent not found on pawn!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Controlled pawn is null!"));
	}

	if (!InventoryAction)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryAction is not set in Blueprint!"));
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryWidgetClass is not set in Blueprint!"));
	}
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
		
		if (InventoryAction)
		{
			EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAlex_PlayerController::ToggleInventory);
			UE_LOG(LogTemp, Warning, TEXT("InventoryAction bound successfully"));
		}
	}
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
	AddYawInput(LookVector.X);
	AddPitchInput(-LookVector.Y);
}

void AAlex_PlayerController::Jump()
{
	if (GetCharacter())
	{
		GetCharacter()->Jump();
	}
}

void AAlex_PlayerController::Run(const FInputActionValue& Value)
{
	if (Value.Get<bool>() && GetCharacter())
	{
		if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetCharacter()))
		{
			PlayerCharacter->SetTargetMoveSpeed(PlayerCharacter->GetRunSpeed());
		}
	}
}

void AAlex_PlayerController::StopRun()
{
	if (GetCharacter())
	{
		if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(GetCharacter()))
		{
			PlayerCharacter->SetTargetMoveSpeed(PlayerCharacter->GetMoveSpeed());
		}
	}
}

void AAlex_PlayerController::ToggleInventory()
{
	UE_LOG(LogTemp, Warning, TEXT("=== ToggleInventory called ==="));
	UE_LOG(LogTemp, Warning, TEXT("Current state: bShowMouseCursor=%s, bIsToggling=%s, bToggleCooldown=%s"), 
		bShowMouseCursor ? TEXT("true") : TEXT("false"),
		bIsToggling ? TEXT("true") : TEXT("false"),
		bToggleCooldown ? TEXT("true") : TEXT("false"));
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	if (bToggleCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cooldown check: CurrentTime=%.2f, LastToggleTime=%.2f, Diff=%.2f"), 
			CurrentTime, LastToggleTime, CurrentTime - LastToggleTime);
		if (CurrentTime - LastToggleTime < 0.5f)
		{
			UE_LOG(LogTemp, Warning, TEXT("In cooldown period, ignoring input"));
			return;
		}
		bToggleCooldown = false;
		UE_LOG(LogTemp, Warning, TEXT("Cooldown expired, resetting bToggleCooldown"));
	}
	
	if (bIsToggling)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already toggling, ignoring"));
		return;
	}
	
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent is null in ToggleInventory!"));
		return;
	}
	
	if (bShowMouseCursor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Inventory is open, closing it"));
		bIsToggling = true;
		LastToggleTime = CurrentTime;
		UE_LOG(LogTemp, Warning, TEXT("Calling CloseInventory()"));
		InventoryComponent->CloseInventory();
		return;
	}
	
	if (!InventoryWidget)
	{
		if (InventoryWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Creating InventoryWidget"));
			InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			if (InventoryWidget)
			{
				UUI_Inventory* InventoryUI = Cast<UUI_Inventory>(InventoryWidget);
				if (InventoryUI && InventoryComponent)
				{
					UE_LOG(LogTemp, Warning, TEXT("Binding inventory to UI"));
					InventoryUI->BindInventory(InventoryComponent);
				}
				InventoryWidget->AddToViewport();
				UE_LOG(LogTemp, Warning, TEXT("InventoryWidget added to viewport"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create InventoryWidget!"));
				return;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InventoryWidgetClass is not set!"));
			return;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Opening inventory"));
	bIsToggling = true;
	LastToggleTime = CurrentTime;
	UE_LOG(LogTemp, Warning, TEXT("Calling OpenInventory()"));
	InventoryComponent->OpenInventory();
}

void AAlex_PlayerController::OnInventoryToggled(bool bIsOpen)
{
	UE_LOG(LogTemp, Warning, TEXT("=== OnInventoryToggled called ==="));
	UE_LOG(LogTemp, Warning, TEXT("bIsOpen: %s"), bIsOpen ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("Before processing: bIsToggling=%s, bToggleCooldown=%s, bShowMouseCursor=%s"), 
		bIsToggling ? TEXT("true") : TEXT("false"),
		bToggleCooldown ? TEXT("true") : TEXT("false"),
		bShowMouseCursor ? TEXT("true") : TEXT("false"));

	if (bIsOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("Processing OPEN state"));

		if (InventoryWidget)
		{
			if (UUI_Inventory* InventoryUI = Cast<UUI_Inventory>(InventoryWidget))
			{
				UE_LOG(LogTemp, Warning, TEXT("Calling UpdateInventory() for existing widget"));
				InventoryUI->UpdateInventory();
			}
		}

		FInputModeGameAndUI InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		
		if (GetCharacter())
		{
			if (GetCharacter()->GetCharacterMovement())
			{
				GetCharacter()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			}
		}
		
		bToggleCooldown = true;
		bIsToggling = false;
		
		UE_LOG(LogTemp, Warning, TEXT("After OPEN: bIsToggling=%s, bToggleCooldown=%s, bShowMouseCursor=%s"), 
			bIsToggling ? TEXT("true") : TEXT("false"),
			bToggleCooldown ? TEXT("true") : TEXT("false"),
			bShowMouseCursor ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Processing CLOSE state"));

		if (InventoryWidget)
		{
			InventoryWidget->RemoveFromParent();
			InventoryWidget = nullptr;
			UE_LOG(LogTemp, Warning, TEXT("InventoryWidget removed"));
		}

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		
		bToggleCooldown = true;
		bIsToggling = false;
		
		UE_LOG(LogTemp, Warning, TEXT("After CLOSE: bIsToggling=%s, bToggleCooldown=%s, bShowMouseCursor=%s"), 
			bIsToggling ? TEXT("true") : TEXT("false"),
			bToggleCooldown ? TEXT("true") : TEXT("false"),
			bShowMouseCursor ? TEXT("true") : TEXT("false"));
		return;
	}
	
	bIsToggling = false;
	UE_LOG(LogTemp, Warning, TEXT("=== OnInventoryToggled finished ==="));
}