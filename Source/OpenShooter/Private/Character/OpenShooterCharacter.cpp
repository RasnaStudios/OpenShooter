// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/OpenShooterCharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AOpenShooterCharacter

AOpenShooterCharacter::AOpenShooterCharacter()
{
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

    // Avoid blocking the camera with other characters capsule
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

    // Set character to not rotate in place initially
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;

    // Set net update frequency
    NetUpdateFrequency = 66.0f;
    MinNetUpdateFrequency = 33.0f;
}

void AOpenShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // We need to replicate the OverlappingWeapon so that the client can show the pickup widget, but only the owner can interact
    // with it so the widget is only shown on the client that owns the character
    DOREPLIFETIME_CONDITION(AOpenShooterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void AOpenShooterCharacter::BeginPlay()
{
    // Call the base class
    Super::BeginPlay();
}

void AOpenShooterCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    if (Combat)
    {
        Combat->Character = this;
    }
}

void AOpenShooterCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Setting the aim offset
    AimOffset(DeltaSeconds);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AOpenShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
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
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AOpenShooterCharacter::AimButtonPressed);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AOpenShooterCharacter::AimButtonReleased);
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
    {
        UnCrouch();
    }
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
    {
        UnCrouch();
    }
    else
    {
        Crouch();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::AimButtonPressed()
{
    if (Combat)
        Combat->SetAiming(true);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AOpenShooterCharacter::AimButtonReleased()
{
    if (Combat)
        Combat->SetAiming(false);
}

void AOpenShooterCharacter::AimOffset(float DeltaSeconds)
{
    if (Combat && Combat->EquippedWeapon == nullptr)
        return;
    FVector Velocity = GetVelocity();
    Velocity.Z = 0;    // We only want the horizontal velocity
    const float Speed = Velocity.Size();
    const bool bIsInAir = GetCharacterMovement()->IsFalling();

    if (Speed == 0 || !bIsInAir)    // standing still, not jumping
    {
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
        StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
        AimOffset_Yaw = 0.f;                 // when moving or in the air, we don't want the aim offset to be applied
        bUseControllerRotationYaw = true;    // we want the controller to rotate the camera
    }

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
