#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_InteractPrompt.generated.h"

class UTextBlock;

UCLASS()
class CAY_FORTRESS_API UUI_InteractPrompt : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> PromptText;
};
