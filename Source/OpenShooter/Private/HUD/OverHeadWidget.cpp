// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "HUD/OverHeadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverHeadWidget::SetDisplayText(const FString& Text) const
{
    if (DisplayText)
        DisplayText->SetText(FText::FromString(Text));
}

void UOverHeadWidget::ShowPlayerName(APawn* PlayerPawn) const
{
    // Set the text to the player's name
    if (PlayerPawn)
    {
        FString PlayerName = "Player unknown";
        if (const APlayerState* PlayerState = PlayerPawn->GetPlayerState())
        {
            PlayerName = PlayerState->GetPlayerName();
        }
        SetDisplayText(PlayerName);
    }
}

void UOverHeadWidget::NativeDestruct()
{
    RemoveFromParent();
    Super::NativeDestruct();
}
