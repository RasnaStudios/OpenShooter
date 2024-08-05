// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/CombatComponent.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCombatComponent, EquippedWeapon);
    DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
    UE_LOG(LogTemp, Warning, TEXT("Equipping Weapon"));
    if (Character == nullptr || Weapon == nullptr)
    {
        return;
    }

    EquippedWeapon = Weapon;
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
    EquippedWeapon->AttachToComponent(
        Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightHandSocket");
    EquippedWeapon->SetOwner(Character);

    // We need this for the lean/strafing animation on the locally controlled character
    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
    Character->bUseControllerRotationYaw = true;

    UE_LOG(LogTemp, Warning, TEXT("Weapon Equipped!"));
}

void UCombatComponent::SetAiming(const bool bIsAiming)
{
    // we set this even if it won't be replicated immediately to the server
    // this is because we want the client to know that the character is aiming
    // so that the animation can be updated immediately.
    // The server will eventually replicate this to all the clients
    bAiming = bIsAiming;

    // Then we call this function to tell the server to set the aiming state there
    // if we would use only the one below, the client would have to wait for the server to replicate the variable
    // to know that the character is aiming, which would cause a delay in the animation
    ServerSetAiming(bIsAiming);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
    if (EquippedWeapon && Character)
    {
        Character->GetCharacterMovement()->bOrientRotationToMovement = false;
        Character->bUseControllerRotationYaw = true;
    }
}

void UCombatComponent::ServerSetAiming_Implementation(const bool bIsAiming)
{
    // This is the function that the client calls to tell the server to set the aiming state
    bAiming = bIsAiming;
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...
}
