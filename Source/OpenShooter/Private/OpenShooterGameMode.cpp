// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenShooterGameMode.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "OpenShooterPlayerState.h"
#include "UObject/ConstructorHelpers.h"

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
