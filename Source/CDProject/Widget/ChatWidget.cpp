// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatWidget.h"

#include "CDProject/Controller/CDPlayerController.h"
#include "Components/EditableText.h"

void UChatWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ChatInputText->OnTextCommitted.AddDynamic(this, &UChatWidget::OnTextCommitted);
}

void UChatWidget::AddMesaage(const class FChatMessage& Data)
{
}

void UChatWidget::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod==ETextCommit::OnEnter&&!Text.IsEmpty())
	{
		if (ACDPlayerController* PC=Cast<ACDPlayerController>(GetOwningPlayer()))
		{
			PC->ServerSendMessage(Text.ToString());
		}
		ChatInputText->SetText(FText::GetEmpty());
	}
}

