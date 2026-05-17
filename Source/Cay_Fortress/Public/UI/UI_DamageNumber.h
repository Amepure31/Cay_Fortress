#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_DamageNumber.generated.h"

class UTextBlock;

UCLASS()
class CAY_FORTRESS_API UUI_DamageNumber : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DamageNumber")
	void SetDamage(float Damage, bool bHeadshot);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> DamageText;

	UPROPERTY(BlueprintReadWrite, Category = "DamageNumber")
	float Lifetime = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "DamageNumber")
	float FloatSpeed = 40.0f;

private:
	float Elapsed = 0.0f;
	float FloatOffset = 0.0f;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
