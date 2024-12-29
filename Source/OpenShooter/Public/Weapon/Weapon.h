// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"

#include "Weapon.generated.h"

class ACasing;
class UWidgetComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    EWS_Initial UMETA(DisplayName = "Initial"),
    EWS_Equipped UMETA(DisplayName = "Equipped"),
    EWS_Dropped UMETA(DisplayName = "Dropped"),

    EWS_MAX UMETA(DisplayName = "DefaultMax"),
};

UCLASS()
class OPENSHOOTER_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AWeapon();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // We override this to replicate to clients the ammo count when the weapon is picked up
    virtual void OnRep_Owner() override;

    void ShowPickupWidget(bool bShow) const;

    // Textures for the weapon crosshair
    UPROPERTY(EditAnywhere, Category = "Crosshair")
    UTexture2D* CrosshairsCenter;

    UPROPERTY(EditAnywhere, Category = "Crosshair")
    UTexture2D* CrosshairsLeft;

    UPROPERTY(EditAnywhere, Category = "Crosshair")
    UTexture2D* CrosshairsRight;

    UPROPERTY(EditAnywhere, Category = "Crosshair")
    UTexture2D* CrosshairsTop;

    UPROPERTY(EditAnywhere, Category = "Crosshair")
    UTexture2D* CrosshairsBottom;

    void Drop();

    void SetHUDAmmo();

    void AddAmmo(int32 Amount);

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    USoundCue* EquipSound;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UFUNCTION()
    virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION()
    virtual void OnSphereEndOverlap(
        UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    TObjectPtr<USphereComponent> AreaSphere;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    TObjectPtr<UWidgetComponent> PickupWidget;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    TSubclassOf<ACasing> CasingClass;    // the bullet shell blueprint

    UPROPERTY(EditAnywhere)
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo, Category = "Weapon Properties")
    int32 Ammo;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    int32 MagCapacity;

    // Client function that updates the HUD when the ammo changes
    UFUNCTION()
    void OnRep_Ammo();

    // Server Function that subtracts 1 to ammo and updates the HUD
    void SpendRound();

    // FireButtonPressed Animation
public:
    virtual void Fire(const FVector& HitTarget);

private:
    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    TObjectPtr<UAnimationAsset> FireAnimation;

    // STATE
public:
    FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
    void SetWeaponState(const EWeaponState State);

private:
    UFUNCTION()
    void OnRep_WeaponState(EWeaponState PreviousState);

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState, Category = "Weapon Properties")
    EWeaponState WeaponState;

    // Zoomed FOV while aiming
    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    float ZoomedFOV = 30.f;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    float ZoomedInterpSpeed = 20.f;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    bool bAutomatic = false;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    float FireDelay = 0.15f;

public:
    FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
    FORCEINLINE UMeshComponent* GetMesh() const { return WeaponMesh; }

    FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
    FORCEINLINE float GetZoomedInterpSpeed() const { return ZoomedInterpSpeed; }

    FORCEINLINE bool IsAutomatic() const { return bAutomatic; }
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
    FORCEINLINE float GetFireDelay() const { return FireDelay; }

    FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
    FORCEINLINE int32 GetAmmo() const { return Ammo; }
    FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

    // Just for caching
private:
    UPROPERTY()
    class AOpenShooterCharacter* OwnerCharacter;
    UPROPERTY()
    class AOpenShooterPlayerController* OwnerController;
};
