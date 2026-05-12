#include "UI/UI_LootProgressBar.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UUI_LootProgressBar::NativeConstruct()
{
	Super::NativeConstruct();

	if (ProgressRing)
	{
		ProgressMaterial = ProgressRing->GetDynamicMaterial();
	}
}

void UUI_LootProgressBar::SetProgress(float Percent)
{
	if (ProgressMaterial)
	{
		ProgressMaterial->SetScalarParameterValue(FName("Progress"), Percent);
	}
}

void UUI_LootProgressBar::SetCountdownValue(float Seconds)
{
	if (CountdownText)
	{
		CountdownText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), FMath::Max(0.0f, Seconds))));
	}
}
