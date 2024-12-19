// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "OpenShooterPlayerState.h"

#include "Character/OpenShooterCharacter.h"
#include "Character/OpenShooterPlayerController.h"

void AOpenShooterPlayerState::AddToScore(float Amount)
{
    SetScore(GetScore() + Amount);
    Character = Character == nullptr ? Cast<AOpenShooterCharacter>(GetPawn()) : Character;
    if (Character != nullptr)
    {
        Controller = Controller == nullptr ? Cast<AOpenShooterPlayerController>(GetOwner()) : Controller;
        if (Controller != nullptr)
        {
            Controller->SetHUDScore(GetScore());
        }
    }
}

void AOpenShooterPlayerState::OnRep_Score()
{
    Super::OnRep_Score();
    // This will be called many times, so we don't want to cast to the character every time
    // Therefore, we will cache the character and controller
    Character = Character == nullptr ? Cast<AOpenShooterCharacter>(GetPawn()) : Character;
    if (Character != nullptr)
    {
        Controller = Controller == nullptr ? Cast<AOpenShooterPlayerController>(GetOwner()) : Controller;
        if (Controller != nullptr)
        {
            Controller->SetHUDScore(GetScore());
        }
    }
}
