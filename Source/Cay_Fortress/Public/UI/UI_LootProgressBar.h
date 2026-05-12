#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_LootProgressBar.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

UCLASS()
class CAY_FORTRESS_API UUI_LootProgressBar : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetProgress(float Percent);
	void SetCountdownValue(float Seconds);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ProgressRing;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ProgressMaterial;
};
