#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BedActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class CAY_FORTRESS_API ABedActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ABedActor();

	// -- Components --
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed|Visuals")
	TObjectPtr<UStaticMeshComponent> BedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed|Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	// -- Config --
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bed")
	FText InteractText;

	// -- Events --
	UFUNCTION(BlueprintImplementableEvent, Category = "Bed")
	void BP_OnBedUsed(AActor* User);

	// -- IInteractableInterface --
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;
};
