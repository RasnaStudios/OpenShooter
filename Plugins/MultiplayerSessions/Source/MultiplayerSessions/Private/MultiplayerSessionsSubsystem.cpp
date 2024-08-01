// Copyright (c) 2023-2024 Rasna Studios. All rights reserved.

#include "MultiplayerSessionsSubsystem.h"

#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
    : CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete))
    , FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete))
    , JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
    , DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
    , StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
    // Get the session interface from the online subsystem
    if (const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        SessionInterface = Subsystem->GetSessionInterface();
    }
}

// Functions to handle session functionalities

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }
    // Destroy the existing session if it exists
    if (const auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession); ExistingSession != nullptr)
    {
        // We need to destroy the session on destroy, so we can create a new one
        bCreateSessionOnDestroy = true;
        LastNumPublicConnections = NumPublicConnections;
        LastMatchType = MatchType;
        DestroySession();
    }

    // Store the delegate handle, so we can remove it later from the delegate list
    SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

    // We create the session settings
    LastSessionSettings = MakeShared<FOnlineSessionSettings>();

    // If there's a subsystem then the match is LAN, otherwise it's online
    LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
    LastSessionSettings->NumPublicConnections =
        NumPublicConnections;    // Number of players that can join the session (not the number of players in the game)
    LastSessionSettings->bAllowJoinInProgress = true;     // Allow players to join the session even if it's already started
    LastSessionSettings->bAllowJoinViaPresence = true;    // Allow players to join the session via presence (friends list)
    LastSessionSettings->bShouldAdvertise = true;    // Advertise the session to the online subsystem so other players can find it
    LastSessionSettings->bUsesPresence = true;       // Use presence (friends list) to find the session
    LastSessionSettings->bUseLobbiesIfAvailable = true;    // Use lobbies if available
    LastSessionSettings->Set(
        FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);    // Set the match type
    LastSessionSettings->BuildUniqueId = 1;    // Generate a new unique ID for the session

    if (const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer(); LocalPlayer != nullptr)
    {
        // Create the session and if it fails, remove the delegate handle and broadcast the custom delegate
        if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
        {
            // Remove the delegate handle
            SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

            // Broadcast our own custom delegate
            MultiplayerOnCreateSessionComplete.Broadcast(false);
        }
    }
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }
    // Store the delegate handle, so we can remove it later from the delegate list
    SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

    // We create the session search settings
    LastSessionSearch = MakeShared<FOnlineSessionSearch>();
    LastSessionSearch->MaxSearchResults = MaxSearchResults;    // Maximum number of search results
    LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() ==
                                     "NULL";    // If there's a subsystem then the match is LAN, otherwise it's online
    LastSessionSearch->QuerySettings.Set(
        SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);    // Search for sessions with presence (friends list)

    const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
    if (SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
    {
        // If the search fails, remove the delegate handle and broadcast the custom delegate
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
        // Broadcast the custom delegate with an empty array because we didn't find any sessions
        MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
    }
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SearchResult)
{
    if (!SessionInterface.IsValid())
    {
        MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
        UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
        return;
    }

    // Store the delegate handle, so we can remove it later from the delegate list
    SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

    // Join the session
    const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
    if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult))
    {
        // If the join fails, remove the delegate handle and broadcast the custom delegate with an error
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
    }
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
    if (!SessionInterface.IsValid())
    {
        MultiplayerOnDestroySessionComplete.Broadcast(false);
        UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
        return;
    }
    // Store the delegate handle, so we can remove it later from the delegate list
    SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

    // Destroy the session
    if (!SessionInterface->DestroySession(NAME_GameSession))
    {
        // If the destroy fails, remove the delegate handle and broadcast the custom delegate with an error
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        MultiplayerOnDestroySessionComplete.Broadcast(false);
    }
}

void UMultiplayerSessionsSubsystem::StartSession()
{
    if (!SessionInterface.IsValid())
    {
        MultiplayerOnStartSessionComplete.Broadcast(false);
        UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
        return;
    }

    // Store the delegate handle, so we can remove it later from the delegate list
    SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

    // Start the session
    if (!SessionInterface->StartSession(NAME_GameSession))
    {
        // If the start fails, remove the delegate handle and broadcast the custom delegate with an error
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
        MultiplayerOnStartSessionComplete.Broadcast(false);
    }
}

// Callbacks for delegates

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    // Remove the delegate handle
    if (SessionInterface)
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

    // Broadcast our own custom delegate. The menu will receive the value of bWasSuccessful
    MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
    // Remove the delegate handle
    if (SessionInterface)
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

    if (LastSessionSearch->SearchResults.Num() <= 0)
    {
        // Broadcast our own custom delegate. The menu will receive an empty array and false
        MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
        return;
    }

    // Broadcast our own custom delegate. The menu will receive the search results and the value of bWasSuccessful
    MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    // Remove the delegate handle
    if (SessionInterface)
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

    // Broadcast our own custom delegate. The menu will receive the result of the join operation
    MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    // Remove the delegate handle
    if (SessionInterface)
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

    // Check if we need to create a session after destroying the current one
    if (bWasSuccessful && bCreateSessionOnDestroy)
    {
        bCreateSessionOnDestroy = false;
        // Create a new session with the last settings
        CreateSession(LastNumPublicConnections, LastMatchType);
    }
    // Broadcast our own custom delegate. The menu will receive the value of bWasSuccessful
    MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
    // Remove the delegate handle
    if (SessionInterface)
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);

    // Broadcast our own custom delegate. The menu will receive the value of bWasSuccessful
    MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}
