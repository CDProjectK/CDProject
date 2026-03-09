// Fill out your copyright notice in the Description page of Project Settings.


#include "CDHUD.h"

#include "CDProject/Widget/KDOverlay.h"
#include "Blueprint/UserWidget.h"
#include "CDProject/Widget/Announcement.h"
#include "CDProject/Widget/C4InteractProgressWidget.h"
#include "CDProject/Widget/CharacterOverlay.h"
#include "CDProject/Widget/ChatWidget.h"
#include "CDProject/Widget/ModeSelect.h"
#include "CDProject/Widget/ShopOverlay.h"
#include "CDProject/Widget/SniperScope.h"
#include "CDProject/WidgetPlus/Compass.h"

void ACDHUD::DrawHUD()
{
	Super::DrawHUD();
	
	if (!Canvas)
	{
		UE_LOG(LogTemp, Error, TEXT("Canvas is NULL!!"));
		return;
	}
	const float SpreadFactor=5.f;
	float CrosshairSpread=HUDPackage.CrosshairSpread*SpreadFactor;
	if (HUDPackage.CrosshairCenter)
	{
		FVector2D Spread(0.f,0.f);
		DrawCrosshair(HUDPackage.CrosshairCenter, Spread, HUDPackage.CrosshairColor);
	}//Getvelociy isinair
	if (HUDPackage.CrosshairRight)
	{
		FVector2D Spread(CrosshairSpread,0.f);
		DrawCrosshair(HUDPackage.CrosshairRight, Spread, HUDPackage.CrosshairColor);
	}
	if (HUDPackage.CrosshairLeft)
	{
		FVector2D Spread(-CrosshairSpread,0.f);
		DrawCrosshair(HUDPackage.CrosshairLeft, Spread, HUDPackage.CrosshairColor);
	}
	if (HUDPackage.CrosshairTop)
	{
		FVector2D Spread(0.f,-CrosshairSpread);
		DrawCrosshair(HUDPackage.CrosshairTop, Spread, HUDPackage.CrosshairColor);
	}
	if (HUDPackage.CrosshairBottom)
	{
		FVector2D Spread(0.f,CrosshairSpread);
		DrawCrosshair(HUDPackage.CrosshairBottom, Spread, HUDPackage.CrosshairColor);
	}
}

void ACDHUD::AddSniperScope()
{
	if (APlayerController* PlayerController=GetOwningPlayerController())
	{
		if (SniperScopeClass)
		{
			SniperScope=CreateWidget<USniperScope>(PlayerController, SniperScopeClass);
			if (SniperScope)
			{
				SniperScope->AddToViewport();
			}
		}
	}
}

void ACDHUD::AddCharacterOverlay()
{
	if (IsValid(CharacterOverlay))
		return;
	
	if (APlayerController* PlayerController=GetOwningPlayerController())
	{
		if (CharacterOverlayClass)
		{
			CharacterOverlay=CreateWidget<UCharacterOverlay>(PlayerController,CharacterOverlayClass);
			if (CharacterOverlay)
			{
				CharacterOverlay->AddToViewport();
				ChatWidget=CharacterOverlay->ChatWidget;
				if (ChatWidget == nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("ChatWidget is Null!!"));
				}
			}
		}
	}
}

void ACDHUD::AddGameStateOverlay()
{
	if (APlayerController* PlayerController=GetOwningPlayerController())
	{
		GameStateOverlay=CreateWidget<UKDOverlay>(PlayerController,GameStateOverlayClass);
		GameStateOverlay->AddToViewport();
	}
}

void ACDHUD::AddAnnouncement()
{
	if (APlayerController* PlayerController=GetOwningPlayerController())
	{
		if (Announcement == nullptr)
		{
			Announcement=CreateWidget<UAnnouncement>(PlayerController,AnnouncementClass);
		}
		Announcement->AddToViewport();
	}
}

void ACDHUD::AddCompass()
{
	if (APlayerController* PlayerController=GetOwningPlayerController())
	{
		if (CompassWidgetClass)
		{
			Compass=CreateWidget<UCompass>(PlayerController,CompassWidgetClass);
			if (Compass)
			{
				Compass->AddToViewport();
			}
		}
	}
}

void ACDHUD::AddStore(bool IsActivate)
{
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		if (IsActivate)
		{
			if (!ShopOverlay)
			{
				ShopOverlay = CreateWidget<UShopOverlay>(PlayerController, StoreWidgetClass);
			}
			if (ShopOverlay && !ShopOverlay->IsInViewport())
			{
				ShopOverlay->AddToViewport();
			}
		}
		else
		{
			if (ShopOverlay && ShopOverlay->IsInViewport())
			{
				ShopOverlay->RemoveFromParent();
				ShopOverlay = nullptr;
			}
		}
	}
}

void ACDHUD::AddKDOverlay(bool IsActivate)
{
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		if (IsActivate)
		{
			if (!KDOverlay)
			{
				KDOverlay = CreateWidget<UKDOverlay>(PlayerController, KDOverlayClass);
			}
			if (KDOverlay && !KDOverlay->IsInViewport())
			{
				KDOverlay->AddToViewport();
				KDOverlay->SetupScoreboard();
			}
		}
		else
		{
			if (KDOverlay && KDOverlay->IsInViewport())
			{
				KDOverlay->RemoveFromParent();
			}
		}
	}
}

void ACDHUD::AddModeSelect()
{
	if (HasAuthority())
	{
		if (ModeSelectClass)
		{
			ModeSelect=CreateWidget<UModeSelect>(GetOwningPlayerController(), ModeSelectClass);
			if (ModeSelect) ModeSelect->AddToViewport();
		}
	}
}

void ACDHUD::AddC4Progress()
{
	if (C4InteractProgressClass)
	{
		C4InteractProgress = CreateWidget<UC4InteractProgressWidget>(GetWorld(), C4InteractProgressClass);
		if (C4InteractProgress)
		{
			C4InteractProgress->AddToViewport();
			C4InteractProgress->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACDHUD::AddChatMessage(const struct FChatMessage& Data)
{
	if (ChatWidget)
	{
		ChatWidget->AddMessageToChat(Data);
	}
}

void ACDHUD::BeginPlay()
{
	Super::BeginPlay();
	//AddCharacterOverlay();
}

void ACDHUD::DrawCrosshair(UTexture2D* Texture, FVector2D Spread, FLinearColor CrosshairColor)
{
	FVector2D ViewportSize;
	if (GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D ViewportCenter = ViewportSize / 2;

	const float ScreenW=60.f;
	const float ScreenH=60.f;

	const float ScreenX=ViewportCenter.X-(ScreenW/2.f)+Spread.X;
	const float ScreenY=ViewportCenter.Y-(ScreenH/2.f)+Spread.Y;

	DrawTexture(  
	Texture,
	ScreenX,  
	ScreenY,  
	ScreenW,  
	ScreenH, 
	0.f,  
	0.f,  
	1.f,  
	1.f,  
	CrosshairColor
	);
	// if (Texture)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("CrosshairColor: R=%.2f, G=%.2f, B=%.2f, A=%.2f"), 
	// 		CrosshairColor.R, CrosshairColor.G, CrosshairColor.B, CrosshairColor.A);
	// }

}

