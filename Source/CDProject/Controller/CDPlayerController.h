// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "CDProject/Types/CurMatchState.h"
#include "CDServer/Player/CDSessionPlayerController.h"
#include "CDPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

USTRUCT(BlueprintType)
struct FChatMessage{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	FString SenderName;
	
	UPROPERTY(BlueprintReadOnly)
	FString MessageContent;
	
	UPROPERTY(BlueprintReadOnly)
	FDateTime TimeStamp;
};

UCLASS()
class CDPROJECT_API ACDPlayerController : public ACDSessionPlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ACDPlayerController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializeController();
	//HUD

	void BindHUDWidget(class ACDCharacter* NewCharacter);
	
	UFUNCTION()
	void SetHUDHealth(float Health);
	UFUNCTION()
	void SetHUDShield(float Shield);
	UFUNCTION()
	void SetHUDWeaponAmmo(int32 WeaponAmmo, int32 CarriedAmmo);
	UFUNCTION()
	void SetHUDWeaponInfo(class AWeapon* weapon);
	UFUNCTION()
	void SetHUDCrossHair(FHUDPackage HudPackage);
	UFUNCTION()
	void SetGold(int32 NewGold);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSendMessage(const FString& Message);
	UFUNCTION(Client, Reliable)
	void ClientReceveMessage(const FString& Message);

	void SetHUDTime();
	void SetHUDMatchCount(float CountdownTime);
	void SetHUDAnnouncementCountdown(float Countdown);
	void SetMinimap(class ACDCharacter* Character);
	void SetKDOverlayUI();
	void UpdateKDOverlayData();
	void CreateKillLog(const FString& Killer, const FString& Victim);
	void ShowHitOverlay();

	UFUNCTION(Client,Reliable)
	void Client_ShowStoreWidget(bool IsActivate);

	//bShowOverlay
	void ShowStoreWidget(bool bShow);
	void RetryShowStoreWidget(bool bActivate);
	
	//Weapon
	UFUNCTION()
	void ShowSniperScope(bool bIsAiming, bool bIsForce);
	
	UFUNCTION()
	void ShowC4PlantingProgress(bool isPlanting, float duration = 0.f);
	UFUNCTION()
	void ShowC4DefusingProgress(bool isDefusing, float duration = 0.f);
	
	UFUNCTION()
	void ShowC4InteractProgress(float time = 0.f);

	//TeamMatch Controller
	void HideRoundScore(bool IsHide);
	void SetHUDATeam(int32 RedScore);
	void SetHUDBTeam(int32 BlueScore);
	void SetTeamUIColor();
	void ShowAnnounceText(bool bShow);
	
	//MatchState
	virtual void AcknowledgePossession(class APawn* P) override;
	virtual void ReceivedPlayer() override;
	virtual float GetServerTime();

	//KDO Overlay
	void ShowKDOverlay(bool isShowing);
	
	void OnMatchStateSet(ECurMatchState State, float time = 0);
	void HandleWaiting();
	void HandleMatchHasStarted(bool bTeamsMatch=false);
	void HandleCooldown();

	UFUNCTION(Server, Reliable)
	void ServerRPC_UpdateMatchState();
	
	UFUNCTION(Client, Reliable)
	void ClientSetMatchTime(float matchTime);

	UFUNCTION(Client, Reliable)
	void ClientSetMatchState(ECurMatchState state, float curTime);
	
	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);
	//Delegate
	FHighPingDelegate HighPingDelegate;

protected:
	virtual void BeginPlay() override;
	virtual void ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass) override;
	void UpdateTeamMarkers();
	FVector2D ConvertWorldLocationToMinimapUV(const FVector& Vector, const FVector& CaptureOrigin, float OrthoWidth, float TextureSize);

//Sync Time
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta=0.f;

	UPROPERTY(EditAnywhere, Category=Time)
	float TimeSyncFrequency=5.f;

	float TimeSyncRunningTime=0.f;
	void CheckTimeSync(float DeltaTime);
private:
	UPROPERTY()
	class ACDHUD* CDHUD;

	UPROPERTY()
	class ACDPlayerState* PS;
	
	UPROPERTY(EditAnywhere, Category="HUD")
	TSubclassOf<class UKDOverlay> KDOverlay;
	
	UPROPERTY(EditAnywhere, Category="HUD")
	TSubclassOf<class UCharacterOverlay> _CharacterOverlay;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_MatchState)
	ECurMatchState MatchState;

	UPROPERTY(VisibleAnywhere)
	TMap<class APawn*, class UUserWidget*> PlayerMarkers;
	
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess))
	TSubclassOf<UUserWidget> PlayerMarkerWidgetClass;
	
	UFUNCTION()
	void OnRep_MatchState();
	UFUNCTION()
	void OnRep_HUDGoldCount();
	UFUNCTION()
	void OnRep_HUDKillCount();
	UFUNCTION()
	void OnRep_HUDDeathCount();
	
	//MatchVariable
	UPROPERTY(Replicated)
	float MatchTime=120.f;
	UPROPERTY(Replicated)
	float WarmupTime=0.f;
	UPROPERTY(Replicated)
	float CooldownTime=0.f;
	UPROPERTY(Replicated)
	float CountStartTime = 0.f;
	int32 CountdownInt=0;
	
	//State Variable
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDCarriedAmmo;
	float HUDWeaponAmmo;
	class Aweapon* HUDWeaponInfo;
	
	//Match KDState
	UPROPERTY(ReplicatedUsing=OnRep_HUDGoldCount)
	float HUDGoldCount;
	UPROPERTY(ReplicatedUsing=OnRep_HUDKillCount)
	float HUDKillCount;
	UPROPERTY(ReplicatedUsing=OnRep_HUDDeathCount)
	float HUDDeathCount;
	UPROPERTY(Replicated)
	FName HUDCharID;

private:
	virtual void LeaveGame() /*override*/;
	

public:
	virtual void SetupInputComponent() override;
	UFUNCTION(Client, Reliable)
	void ClientSetPlayerAlive(bool isAlive);
	UFUNCTION(Client, Reliable)
	void ClientSetEnableInput(bool tf);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Input")
	class ACDCharacter* OwnedCharacter = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TArray<TObjectPtr<ACDCharacter>> TeamCharacters;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultInputMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> LeftClickAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _tabAction;
	int32 CurPlayerIndex = 0;
	
	void LMouseDown();
	void TabStart();
	void TabEnd();
	
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	//Ping Variable
	float HighPingRunningTime = 0.f;
	
	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;
	
	UPROPERTY(EditAnywhere, Category = "Ping")
	float CheckPingFrequency = 20.f;
	
	
};

