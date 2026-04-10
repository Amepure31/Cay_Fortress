// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_Base_Class.h"

void UUI_Base_Class::InitializeUI()
{
	bIsOpen = false;
}

void UUI_Base_Class::OpenUI()
{
	if (!bIsOpen)
	{
		bIsOpen = true;
		UpdateUI();
	}
}

void UUI_Base_Class::CloseUI()
{
	if (bIsOpen)
	{
		bIsOpen = false;
	}
}

void UUI_Base_Class::UpdateUI()
{
	if (bIsOpen)
	{
		OnUIUpdateDelegate.Broadcast();
	}
}

bool UUI_Base_Class::IsUIOpen() const
{
	return bIsOpen;
}

void UUI_Base_Class::SetUIName(const FName& InUIName)
{
	UIName = InUIName;
}

FName UUI_Base_Class::GetUIName() const
{
	return UIName;
}
