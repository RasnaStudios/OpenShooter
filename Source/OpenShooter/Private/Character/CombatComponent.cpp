// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/OpenShooterCharacter.h"
#include "Character/OpenShooterPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/OpenShooterHUD.h"
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
    DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);    // this matter only for the client
    DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

        // Set the current FOV and the default FOV
        if (Character->GetFollowCamera())
        {
            DefaultFOV = Character->GetFollowCamera()->FieldOfView;
            CurrentFOV = DefaultFOV;
        }
    }
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Character && Character->IsLocallyControlled())
    {
        SetHUDCrosshair(DeltaTime);

        // Obtain the hit target for the right hand rotation correction to aim at the crosshair
        FHitResult HitResult;
        TraceUnderCrosshair(HitResult);
        HitTarget = HitResult.ImpactPoint;

        // Set the FOV based on the aiming state
        InterpFOV(DeltaTime);

        // if aiming, change camera aperture and focal distance to avoid blur in foreground and background
        if (bAiming && Character->GetFollowCamera())
        {
            Character->GetFollowCamera()->PostProcessSettings.DepthOfFieldFstop = 32.f;    // fixes the blur in foreground
            Character->GetFollowCamera()->PostProcessSettings.DepthOfFieldFocalDistance =
                10000.f;    // fixes the blur in background
        }
    }
}

void UCombatComponent::EquipWeapon(AWeapon* Weapon)
{
    UE_LOG(LogTemp, Warning, TEXT("Equipping Weapon"));
    if (Character == nullptr || Weapon == nullptr)
        return;
    if (EquippedWeapon)
        EquippedWeapon->Drop();

    EquippedWeapon = Weapon;
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
    EquippedWeapon->AttachToComponent(
        Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightHandSocket");
    Character = Character ? Character : Cast<AOpenShooterCharacter>(GetOwner());
    EquippedWeapon->SetOwner(Character);
    EquippedWeapon->SetHUDAmmo();

    // We set the carried ammo (controller)
    if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
        CarriedAmmo =
            CarriedAmmoMap[EquippedWeapon->GetWeaponType()];    // this triggers OnRep_CarriedAmmo, and we need to update the HUD
    Controller = Controller ? Controller : Cast<AOpenShooterPlayerController>(Character->GetController());
    if (Controller)
        Controller->SetHUDCarriedAmmo(CarriedAmmo);

    // We need this for the lean/strafing animation on the locally controlled character
    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
    Character->bUseControllerRotationYaw = true;

    UE_LOG(LogTemp, Warning, TEXT("Weapon Equipped!"));
}

void UCombatComponent::OnRep_EquippedWeapon() const
{
    if (EquippedWeapon && Character)
    {
        // Even if these should be run on the server, we need to run them on the client as well
        // the reason is that SetWeaponState will handle physics and collision which might interfere with attaching the weapon
        // therefore we need to run this on the client as well so we are sure of the order of execution
        EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
        EquippedWeapon->AttachToComponent(
            Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "RightHandSocket");

        Character->GetCharacterMovement()->bOrientRotationToMovement = false;
        Character->bUseControllerRotationYaw = true;
    }
}

void UCombatComponent::OnRep_CombatState()
{    // all of the following is run on the client
    switch (CombatState)
    {
        case ECombatState::ECS_Reloading:
            HandleReload();
            break;
        default:
            break;
    }
}

void UCombatComponent::FireButtonPressed(const bool bButtonPressed)
{
    bFireButtonPressed = bButtonPressed;
    if (bFireButtonPressed)
    {
        Fire();
    }
}

void UCombatComponent::Fire()
{
    if (!CanFire())
        return;
    bCanFire = false;    // we set this to false to prevent the player from firing too quickly
    // We set it to true in the FireTimerFinished function

    // If we replace ServerFire with MulticastFire directly here, it will work only server and not on clients.
    // The reason is that clients do not have authority to call multicast functions directly; only the server can do that.
    ServerFire(HitTarget);

    // if we are shooting, we should increase the spread of the crosshair
    if (EquippedWeapon)
    {
        CrosshairShootingFactor = FMath::Clamp(CrosshairShootingFactor + 0.75f, 0.f, 10.f);

        // We start the timer to fire the next shot if the weapon is automatic
        if (EquippedWeapon->IsAutomatic())
            StartFireTimer();
    }
}

bool UCombatComponent::CanFire() const
{
    if (EquippedWeapon == nullptr || Character == nullptr)
        return false;

    if (EquippedWeapon->IsEmpty())
        return false;

    return true;
}

void UCombatComponent::StartFireTimer()
{
    if (EquippedWeapon == nullptr || Character == nullptr)
        return;

    Character->GetWorldTimerManager().SetTimer(
        FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->GetFireDelay(), false);
}
void UCombatComponent::FireTimerFinished()
{
    bCanFire = true;

    // needs to check if we still have the fire button pressed
    if (bFireButtonPressed && EquippedWeapon->IsAutomatic())
        Fire();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
    MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
    if (EquippedWeapon == nullptr)
        return;
    if (Character)
    {
        Character->PlayFireMontage(bAiming);
        EquippedWeapon->Fire(TraceHitTarget);
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
    // ReSharper disable once CppTooWideScope
    const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
        UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
    if (bScreenToWorld)
    {
        FVector Start = CrosshairWorldPosition;

        if (Character)
        {    // we push forward the start location to avoid hitting the character and characters behind the character
            const float DistanceToCharacter = FVector::Dist(Character->GetActorLocation(), Start);
            Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
        }

        const FVector End = CrosshairWorldPosition + CrosshairWorldDirection * TRACE_LENGTH;

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Character);
        QueryParams.AddIgnoredActor(EquippedWeapon);
        GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QueryParams);

        // We set this, so we can change the spread and the color of the crosshair
        OnTarget = HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractWithCrosshairInterface>();
    }
}

void UCombatComponent::SetHUDCrosshair(float DeltaSeconds)
{
    if (Character == nullptr || Character->Controller == nullptr)
        return;

    Controller = Controller == nullptr ? Cast<AOpenShooterPlayerController>(Character->Controller) : Controller;
    if (Controller)
    {
        HUD = HUD == nullptr ? Cast<AOpenShooterHUD>(Controller->GetHUD()) : HUD;
        if (HUD)
        {
            if (EquippedWeapon)
            {
                HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
                HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
                HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
                HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
                HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
            }
            else
            {
                HUDPackage.CrosshairsCenter = nullptr;
                HUDPackage.CrosshairsLeft = nullptr;
                HUDPackage.CrosshairsRight = nullptr;
                HUDPackage.CrosshairsBottom = nullptr;
                HUDPackage.CrosshairsTop = nullptr;
            }
            // Calculate the spread of the crosshair
            FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
            FVector2D VelocityMultiplierRange(0.f, 1.f);
            FVector Velocity = Character->GetVelocity();
            Velocity.Z = 0.f;

            CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
            // TODO: if crouching, we should probably increase the spread

            // If falling, we should increase the spread even more
            if (Character->GetCharacterMovement()->IsFalling())
            {
                CrosshairInAirVelocityFactor = FMath::FInterpTo(CrosshairInAirVelocityFactor, 2.25, DeltaSeconds, 2.25f);
            }
            else
            {    // when hitting the ground we want to go back to 0 very quickly
                CrosshairInAirVelocityFactor = FMath::FInterpTo(CrosshairInAirVelocityFactor, 0.f, DeltaSeconds, 30.f);
            }

            // If aiming, we should decrease the spread
            if (bAiming)
            {
                CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.4f, DeltaSeconds, 30.f);
            }
            else
            {
                CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaSeconds, 30.f);
            }

            // Color the crosshair red if we can interact with the object (aiming at a character)
            if (OnTarget)
            {
                HUDPackage.CrosshairColor = FLinearColor::Red;
                CrosshairOnTargetFactor = FMath::FInterpTo(CrosshairOnTargetFactor, 0.2f, DeltaSeconds, 30.f);
            }
            else
            {
                HUDPackage.CrosshairColor = FLinearColor::White;
                CrosshairOnTargetFactor = FMath::FInterpTo(CrosshairOnTargetFactor, 0.f, DeltaSeconds, 30.f);
            }

            // If shooting, we should increase the spread (in FireButtonPressed function we are increasing this value)
            CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaSeconds, 5.f);

            HUDPackage.CrosshairSpread = BaselineCrosshairSpread + CrosshairVelocityFactor + CrosshairInAirVelocityFactor -
                                         CrosshairAimFactor - CrosshairOnTargetFactor + CrosshairShootingFactor;
            HUD->SetHUDPackage(HUDPackage);
        }
    }
}

void UCombatComponent::Reload()
{    // We need to check if we can reload on the server before we send an RPC to play the reload on all clients.
    // Therefore we use the ServerReload function

    // To avoid sending useless calls to the server, we check if there are any rounds in the carried ammo
    if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
    {
        ServerReload();
    }
}

void UCombatComponent::ServerReload_Implementation()
{
    if (Character == nullptr || EquippedWeapon == nullptr)
        return;
    CombatState = ECombatState::ECS_Reloading;
    HandleReload();
}

void UCombatComponent::HandleReload()
{
    Character->PlayReloadMontage();
}

void UCombatComponent::FinishReloading()
{    // This is called from the animation blueprint AnimBP_EpicCharacter
    if (Character && Character->HasAuthority())
        CombatState = ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InterpFOV(const float DeltaSeconds)
{
    if (EquippedWeapon == nullptr)
        return;

    // The weapon controls how much the FOV should be zoomed when aiming
    // but when not aiming, we want to go back to the default FOV and the default interp speed

    if (bAiming)
    {
        CurrentFOV =
            FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaSeconds, EquippedWeapon->GetZoomedInterpSpeed());
    }
    else
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaSeconds, ZoomedInterpSpeed);
    }
    if (Character && Character->GetFollowCamera())
    {
        Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
    }
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

void UCombatComponent::OnRep_CarriedAmmo()
{    // we need to update the HUD on the client when the carried ammo changes
    Controller = Controller ? Controller : Cast<AOpenShooterPlayerController>(Character->GetController());
    if (Controller)
        Controller->SetHUDCarriedAmmo(CarriedAmmo);
}
