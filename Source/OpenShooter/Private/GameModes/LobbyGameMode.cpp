// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "GameModes/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (const int32 NumberOfPlayers = GameState->PlayerArray.Num(); NumberOfPlayers >= 2)
    {
        if (UWorld* World = GetWorld())
        {
            bUseSeamlessTravel = true;
            World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
        }
    }
}
