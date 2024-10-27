// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/OpenShooterCharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/CombatComponent.h"
#include "Character/OpenShooterPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "OpenShooter.h"
#include "OpenShooterGameMode.h"
#include "Sound/SoundCue.h"
#include "Weapon/Weapon.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AOpenShooterCharacter

AOpenShooterCharacter::AOpenShooterCharacter()
{
    PrimaryActorTick.bCanEverTick = true;    // we need this for combat component to tick

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;               // Character moves in the direction of input...
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);    // ...at this rotation rate

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetMesh());
    CameraBoom->TargetArmLength = 600.0f;          // The camera follows at this distance behind the character
    CameraBoom->bUsePawnControlRotation = true;    // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(
        CameraBoom, USpringArmComponent::SocketName);    // Attach the camera to the end of the boom and let the boom adjust to
                                                         // match the controller orientation
    FollowCamera->bUsePawnControlRotation = false;       // Camera does not rotate relative to arm

    bUseControllerRotationYaw = false;    // We don't want the controller rotating the camera
    GetCharacterMovement()->bOrientRotationToMovement = true;

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
    // are set in the derived blueprint asset named BP_OpenShooterCharacter (to avoid direct content references in C++)

    OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
    OverHeadWidget->SetupAttachment(GetRootComponent());

    Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
    Combat->SetIsReplicated(true);    // This is enough to replicate the component
    // We want the combat component to replicate because it has replicated variables.
    // The component itself needs to be replicated for it to have replicated variables.

    // Enable crouching
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->SetCrouchedHalfHeight(60.0f);
    GetCharacterMovement()->MaxWalkSpeedCrouched = 200.0f;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 850.0f, 0.0f);

    GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

    // Avoid blocking the camera with other characters capsule
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    // We block visibility to allow to aim at other characters
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

    // Set character to not rotate in place initially
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;

    // Set net update frequency
    NetUpdateFrequency = 66.0f;
    MinNetUpdateFrequency = 33.0f;

    // Make the character always spawn even if colliding with other characters in the same spawn point
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Dissolve effect when killed
    DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline"));
}

void AOpenShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // We need to replicate the OverlappingWeapon so that the client can show the pickup widget, but only the owner can interact
    // with it so the widget is only shown on the client that owns the character
    DOREPLIFETIME_CONDITION(AOpenShooterCharacter, OverlappingWeapon, COND_OwnerOnly);

    DOREPLIFETIME(AOpenShooterCharacter, Health);
}

void AOpenShooterCharacter::BeginPlay()
{
    // Call the base class
    Super::BeginPlay();

    // We set the health of the character to the max health
    UpdateHUDHealth();

    // We bind the OnTakeAnyDamage event to the ReceiveDamage function only for server
    if (HasAuthority())
        OnTakeAnyDamage.AddDynamic(this, &AOpenShooterCharacter::ReceiveDamage);
}

void AOpenShooterCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    if (Combat)
        Combat->Character = this;
}

void AOpenShooterCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())    // authority or autonomous proxy, only locally controlled
    {
        // Setting the aim offset
        AimOffset(DeltaSeconds);
    }    // for simulated proxy it's done in OnRep_ReplicatedMovement which is run only when necessary
    else
    {
        TimeSinceLastMovementReplication += DeltaSeconds;
        // We keep track of this because if this value  reaches a certain amount, we force call OnRep_ReplicatedMovement
        if (TimeSinceLastMovementReplication > 0.25f)
        {
            OnRep_ReplicatedMovement();
        }

        // we calculate the pitch at every frame
        CalculateAimOffsetPitch();
    }

    HideCameraIfCharacterClose();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AOpenShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Add Input Mapping Context
    if (APlayerController* BasePlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(BasePlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AOpenShooterCharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOpenShooterCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOpenShooterCharacter::Look);

        // Equip Weapon
        EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &AOpenShooterCharacter::EquipPressed);

        // Crouch
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AOpenShooterCharacter::CrouchPressed);

        // Aim
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AOpenShooterCharacter::AimPressed);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AOpenShooterCharacter::AimReleased);

        // FireButtonPressed
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AOpenShooterCharacter::FirePressed);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AOpenShooterCharacter::FireReleased);
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error,
            TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you "
                 "intend to use the legacy system, then you will need to update this C++ file."),
            *GetNameSafe(this));
    }
}

// Called when the OverlappingWeapon is replicated to the client
// This is called ONLY in the client when the OverlappingWeapon is set on the server
void AOpenShooterCharacter::OnRep_OverlappingWeapon(const AWeapon* LastWeapon) const
{
    if (OverlappingWeapon)
        OverlappingWeapon->ShowPickupWidget(true);
    // hide the pickup widget for the last weapon
    // this is done because the client only knows about the weapon that is overlapping with the character
    if (LastWeapon)
        LastWeapon->ShowPickupWidget(false);
}

// Called when the character overlaps with a weapon on the server only
void AOpenShooterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
    // hide the pickup widget for the last weapon
    if (OverlappingWeapon)
        OverlappingWeapon->ShowPickupWidget(false);

    // set the overlapping weapon
    OverlappingWeapon = Weapon;
    // on the client, OnRep_OverlappingWeapon would be called, but not on the server
    // so we need to call it manually on the server
    if (IsLocallyControlled())
        if (OverlappingWeapon)
            OverlappingWeapon->ShowPickupWidget(true);
}

bool AOpenShooterCharacter::IsWeaponEquipped() const
{
    // Check if the character has a weapon equipped
    // This works only if EquippedWeapon is replicated
    return Combat && Combat->EquippedWeapon;
}

bool AOpenShooterCharacter::IsAiming() const
{
    return Combat && Combat->bAiming;
}

void AOpenShooterCharacter::PlayFireMontage(const bool bAiming) const
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
        return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && FireWeaponMontage)
    {
        AnimInstance->Montage_Play(FireWeaponMontage, 1.0f);
        const FName Section = bAiming ? FName("RifleAim") : FName("RifleHip");
        AnimInstance->Montage_JumpToSection(Section, FireWeaponMontage);
    }
}

void AOpenShooterCharacter::PlayHitReactMontage() const
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
        return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && HitReactMontage)
    {
        AnimInstance->Montage_Play(HitReactMontage, 1.0f);
        FName SectionName("FromFront");
        AnimInstance->Montage_JumpToSection(SectionName, HitReactMontage);
    }
}

void AOpenShooterCharacter::OnRep_ReplicatedMovement()
{
    Super::OnRep_ReplicatedMovement();

    // We need to run the simulated proxies' turn in place only when necessary instead of doing in tick (because tick is not
    // fast enough)
    SimProxiesTurn();
    TimeSinceLastMovementReplication = 0.f;
}

void AOpenShooterCharacter::Move(const FInputActionValue& Value)
{
    // input is a Vector2D
    const FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // get right vector
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AOpenShooterCharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AOpenShooterCharacter::Jump()
{
    if (bIsCrouched)
        UnCrouch();
    Super::Jump();
}

void AOpenShooterCharacter::EquipPressed()
{
    if (Combat)
    {
        // If it's the server, run local function
        if (HasAuthority())
            Combat->EquipWeapon(OverlappingWeapon);
        // If it's the client, run RPC
        else
            ServerEquipPressed();
    }
}

void AOpenShooterCharacter::CrouchPressed()
{
    if (bIsCrouched)
        UnCrouch();
    else
        Crouch();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::AimPressed()
{
    if (Combat && Combat->EquippedWeapon)
        Combat->SetAiming(true);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::AimReleased()
{
    if (Combat)
        Combat->SetAiming(false);
}

void AOpenShooterCharacter::CalculateAimOffsetPitch()
{
    AimOffset_Pitch = GetBaseAimRotation().Pitch;
    // If the character is not locally controlled, the pitch and yaw values are packaged together by the CharacterMovememntComponent
    // Therefore the pitch ends up being not in [-90, 90] range. We need to adjust it only if the character is not locally
    // controlled
    if (!IsLocallyControlled() && AimOffset_Pitch > 90.f)
    {
        // we re-map the pitch from [270, 360) to [-90, 0)
        const FVector2D InRange(270.f, 360.f);
        const FVector2D OutRange(-90.f, 0.f);
        AimOffset_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimOffset_Pitch);
    }
}

float AOpenShooterCharacter::CalculateSpeed() const
{
    FVector Velocity = GetVelocity();
    Velocity.Z = 0;    // We only want the horizontal velocity
    return Velocity.Size();
}

void AOpenShooterCharacter::AimOffset(float DeltaSeconds)
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
        return;
    const float Speed = CalculateSpeed();
    const bool bIsInAir = GetCharacterMovement()->IsFalling();

    if (Speed == 0 || !bIsInAir)    // standing still, not jumping
    {
        bRotateRootBone = true;
        const FRotator CurrentAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
        const FRotator DeltaRotation =
            UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);    // the order were is important
        // if we change the order, the sign of the finale yaw will be opposite
        AimOffset_Yaw = DeltaRotation.Yaw;
        bUseControllerRotationYaw = true;    // we want the controller to rotate the camera when standing still
        TurnInPlace(DeltaSeconds);           // This allows to use the updated Yaw

        // If we are not turning in place, we want to just use AimOffset_Yaw, otherwise we want to interpolate it to 0 (done in
        // TurnInPlace)
        if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
            InterpolatedAimOffsetYaw = AimOffset_Yaw;
    }
    // If we are moving or in the air,
    if (Speed > 0.f || bIsInAir)
    {
        bRotateRootBone = false;
        StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
        AimOffset_Yaw = 0.f;                 // when moving or in the air, we don't want the aim offset to be applied
        bUseControllerRotationYaw = true;    // we want the controller to rotate the camera
    }

    CalculateAimOffsetPitch();
}

void AOpenShooterCharacter::SimProxiesTurn()
{
    if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
        return;

    bRotateRootBone = false;

    if (const float Speed = CalculateSpeed(); Speed > 0.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        return;
    }

    // At this point, the simulated proxies cannot turn in place, so we need to simulate it
    ProxyRotationLastFrame = ProxyRotation;
    ProxyRotation = GetActorRotation();
    ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

    if (FMath::Abs(ProxyYaw) > TurnThreshold)
    {
        if (ProxyYaw > TurnThreshold)
        {
            TurningInPlace = ETurningInPlace::ETIP_Right;
        }
        else if (ProxyYaw < -TurnThreshold)
        {
            TurningInPlace = ETurningInPlace::ETIP_Left;
        }
        else
        {    // in between
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        }
        return;
    }
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;    // not enough turn, so do not turn in place
}

void AOpenShooterCharacter::TurnInPlace(float DeltaSeconds)
{
    if (AimOffset_Yaw > 90.f)
    {
        // Turn right
        TurningInPlace = ETurningInPlace::ETIP_Right;
    }
    else if (AimOffset_Yaw < -90.f)
    {
        // Turn left
        TurningInPlace = ETurningInPlace::ETIP_Left;
    }
    else
    {
        // Not turning
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    }
    if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
    {
        // Interpolate the AimOffset_Yaw to 0
        InterpolatedAimOffsetYaw = FMath::FInterpTo(InterpolatedAimOffsetYaw, 0.f, DeltaSeconds, 4.f);
        AimOffset_Yaw = InterpolatedAimOffsetYaw;
        // Then we check if we have turned enough and in that case we reset the TurningInPlace and the StartingAimRotation
        if (FMath::Abs(AimOffset_Yaw) < 10.f)
        {
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;
            StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
        }
    }
}

void AOpenShooterCharacter::ServerEquipPressed_Implementation()
{
    if (Combat)
        Combat->EquipWeapon(OverlappingWeapon);
}

AWeapon* AOpenShooterCharacter::GetEquippedWeapon() const
{
    if (Combat == nullptr)
        return nullptr;
    return Combat->EquippedWeapon;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::FirePressed()
{
    if (Combat && Combat->EquippedWeapon)    // necessary to check if weapon is equipped otherwise we end up in a broken state
        Combat->FireButtonPressed(true);
}
// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::FireReleased()
{
    if (Combat)    // even if weapon is not equipped we need to set the button as not pressed
        Combat->FireButtonPressed(false);
}

void AOpenShooterCharacter::HideCameraIfCharacterClose() const
{
    if (!IsLocallyControlled())
        return;
    if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraHideDistanceThreshold)
    {
        GetMesh()->SetVisibility(false);
        if (Combat && Combat->EquippedWeapon)
        {
            Combat->EquippedWeapon->GetMesh()->bOwnerNoSee = true;
        }
    }
    else
    {
        GetMesh()->SetVisibility(true);
        if (Combat && Combat->EquippedWeapon)
        {
            Combat->EquippedWeapon->GetMesh()->bOwnerNoSee = false;
        }
    }
}

void AOpenShooterCharacter::UpdateHUDHealth()
{
    PlayerController = PlayerController ? PlayerController : Cast<AOpenShooterPlayerController>(Controller);
    if (PlayerController)
        PlayerController->SetHUDHealth(Health, MaxHealth);
}

void AOpenShooterCharacter::MulticastPlayImpactEffects_Implementation(FVector_NetQuantize ImpactPoint)
{
    if (HitSound)
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
    if (HitParticles)
        UGameplayStatics::SpawnEmitterAtLocation(this, HitParticles, ImpactPoint);
}

void AOpenShooterCharacter::OnRep_Health()
{
    // This will be called on the clients when the Health variable is updated on the server

    // We play the hit react montage
    PlayHitReactMontage();
    // We update the health on the HUD
    UpdateHUDHealth();
}

void AOpenShooterCharacter::ReceiveDamage(
    AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
    Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
    // This will call the OnRep_Health function on the clients but we need to do the same things in the server

    // We play the hit react montage
    PlayHitReactMontage();
    // We update the health on the HUD
    UpdateHUDHealth();

    // We need to eliminate the player if the health is 0
    if (Health <= 0.1)
    {
        if (AOpenShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AOpenShooterGameMode>())
        {
            PlayerController = PlayerController == nullptr ? Cast<AOpenShooterPlayerController>(Controller) : PlayerController;
            AOpenShooterPlayerController* AttackerController = Cast<AOpenShooterPlayerController>(InstigatorController);
            GameMode->PlayerEliminated(this, PlayerController, AttackerController);    // ptr checks are done inside this function
        }
    }
}

void AOpenShooterCharacter::Eliminate()
{
    if (Combat && Combat->EquippedWeapon)
        Combat->EquippedWeapon->Drop();
    MulticastEliminate();
    GetWorldTimerManager().SetTimer(EliminationTimer, this, &AOpenShooterCharacter::EliminationFinished, EliminationDelay);
}

void AOpenShooterCharacter::MulticastEliminate_Implementation()
{
    bEliminated = true;    // Enables the elimination slot (from standing idle state machine)
    PlayEliminationMontage();

    // Start dissolve effect
    // We get the material of the mesh and set the dissolve parameter to the initial value (0).
    if (GetMesh()->GetMaterial(0))
    {
        DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
        GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.f);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
        StartDissolve();
    }

    // Disable character movement and collision
    GetCharacterMovement()->DisableMovement(); // no movement with wasd
    GetCharacterMovement()->StopMovementImmediately(); // no movement with mouse

    if (PlayerController)
        DisableInput(PlayerController); // no input at all

    // Disable collision
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Spawn Elimination Bot
    if (EliminationBotEffects)
    {
        FVector EliminationBotSpawnLocation(GetActorLocation() + FVector(0.f, 0.f, 200.f));
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EliminationBotEffects, EliminationBotSpawnLocation);
    }
    if (EliminationSound)
    {
        UGameplayStatics::SpawnSoundAtLocation(this, EliminationSound, GetActorLocation());
    }
}

void AOpenShooterCharacter::PlayEliminationMontage() const
{
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); AnimInstance && EliminationMontage)
        AnimInstance->Montage_Play(EliminationMontage, 1.0f);
}

void AOpenShooterCharacter::EliminationFinished()
{
    if (AOpenShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AOpenShooterGameMode>())
        GameMode->RequestRespawn(this, PlayerController);
    if (Combat && Combat->EquippedWeapon)
        Combat->EquippedWeapon->Drop();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::UpdateDissolveMaterial(const float DissolveValue)
{
    if (DynamicDissolveMaterialInstance)
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
}

void AOpenShooterCharacter::StartDissolve()
{
    DissolveTrack.BindDynamic(this, &AOpenShooterCharacter::UpdateDissolveMaterial);
    if (DissolveCurve && DissolveTimeline)
    {
        DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
        DissolveTimeline->Play();
    }
}
