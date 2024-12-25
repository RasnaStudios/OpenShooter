// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "CharacterOverlay.generated.h"

class URichTextBlock;
class UTextBlock;
class UProgressBar;
/**
 *
 */
UCLASS()
class OPENSHOOTER_API UCharacterOverlay : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthText;

    // Kills
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreAmount;

    // Deaths
    UPROPERTY(meta = (BindWidget))
    UTextBlock* DefeatsAmount;

    UPROPERTY(meta = (BindWidget))
    URichTextBlock* AnnouncementText;
};
