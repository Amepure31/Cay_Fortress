#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Alex_PlayerController.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UPlayerInteractComponent;
class UUserWidget;
class ALootContainerActor;

UCLASS()
class CAY_FORTRESS_API AAlex_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AAlex_PlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump();
	void Run(const FInputActionValue& Value);
	void StopRun();
	void ToggleInventory();
	void Interact();
	/** @param bShowEquipment 为 false 时仅显示背包（如搜刮容器），不创建/显示装备栏。 */
	bool EnsureInventoryUIVisible(bool bShowEquipment = true);
	/** 与背包同时显示装备栏（创建/入视口/可见性/格子同步）。 */
	void EnsureEquipmentUIVisibleWithInventory();
	void OpenLootContainerUI(ALootContainerActor* LootContainer);
	void CloseLootContainerUI(bool bCloseInventoryIfOpenedByLootInteraction);

protected:
	UFUNCTION()
	void OnInventoryToggled(bool bIsOpen);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RunAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> LootContainerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InventoryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> EquipmentWidgetClass;

	UPROPERTY()
	UUserWidget* InventoryWidget;

	UPROPERTY()
	UUserWidget* EquipmentWidget;

	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	UPROPERTY()
	UPlayerInteractComponent* PlayerInteractComponent;

	UPROPERTY()
	UUserWidget* LootContainerWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<ALootContainerActor> ActiveLootContainer;

	bool bLootInteractionActive;
	bool bInventoryOpenedByLootInteraction;

	bool bIsToggling;
	
	float LastToggleTime;
	bool bToggleCooldown;
};