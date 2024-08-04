// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/CombatComponent.h"

#include "Character/OpenShooterCharacter.h"
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
    UE_LOG(LogTemp, Warning, TEXT("Weapon Equipped!"));
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...
}
