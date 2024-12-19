// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/OpenShooterPlayerController.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/OpenShooterHUD.h"

void AOpenShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    HUD = Cast<AOpenShooterHUD>(GetHUD());
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
