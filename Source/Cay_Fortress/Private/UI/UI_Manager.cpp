// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_Manager.h"
#include "UI/UI_Base_Class.h"

UUI_Base_Class* UUI_Manager::OpenUI(const FName& UIName, bool bAddToViewport)
{
	if (UUI_Base_Class* ExistingUI = GetUI(UIName))
	{
		ExistingUI->OpenUI();
		return ExistingUI;
	}

	return nullptr;
}

void UUI_Manager::CloseUI(const FName& UIName)
{
	if (UUI_Base_Class* ExistingUI = GetUI(UIName))
	{
		ExistingUI->CloseUI();
		OnUIUpdate.Broadcast(UIName);
	}
}

void UUI_Manager::ToggleUI(const FName& UIName)
{
	if (UUI_Base_Class* ExistingUI = GetUI(UIName))
	{
		if (ExistingUI->IsUIOpen())
		{
			ExistingUI->CloseUI();
		}
		else
		{
			ExistingUI->OpenUI();
		}
		OnUIUpdate.Broadcast(UIName);
	}
}

UUI_Base_Class* UUI_Manager::GetUI(const FName& UIName) const
{
	return OpenedUIs.FindRef(UIName);
}

void UUI_Manager::UpdateAllUI()
{
	for (auto& Pair : OpenedUIs)
	{
		if (Pair.Value)
		{
			Pair.Value->UpdateUI();
		}
	}
}
