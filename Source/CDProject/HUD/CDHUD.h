// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CDHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;
	float CrosshairSpread;
	FLinearColor CrosshairColor;
};



UCLASS()
class CDPROJECT_API ACDHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category="Player State")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category="Player State")
	TSubclassOf<class UUserWidget> GameStateOverlayClass;
	
	UPROPERTY()
	class UKDOverlay* GameStateOverlay;

	UPROPERTY(EditAnywhere, Category="Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass;
	
	UPROPERTY()
	class UAnnouncement* Announcement;
	
	UPROPERTY(EditAnywhere, Category="SniperScope")
	TSubclassOf<UUserWidget> SniperScopeClass;
	
	UPROPERTY()
	class USniperScope* SniperScope;

	UPROPERTY(EditAnywhere, Category="C4Progress")
	TSubclassOf<UUserWidget> C4InteractProgressClass;
	
	UPROPERTY()
	class UC4InteractProgressWidget* C4InteractProgress;

	UPROPERTY(EditAnywhere, Category="Compass")
	TSubclassOf<UUserWidget> CompassWidgetClass;
	
	UPROPERTY()
	class UCompass* Compass;

	UPROPERTY(EditAnywhere, Category="Store")
	TSubclassOf<UUserWidget> StoreWidgetClass;

	UPROPERTY()
	class UShopOverlay* ShopOverlay;

	UPROPERTY(EditAnywhere, Category="KDOVerlay")
	TSubclassOf<UUserWidget> KDOverlayClass;

	UPROPERTY()
	class UKDOverlay* KDOverlay;

	UPROPERTY(EditAnywhere, Category="ModeSelect")
	TSubclassOf<UUserWidget> ModeSelectClass;

	UPROPERTY(EditAnywhere, Category="Chat")
	TSubclassOf<UUserWidget> ChatWidgetClass;
	
	UPROPERTY()
	class UChatWidget* ChatWidget;
	
	UPROPERTY()
	class UModeSelect* ModeSelect;
	
	

	void AddSniperScope();
	void AddCharacterOverlay();
	void AddGameStateOverlay();//KDOverlay or GameStateOverlay
	void AddAnnouncement();
	void AddCompass();
	void AddStore(bool IsActivate);
	void AddKDOverlay(bool IsActivate);
	void AddModeSelect();
	void AddC4Progress();
	void AddChatMessage(const struct FChatMessage& Data);
protected:
	virtual void BeginPlay() override;
private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D Spread, FLinearColor CrosshairColor);
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) {HUDPackage=Package;}
};
