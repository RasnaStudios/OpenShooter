// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Weapon.generated.h"

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

    void ShowPickupWidget(bool bShow) const;

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

    // Fire Animation
public:
    void Fire() const;

private:
    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    UAnimationAsset* FireAnimation;

    // STATE
public:
    FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
    void SetWeaponState(const EWeaponState State);

private:
    UFUNCTION()
    void OnRep_WeaponState(EWeaponState PreviousState);

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState, Category = "Weapon Properties")
    EWeaponState WeaponState;

public:
    FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
    FORCEINLINE UMeshComponent* GetMesh() const { return WeaponMesh; }
};
