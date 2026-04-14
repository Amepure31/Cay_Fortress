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
		}
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		InventoryComponent = ControlledPawn->FindComponentByClass<UInventoryComponent>();
		if (InventoryComponent)
		{
			InventoryComponent->OnInventoryToggled.AddDynamic(this, &AAlex_PlayerController::OnInventoryToggled);
		}
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
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	if (bToggleCooldown)
	{
		if (CurrentTime - LastToggleTime < 0.5f)
		{
			return;
		}
		bToggleCooldown = false;
	}
	
	if (bIsToggling)
	{
		return;
	}
	
	if (!InventoryComponent)
	{
		return;
	}
	
	if (bShowMouseCursor)
	{
		bIsToggling = true;
		LastToggleTime = CurrentTime;
		InventoryComponent->CloseInventory();
		return;
	}
	
	if (!InventoryWidget)
	{
		if (InventoryWidgetClass)
		{
			InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			if (InventoryWidget)
			{
				UUI_Inventory* InventoryUI = Cast<UUI_Inventory>(InventoryWidget);
				if (InventoryUI && InventoryComponent)
				{
					InventoryUI->BindInventory(InventoryComponent);
				}
				InventoryWidget->AddToViewport();
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}
	
	bIsToggling = true;
	LastToggleTime = CurrentTime;
	InventoryComponent->OpenInventory();
}

void AAlex_PlayerController::OnInventoryToggled(bool bIsOpen)
{
	if (bIsOpen)
	{
		if (InventoryWidget)
		{
			if (UUI_Inventory* InventoryUI = Cast<UUI_Inventory>(InventoryWidget))
			{
				InventoryUI->UpdateInventory();
			}
		}

		FInputModeGameAndUI InputMode;
		if (InventoryWidget)
		{
			InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		SetIgnoreLookInput(true);
		
		if (GetCharacter())
		{
			if (GetCharacter()->GetCharacterMovement())
			{
				GetCharacter()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			}
		}
		
		bToggleCooldown = true;
		bIsToggling = false;
	}
	else
	{
		if (InventoryWidget)
		{
			InventoryWidget->RemoveFromParent();
			InventoryWidget = nullptr;
		}

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		SetIgnoreLookInput(false);
		
		bToggleCooldown = true;
		bIsToggling = false;
		return;
	}
	
	bIsToggling = false;
}