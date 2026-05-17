#include "UI/UI_InteractPrompt.h"
#include "Components/TextBlock.h"

void UUI_InteractPrompt::NativeConstruct()
{
	Super::NativeConstruct();
	if (PromptText)
		PromptText->SetText(FText::FromString(TEXT("E")));
}
