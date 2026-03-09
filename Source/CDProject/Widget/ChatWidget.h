// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"



UCLASS()
class CDPROJECT_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void AddMessageToChat(const struct FChatMessage& Data);
	
	class UEditableText* GetChatInputText() const {return ChatInputText;}
	
	UFUNCTION()
	void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
	virtual void NativeConstruct() override;
protected:
	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ChatScrollBox;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* ChatInputText;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> MessageRowClass;
	
	
};
