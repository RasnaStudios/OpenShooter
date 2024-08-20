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
        FHitResult HitResult;
        TraceUnderCrosshair(HitResult);
        // If we replace ServerFire with MulticastFire directly here, it will work only server and not on clients.
        // The reason is that clients do not have authority to call multicast functions directly; only the server can do that.
        ServerFire(HitResult.ImpactPoint);

        // if we are shooting, we should increase the spread of the crosshair
        if (EquippedWeapon)
        {
            CrosshairShootingFactor = FMath::Clamp(CrosshairShootingFactor + 0.75f, 0.f, 10.f);
        }
    }
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
            FHUDPackage HUDPackage;
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
                CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaSeconds, 30.f);
            }
            else
            {
                CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaSeconds, 30.f);
            }

            // If shooting, we should increase the spread (in Fire function we are increasing this value)
            CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaSeconds, 5.f);

            HUDPackage.CrosshairSpread = BaselineCrosshairSpread + CrosshairVelocityFactor + CrosshairInAirVelocityFactor -
                                         CrosshairAimFactor + CrosshairShootingFactor;
            HUD->SetHUDPackage(HUDPackage);
        }
    }
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
