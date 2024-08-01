// Copyright (c) 2023-2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * UMultiplayerSessionsSubsystem class is a game instance subsystem that provides functionality for handling multiplayer sessions.
 */

/**
 * Custom delegates for handling multiplayer session events.
 * These delegates serve as communication channels between the MultiplayerSessionsSubsystem and the Menu class.
 *
 * The Menu class will:
 * 1. Call the public session management functions (CreateSession, FindSessions, JoinSession, DestroySession, StartSession) in this
 * subsystem.
 * 2. Bind its own functions to these delegates to handle the results of the session operations.
 *
 * When a session operation completes, the corresponding delegate will be broadcast,
 * allowing the Menu class to respond appropriately to the success or failure of each operation.
 *
 * Note:
 * - The delegate signature must match the function signature of the bound function in the Menu class.
 * - The functions in the Menu class must be declared as UFUNCTION() to be bound to the delegate.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FMultiplayerOnFindSessionComplete, const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

// Note, the second and the third delegates are not dynamic because they use parameters that are not supported by dynamic delegates.

UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UMultiplayerSessionsSubsystem();

    //
    // Functions to handle session functionalities
    // The Menu class will call these
    //

    void CreateSession(int32 NumPublicConnections, FString MatchType);
    void FindSessions(int32 MaxSearchResults);
    void JoinSession(const FOnlineSessionSearchResult& SearchResult);
    void DestroySession();
    void StartSession();

    //
    // Our own custom delegates for the Menu class to bind callbacks to
    //
    FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
    FMultiplayerOnFindSessionComplete MultiplayerOnFindSessionsComplete;
    FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
    FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
    FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:
    //
    // Internal callbacks for the delegates we will add to the Online Session Interface delegate list
    // These don't need to be called outside of this class
    //

    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
    IOnlineSessionPtr SessionInterface;

    // The last session settings used to create a session
    TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

    //
    // To add to the Online Session Interface delegate list
    // We will bind our MultiplayerSessionsSystem internal callbacks to these.
    // For each of the delegates we need a delegate handle to add and remove them
    //

    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FDelegateHandle CreateSessionCompleteDelegateHandle;

    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;

    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;

    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;

    FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
    FDelegateHandle StartSessionCompleteDelegateHandle;

    // Variables to store the last session settings used to create a session
    // so that we can use them to know if we need to create a session on destroy
    bool bCreateSessionOnDestroy{false};
    int32 LastNumPublicConnections{0};
    FString LastMatchType{TEXT("")};
};
