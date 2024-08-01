// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"

#include "OpenShooterAnimInstance.generated.h"

class AOpenShooterCharacter;
/**
 *
 */
UCLASS()
class OPENSHOOTER_API UOpenShooterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<AOpenShooterCharacter> OpenShooterCharacter;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float Speed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    bool bIsInAir;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float bIsAccelerating;
};
