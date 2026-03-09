// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatRow.h"

#include "CDProject/Controller/CDPlayerController.h"
#include "Components/TextBlock.h"

void UChatRow::SetMessageData(const struct FChatMessage& Data)
{
	if (SenderNameText&& MessageContentText)
	{
		SenderNameText->SetText(FText::FromString(Data.SenderName+TEXT(" : ")));
		MessageContentText->SetText(FText::FromString(Data.MessageContent));	
	}
}
