// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatWidget.h"

#include "CDProject/Controller/CDPlayerController.h"
#include "CDProject/WidgetPlus/ChatRow.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"

void UChatWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ChatInputText->OnTextCommitted.AddDynamic(this, &UChatWidget::OnTextCommitted);
}

void UChatWidget::AddMessageToChat(const struct FChatMessage& Data)
{
	
	UChatRow* NewRow=CreateWidget<UChatRow>(this, MessageRowClass);
	if (!NewRow) UE_LOG(LogTemp, Error, TEXT("Failed to create new row!"));
	if (NewRow)
	{
		NewRow->SetMessageData(Data);
		ChatScrollBox->AddChild(NewRow);
		ChatScrollBox->ScrollToEnd();
		SetRenderOpacity(1.0f);
	}
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

