// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/CombatComponent.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

#define TRACE_LENGTH 80000.f;

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;    // We want to tick this component every frame for the firing logic

    BaseWalkSpeed = 600.0f;
    AimWalkSpeed = 400.0f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCombatComponent, EquippedWeapon);
    DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (Character)
        Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    FHitResult HitResult;
    TraceUnderCrosshair(HitResult);
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

    UE_LOG(LogTemp, Warning, TEXT("Setting Aiming %d"), bIsAiming);

    // Set the walk speed based on the aiming state
    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}

void UCombatComponent::ServerSetAiming_Implementation(const bool bIsAiming)
{
    // This is the function that the client calls to tell the server to set the aiming state
    bAiming = bIsAiming;

    // Set the walk speed based on the aiming state
    if (Character)
        Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::OnRep_EquippedWeapon() const
{
    if (EquippedWeapon && Character)
    {
        Character->GetCharacterMovement()->bOrientRotationToMovement = false;
        Character->bUseControllerRotationYaw = true;
    }
}

void UCombatComponent::Fire(const bool bButtonPressed)
{
    bFireButtonPressed = bButtonPressed;
    if (bFireButtonPressed)
    {
        // If we replace ServerFire with MulticastFire directly here, it will work only server and not on clients.
        // The reason is that clients do not have authority to call multicast functions directly; only the server can do that.
        ServerFire();
    }
}

void UCombatComponent::ServerFire_Implementation()
{
    MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
    if (EquippedWeapon == nullptr)
        return;
    if (Character)
    {
        Character->PlayFireMontage(bAiming);
        EquippedWeapon->Fire(HitTarget);
    }
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult)
{
    // We trace from the center of the screen (crosshair)
    FVector2D ViewportSize;
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }
    // Crosshair is the center of the viewport
    const FVector2D CrosshairLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
    FVector CrosshairWorldPosition;
    FVector CrosshairWorldDirection;
    bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
        UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
    if (bScreenToWorld)
    {
        const FVector Start = CrosshairWorldPosition;
        const FVector End = CrosshairWorldPosition + CrosshairWorldDirection * TRACE_LENGTH;

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Character);
        QueryParams.AddIgnoredActor(EquippedWeapon);
        GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QueryParams);

        // If we didn't hit anything, we set the impact point to the end of the trace
        if (!HitResult.bBlockingHit)
        {
            HitResult.ImpactPoint = End;
            HitTarget = End;
        }
        else
        {
            HitTarget = HitResult.ImpactPoint;
            DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.f, 12, FColor::Red);
        }
    }
}
