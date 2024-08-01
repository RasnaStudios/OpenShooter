// Copyright (c) 2023-2024 Rasna Studios. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "Menu.generated.h"

class UMultiplayerSessionsSubsystem;
class UButton;

/**
 * UMenu class is a user interface widget that provides functionality for hosting and joining multiplayer sessions.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeDestruct() override;

    /**
     * Sets up the menu. This function is callable from Blueprints.
     */
    UFUNCTION(BlueprintCallable)
    void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")),
        FString LobbyPath = FString(TEXT("/Game/Maps/Lobby")));

protected:
    /**
     * Initializes the widget. This function is called when the widget is constructed.
     * @return true if the initialization is successful, false otherwise.
     */
    virtual bool Initialize() override;

    //
    // Callbacks for the custom delegates on the MultiplayerSessionSubsystem
    // These will be called in this class to handle the results of the session operations
    //
    UFUNCTION()
    void OnCreateSession(bool bWasSuccessful);
    // not UFUNCTION() because the delegate is not dynamic
    void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
    // not UFUNCTION() because the delegate is not dynamic
    void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
    UFUNCTION()
    void OnDestroySession(bool bWasSuccessful);
    UFUNCTION()
    void OnStartSession(bool bWasSuccessful);

private:
    // These buttons are binded in the blueprint and the names must match the names in the blueprint

    /** Button to host a multiplayer session. */
    UPROPERTY(meta = (BindWidget))
    UButton* HostButton;

    /** Button to join a multiplayer session. */
    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton;

    // These functions must be UFUNCTION() to be binded in the blueprint

    // Function called when the HostButton is clicked.
    UFUNCTION()
    void HostButtonClicked();

    // Function called when the JoinButton is clicked.
    UFUNCTION()
    void JoinButtonClicked();

    void MenuTearDown();

    /** Subsystem for handling multiplayer sessions. */
    UPROPERTY()
    UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    int32 NumPublicConnections{4};
    FString MatchType{TEXT("FreeForAll")};
    FString PathToLobby{TEXT("")};
};
