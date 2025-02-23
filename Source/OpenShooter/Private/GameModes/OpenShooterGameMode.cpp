// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/OpenShooterGameMode.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "OpenShooterPlayerState.h"
#include "UObject/ConstructorHelpers.h"

AOpenShooterGameMode::AOpenShooterGameMode()
{
    bDelayedStart = true;
    /**
     * When we set this to true the game will stay in the WaitingToStart state until we call StartMatch().
     * Until then, the player will be flying pawns that can roam around the map.
     */
}

void AOpenShooterGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (MatchState == MatchState::WaitingToStart)
    {
        // Begins a one-shot timer to start the match when warmup time expires.
        GetWorldTimerManager().SetTimer(WarmupTimerHandle, this, &AOpenShooterGameMode::StartMatch, WarmupTime, false);
    }
}

void AOpenShooterGameMode::OnMatchStateSet()
{
    Super::OnMatchStateSet();

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        AOpenShooterPlayerController* PlayerController = Cast<AOpenShooterPlayerController>(Iterator->Get());
        if (PlayerController)
        {
            PlayerController->OnMatchStateSet(MatchState);
        }
    }
}

void AOpenShooterGameMode::PlayerEliminated(AOpenShooterCharacter* EliminatedCharacter,
    AOpenShooterPlayerController* VictimController, AOpenShooterPlayerController* AttackerController)
{
    AOpenShooterPlayerState* AttackerPlayerState =
        AttackerController ? Cast<AOpenShooterPlayerState>(AttackerController->PlayerState) : nullptr;
    AOpenShooterPlayerState* VictimPlayerState =
        VictimController ? Cast<AOpenShooterPlayerState>(VictimController->PlayerState) : nullptr;

    if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
    {
        AttackerPlayerState->AddToScore(1.0f);
    }

    if (VictimPlayerState)
    {
        VictimPlayerState->AddToDefeats(1);
    }

    if (EliminatedCharacter)
        EliminatedCharacter->Eliminate();
}

void AOpenShooterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AOpenShooterPlayerController* PlayerController)
{
    if (EliminatedCharacter)
    {
        EliminatedCharacter->Reset();      // it detaches the character from the controller
        EliminatedCharacter->Destroy();    // this is the reason why we use playerstate and gamestate to store the player's data
    }
    // We respawn the player at a random player start among all the available player starts positioned in the map
    if (PlayerController)
    {
        TArray<AActor*> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

        const int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
        RestartPlayerAtPlayerStart(PlayerController, PlayerStarts[Selection]);
    }
}
