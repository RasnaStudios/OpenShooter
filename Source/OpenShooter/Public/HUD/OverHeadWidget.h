// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "OverHeadWidget.generated.h"

class UTextBlock;
/**
 * Text that will be displayed over the character's head
 */
UCLASS()
class OPENSHOOTER_API UOverHeadWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* DisplayText;

    void SetDisplayText(const FString& Text) const;

    UFUNCTION(BlueprintCallable)
    void ShowPlayerName(APawn* PlayerPawn) const;

protected:
    virtual void NativeDestruct() override;
};
