// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */

UCLASS()
class CDPROJECT_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	//Status
	UPROPERTY(meta=(BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta=(BindWidget))
	class UTextBlock* HealthText;
	
	UPROPERTY(meta=(BindWidget))
	class UProgressBar* ShieldBar;

	//Gold
	UPROPERTY(meta=(BindWidget))
	UTextBlock* Gold;

	//Team Round Score
	UPROPERTY(meta=(BindWidget))
	UTextBlock* RedTeamScore;
	
	UPROPERTY(meta=(BindWidget))
	UTextBlock* BlueTeamScore;
	//Game State Box
	UPROPERTY(meta=(BindWidget))
	class UHorizontalBox *StateBox;

	//Chat
	UPROPERTY(meta=(BindWidget))
	class UChatWidget* ChatWidget;

	//Minimap
	UPROPERTY(meta=(BindWidget))
	class UCanvasPanel *MinimapBox;
	
	UPROPERTY(meta=(BindWidget))
	class UImage* MiniMapImage;
	
	//Weapon
	UPROPERTY(meta=(BindWidget))
	UTextBlock* WeaponName;
	
	UPROPERTY(meta=(BindWidget))
	class UImage* WeaponImage;
	
	UPROPERTY(meta=(BindWidget))
	UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* CarriedAmmoAmount;


	//Timer
	UPROPERTY(meta=(BindWidget))
	UTextBlock* MatchCountdownText;

	UPROPERTY(meta=(BindWidgetAnim), Transient)
	class UWidgetAnimation* Hit_Anim;
	
	void CreateKillLog(const FString& Killer, const FString& Victim);

	UPROPERTY(meta=(BindWidget))
	class UScrollBox* ScrollBox_KillLog;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UKillLogLine> KillLogLineClass;
};
