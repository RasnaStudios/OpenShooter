// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "OpenShooterPlayerState.h"

#include "Character/OpenShooterCharacter.h"
#include "Character/OpenShooterPlayerController.h"
#include "Net/UnrealNetwork.h"

void AOpenShooterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);    // This replicates the score

    // We need to replicate the defeats so that the client can show the defeats in the HUD
    DOREPLIFETIME(AOpenShooterPlayerState, Defeats);
    DOREPLIFETIME_CONDITION(AOpenShooterPlayerState, AnnoucementMessage, COND_OwnerOnly);
}

void AOpenShooterPlayerState::AddToScore(const float Amount)
{
    SetScore(GetScore() + Amount);

    const float CurrentScore = GetScore();

    // Update the HUD to reflect the new score
    UpdateHUD(&CurrentScore, nullptr);
}

void AOpenShooterPlayerState::AddToDefeats(const int32 Amount)
{
    Defeats += Amount;

    // Update the HUD to reflect the new defeats
    UpdateHUD(nullptr, &Defeats);
}

void AOpenShooterPlayerState::OnRep_Score()
{
    Super::OnRep_Score();

    const float CurrentScore = GetScore();

    UpdateHUD(&CurrentScore, nullptr);
}

void AOpenShooterPlayerState::OnRep_Defeats()
{
    // Update the HUD when the replicated defeats value changes
    UpdateHUD(nullptr, &Defeats);
}

void AOpenShooterPlayerState::OnRep_AnnounceMessage()
{
    Character = Character ? Character : Cast<AOpenShooterCharacter>(GetPawn());
    if (Character)
    {
        Controller = Controller ? Controller : Cast<AOpenShooterPlayerController>(Character->GetController());
        if (Controller)
        {
            Controller->SetHUDAnnoucement(AnnoucementMessage);
        }
    }
}

void AOpenShooterPlayerState::SetAnnoucementMessage(const FString& Message)
{
    AnnoucementMessage = Message;    // triggers OnRep_AnnounceMessage
    Character = Character ? Character : Cast<AOpenShooterCharacter>(GetPawn());
    if (Character)
    {
        Controller = Controller ? Controller : Cast<AOpenShooterPlayerController>(Character->GetController());
        if (Controller)
        {
            Controller->SetHUDAnnoucement(AnnoucementMessage);
        }
    }
}

void AOpenShooterPlayerState::ClearAnnoucementMessage()
{
    AnnoucementMessage = "";    // triggers OnRep_AnnounceMessage
    Character = Character ? Character : Cast<AOpenShooterCharacter>(GetPawn());
    if (Character)
    {
        Controller = Controller ? Controller : Cast<AOpenShooterPlayerController>(Character->GetController());
        if (Controller)
        {
            Controller->ClearAnnoucementText();
        }
    }
}

void AOpenShooterPlayerState::UpdateHUD(const float* NewScore, const int32* NewDefeats)
{
    // Cache the character and controller only once if not already cached
    if (!Character)
    {
        Character = Cast<AOpenShooterCharacter>(GetPawn());
    }

    if (Character && !Controller)
    {
        Controller = Cast<AOpenShooterPlayerController>(GetOwner());
    }

    if (Controller)
    {
        // Update the HUD with the new score if provided
        if (NewScore)
        {
            Controller->SetHUDScore(*NewScore);
        }

        // Update the HUD with the new defeats if provided
        if (NewDefeats)
        {
            Controller->SetHUDDefeats(*NewDefeats);
        }
    }
}
