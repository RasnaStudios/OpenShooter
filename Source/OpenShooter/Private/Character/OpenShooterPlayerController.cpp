// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/OpenShooterPlayerController.h"

#include "Components/ProgressBar.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/OpenShooterHUD.h"

void AOpenShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    HUD = Cast<AOpenShooterHUD>(GetHUD());
    HideAnnoucementText();
}

void AOpenShooterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    HUD = HUD == nullptr ? Cast<AOpenShooterHUD>(GetHUD()) : HUD;

    if (HUD && HUD->CharacterOverlay && HUD->CharacterOverlay->HealthBar && HUD->CharacterOverlay->HealthText)
    {
        const float HealthPercentage = Health / MaxHealth;
        HUD->CharacterOverlay->HealthBar->SetPercent(HealthPercentage);
        const FText HealthText =
            FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth)));
        HUD->CharacterOverlay->HealthText->SetText(HealthText);
    }
}

void AOpenShooterPlayerController::SetHUDScore(float Score)
{
    HUD = HUD == nullptr ? Cast<AOpenShooterHUD>(GetHUD()) : HUD;

    if (HUD && HUD->CharacterOverlay && HUD->CharacterOverlay->ScoreAmount)
    {
        const FText ScoreText = FText::FromString(FString::Printf(TEXT("%d"), FMath::FloorToInt(Score)));
        HUD->CharacterOverlay->ScoreAmount->SetText(ScoreText);
    }
}

void AOpenShooterPlayerController::SetHUDDefeats(int32 Defeats)
{
    HUD = HUD == nullptr ? Cast<AOpenShooterHUD>(GetHUD()) : HUD;

    if (HUD && HUD->CharacterOverlay && HUD->CharacterOverlay->DefeatsAmount)
    {
        const FText DefeatsText = FText::FromString(FString::Printf(TEXT("%d"), Defeats));
        HUD->CharacterOverlay->DefeatsAmount->SetText(DefeatsText);
    }
}

void AOpenShooterPlayerController::SetHUDAnnoucement(const FText& Message, float DisplayTime)
{
    HUD = HUD == nullptr ? Cast<AOpenShooterHUD>(GetHUD()) : HUD;

    if (HUD && HUD->CharacterOverlay && HUD->CharacterOverlay->AnnouncementText)
    {
        HUD->CharacterOverlay->AnnouncementText->SetText(Message);
        HUD->CharacterOverlay->AnnouncementText->SetVisibility(ESlateVisibility::Visible);

        // Start a timer to hide the text after DisplayTime seconds
        GetWorldTimerManager().ClearTimer(HideAnnoucementTextTimerHandle);    // Clear any existing timers
        GetWorldTimerManager().SetTimer(
            HideAnnoucementTextTimerHandle, this, &AOpenShooterPlayerController::HideAnnoucementText, DisplayTime, false);
    }
}

void AOpenShooterPlayerController::HideAnnoucementText()
{
    HUD = HUD == nullptr ? Cast<AOpenShooterHUD>(GetHUD()) : HUD;

    if (HUD && HUD->CharacterOverlay && HUD->CharacterOverlay->AnnouncementText)
    {
        HUD->CharacterOverlay->AnnouncementText->SetVisibility(ESlateVisibility::Hidden);
    }
}
