// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatRow.generated.h"

/**
 * 
 */
UCLASS()
class CDPROJECT_API UChatRow : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetMessageData(const struct FChatMessage& Data);
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SenderNameText;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MessageContentText;
};
