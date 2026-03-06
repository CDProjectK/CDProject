// Fill out your copyright notice in the Description page of Project Settings.


#include "CDPlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "CDProject/Character/CDCharacter.h"
#include "CDProject/Character/CDCharacterAttributeSet.h"
#include "CDProject/Component/CDSpringArmComponent.h"
#include "CDProject/Component/CombatComponent.h"
#include "CDProject/GameMode/DeathMatchGameMode.h"
#include "CDProject/GameMode/RoundGameMode.h"
#include "CDProject/GameState/CDGameState.h"
#include "CDProject/HUD/CDHUD.h"
#include "CDProject/PlayerState/CDPlayerState.h"
#include "CDProject/Weapon/Weapon.h"
#include "CDProject/Widget/Announcement.h"
#include "CDProject/Widget/C4InteractProgressWidget.h"
#include "CDProject/Widget/CharacterOverlay.h"
#include "CDProject/Widget/KDOverlay.h"
#include "CDProject/Widget/SniperScope.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


ACDPlayerController::ACDPlayerController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ACDPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == ECurMatchState::EMS_None)
		return;

	if (IsLocalController())
	{
		SetHUDTime();
		CheckTimeSync(DeltaSeconds);
		UpdateTeamMarkers();
	}
}

void ACDPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ACDPlayerState* APS = GetPlayerState<ACDPlayerState>())
	{
		int32 TeamIdFromPS = APS->GetTeam()==ETeam::ET_RedTeam?1:2; 
		SetGenericTeamId(FGenericTeamId(TeamIdFromPS));

		if (ACDCharacter* CDCharacter = Cast<ACDCharacter>(InPawn))
		{
			CDCharacter->SetUserName(APS->GetUsername());
		}
	}
}

void ACDPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACDPlayerController, MatchState);
	DOREPLIFETIME(ACDPlayerController, MatchTime);
	DOREPLIFETIME(ACDPlayerController, WarmupTime);
	DOREPLIFETIME(ACDPlayerController, CooldownTime);
	DOREPLIFETIME(ACDPlayerController, CountStartTime);
	DOREPLIFETIME(ACDPlayerController, HUDGoldCount);
	DOREPLIFETIME(ACDPlayerController, HUDDeathCount);
	DOREPLIFETIME(ACDPlayerController, HUDKillCount);
	DOREPLIFETIME(ACDPlayerController, HUDCharID);
}

void ACDPlayerController::InitializeController()
{
	// CDHUD=Cast<ACDHUD>(GetHUD());
	// if (IsLocalController())
	// {
	// 	if (CDHUD)
	// 	{
	// 		CDHUD->AddCharacterOverlay();
	// 		ShowAnnounceText(true);
	// 	}
	// }
	ClientSetPlayerAlive_Implementation(true);
	ServerRPC_UpdateMatchState();
}

void ACDPlayerController::BindHUDWidget(class ACDCharacter* NewCharacter)
{
	//UnBind For Cur Character
	ACDCharacter* CurCharacter = Cast<ACDCharacter>(GetViewTarget());
	if (CurCharacter)
	{
		CurCharacter->OnHealthChangedDelegate.RemoveAll(this);
		CurCharacter->OnShieldChangedDelegate.RemoveAll(this);
		CurCharacter->GetCombatComponent()->C4InteractDelegate.RemoveAll(this);
		CurCharacter->GetCombatComponent()->OnWeaponAmmoChangedDelegate.RemoveAll(this);
		CurCharacter->GetCombatComponent()->OnWeaponInfoChangedDelegate.RemoveAll(this);
		CurCharacter->GetCombatComponent()->OnCrossHairInfoChangedDelegate.RemoveAll(this);
		CurCharacter->GetCombatComponent()->OnScopeUIChangedDelegate.RemoveAll(this);

		PS = Cast<ACDPlayerState>(CurCharacter->GetPlayerState<ACDPlayerState>());
		if (PS)
		{
			PS->OnGoldUpdated.RemoveAll(this);
		}
	}

	//Bind Cur Character
	if (NewCharacter)
	{
		NewCharacter->OnHealthChangedDelegate.AddDynamic(this, &ACDPlayerController::SetHUDHealth);
		NewCharacter->OnShieldChangedDelegate.AddDynamic(this, &ACDPlayerController::SetHUDShield);
		NewCharacter->GetCombatComponent()->C4InteractDelegate.AddDynamic(this, &ACDPlayerController::ShowC4InteractProgress);
		NewCharacter->GetCombatComponent()->OnWeaponAmmoChangedDelegate.AddDynamic(this, &ACDPlayerController::SetHUDWeaponAmmo);
		NewCharacter->GetCombatComponent()->OnWeaponInfoChangedDelegate.AddDynamic(this, &ACDPlayerController::SetHUDWeaponInfo);
		NewCharacter->GetCombatComponent()->OnCrossHairInfoChangedDelegate.AddDynamic(this, &ACDPlayerController::SetHUDCrossHair);
		NewCharacter->GetCombatComponent()->OnScopeUIChangedDelegate.AddDynamic(this, &ACDPlayerController::ShowSniperScope);
		
		NewCharacter->InvokeHUDDelegate();
		
		PS = Cast<ACDPlayerState>(NewCharacter->GetPlayerState<ACDPlayerState>());
		if (PS)
		{
			PS->OnGoldUpdated.AddDynamic(this, &ACDPlayerController::SetGold);
			PS->OnGoldUpdated.Broadcast(PS->GetGold());
		}
		SetMinimap(NewCharacter);
	}
}

void ACDPlayerController::ServerRPC_UpdateMatchState_Implementation()
{
	ARoundGameMode* GameMode=Cast<ARoundGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime=GameMode->WarmUpTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime=GameMode->CooldownTime;
		MatchState = GameMode->GetCurMatchState();
	}
}

void ACDPlayerController::ClientSetMatchTime_Implementation(float matchTime)
{
	MatchTime = matchTime;
}

void ACDPlayerController::ClientSetMatchState_Implementation(ECurMatchState state, float curTime)
{
	MatchState = state;
	OnMatchStateSet(MatchState);
}

void ACDPlayerController::CheckPing(float DeltaTime)
{
	if (HasAuthority()) return;
	HighPingRunningTime+=DeltaTime;
	if (HighPingRunningTime>CheckPingFrequency)
	{
		if (PlayerState==nullptr) PlayerState=GetPlayerState<ACDPlayerState>();
		if (PlayerState)
		{
			if (PlayerState->ExactPing>HighPingThreshold)
			{
				//HighPingWarning();
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime=0.f;
	}
	//StopHighPingWarning();
}

void ACDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		InitializeController();
	}
}

void ACDPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass)
{
	Super::ClientSetHUD_Implementation(NewHUDClass);

	CDHUD=Cast<ACDHUD>(GetHUD());
	if (CDHUD)
	{
		CDHUD->AddCharacterOverlay();
		ShowAnnounceText(true);
		if (MatchState != ECurMatchState::EMS_None)
		{
			CDHUD->Announcement->AnnouncementText->SetText(FText::FromString(TEXT("")));
		}
	}
}

float ACDPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();// 서버라면 시간 반환
	else return GetWorld()->GetTimeSeconds()+ClientServerDelta;//클라이언트라면 보정값 반환
}

void ACDPlayerController::ShowKDOverlay(bool isShowing)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		if (isShowing)
		{
			CDHUD->AddKDOverlay(true);
		}
		else
		{
			CDHUD->AddKDOverlay(false);
		}
	}
}

void ACDPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}//접속하면 자신의 시간 서버에 보고
}


void ACDPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt=GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}
//접속한 모든 클라들은 서버에 접속하면 서버에게 현재시간 보고, 서버는 서버시간, 해당 클라시간을 같이 클라에 보냄.

void ACDPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
                                                                 float TimeServerReceivedClientRequest)
{
	float RoundTripTime=GetWorld()->GetTimeSeconds()-TimeOfClientRequest;
	float CurrentServerTime=TimeServerReceivedClientRequest+(0.5f*RoundTripTime);
	ClientServerDelta=CurrentServerTime-GetWorld()->GetTimeSeconds();
}
//RTT*0.5해서 서버 예측시간을 클라의 현재시간에 반환. GetServerTime을 요청한 클라는 Delta를 통해 해당시간 동기화 가능해짐.

void ACDPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime+=DeltaTime;
	if (IsLocalController()&&TimeSyncRunningTime>TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime=0.f;
	}
}

void ACDPlayerController::HandleWaiting()
{
	if (!IsLocalController())
		return;
	if (IsValid(GetPawn()))
	{
		GetPawn()->DisableInput(this);
		SetShowMouseCursor(true);
	}
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		if (GetWorld() && !Cast<ADeathMatchGameMode>(GetWorld()->GetAuthGameMode()))
			ShowStoreWidget(true);

		if (CDHUD->Announcement&&CDHUD->Announcement->AnnouncementText&&CDHUD->Announcement->AnnouncementCountdown)
		{
			CDHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("");
			CDHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		}
	}
}

void ACDPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	if (!IsLocalController())
		return;
	if (IsValid(GetPawn()))
	{
		SetShowMouseCursor(false);
	}
	
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		ShowStoreWidget(false);

		if (CDHUD->Announcement)
		{
			CDHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACDPlayerController::HandleCooldown()
{
	if (!IsLocalController())
		return;
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		if (CDHUD->Announcement&&CDHUD->Announcement->AnnouncementText&&CDHUD->Announcement->AnnouncementCountdown)
		{
			CDHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("Waiting For Next Round");
			CDHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		}
	}
}

void ACDPlayerController::SetHUDHealth(float Health)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&CDHUD->CharacterOverlay)
	{
		const float HealthPercent = Health/100.f;
		CDHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText=FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		CDHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ACDPlayerController::SetHUDShield(float Shield)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&CDHUD->CharacterOverlay)
	{
		const float ShieldPercent = Shield/100.f;
		CDHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		// FString HealthText=FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		// CDHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}


void ACDPlayerController::SetHUDWeaponAmmo(int32 WeaponAmmo, int32 CarriedAmmo)
{
	if (CDHUD&&CDHUD->CharacterOverlay && CDHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		FString WeaponAmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
		CDHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));
		//CDHUD->AddCompass();//here!/ if you want to deactivate Compass UI, annotation this!
		
		FString CarriedText = FString::Printf(TEXT("%d"), CarriedAmmo);
        CDHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedText));
	}
}

void ACDPlayerController::SetHUDWeaponInfo(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;

	if (CDHUD&&CDHUD->CharacterOverlay && CDHUD->CharacterOverlay)
	{
		FString WeaponName = Weapon->WeaponName;
		if (!WeaponName.IsEmpty() && CDHUD->CharacterOverlay->WeaponName)
		{
			CDHUD->CharacterOverlay->WeaponName->SetText(FText::FromString(WeaponName));
		}

		UTexture2D* WeaponImage = Weapon->WeaponImage;
		if (CDHUD->CharacterOverlay->WeaponImage)
		{
			if (WeaponImage)
				CDHUD->CharacterOverlay->WeaponImage->SetBrushFromTexture(WeaponImage);
			else
				CDHUD->CharacterOverlay->WeaponImage->SetBrushFromTexture(nullptr);
		}
	}
}

void ACDPlayerController::SetHUDCrossHair(FHUDPackage HudPackage)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		CDHUD->SetHUDPackage(HudPackage);
	}
}

void ACDPlayerController::SetHUDMatchCount(float CountdownTime)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&CDHUD->CharacterOverlay&&CDHUD->CharacterOverlay->MatchCountdownText)
	{
		if (CountdownTime<0.f)
		{
			CDHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Min=FMath::FloorToInt(CountdownTime/60);
		int32 Sec = FMath::FloorToInt(FMath::Fmod(CountdownTime, 60.f));
		FString CountdownText = FString::Printf(TEXT("%d:%d"), Min,Sec);
		CDHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}
void ACDPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	
	if (MatchState == ECurMatchState::EMS_Waiting)
	{
		TimeLeft = CountStartTime + WarmupTime - GetServerTime();
	}
	else if (MatchState == ECurMatchState::EMS_InGame)
	{
		TimeLeft = CountStartTime + MatchTime - GetServerTime();
	}
	else if (MatchState == ECurMatchState::EMS_CoolDown)
	{
		TimeLeft = CountStartTime + CooldownTime - GetServerTime();
	}
	else if (MatchState == ECurMatchState::EMS_None || MatchState == ECurMatchState::EMS_GameEnd)
	{
		TimeLeft = 0.f;
	}
	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	
	if (CountdownInt!=SecondsLeft)
	{
		SetHUDMatchCount(TimeLeft);
	}
	CountdownInt=SecondsLeft;
}

void ACDPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && !CDHUD->Announcement)
	{
		CDHUD->AddAnnouncement();
	}
	
	if (CDHUD&&CDHUD->Announcement&&CDHUD->Announcement->AnnouncementCountdown)
	{
		int32 Sec = CountdownTime;
		FString CountdownText = FString::Printf(TEXT("%d"), Sec);
		//UE_LOG(LogTemp,Display,TEXT("%s"),*CountdownText);
		CDHUD->Announcement->AnnouncementCountdown->SetText(FText::FromString(CountdownText));
		if (CountdownTime<0.f)
		{
			CDHUD->Announcement->AnnouncementCountdown->SetText(FText());
		}
	}
}

void ACDPlayerController::SetMinimap(class ACDCharacter* NewCharacter)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&CDHUD->CharacterOverlay&&CDHUD->CharacterOverlay->MinimapBox)
	{
		if (CDHUD->CharacterOverlay->MiniMapImage)
		{
			UTextureRenderTarget2D* MiniMapRenderTarget=NewCharacter->GetMiniMapTarget();
			FSlateBrush MiniMapBrush;
			MiniMapBrush.SetResourceObject(MiniMapRenderTarget);
			MiniMapBrush.ImageSize = FVector2D(128, 128);
			
			CDHUD->CharacterOverlay->MiniMapImage->SetBrush(MiniMapBrush);
		}
	}
}

void ACDPlayerController::UpdateTeamMarkers()
{
    ACDCharacter* MyCharacter = Cast<ACDCharacter>(GetViewTarget());
    if (!MyCharacter || !MyCharacter->GetCaptureTarget2D() || !GetWorld()) return;

    FVector CaptureOrigin = MyCharacter->GetCaptureTarget2D()->GetComponentLocation();
    float OrthoWidth = MyCharacter->GetCaptureTarget2D()->OrthoWidth;
    float TextureSize = MyCharacter->GetMiniMapTarget() ? MyCharacter->GetMiniMapTarget()->SizeX : 256.f;

    TArray<APawn*> CurrentTeamMembers;
	
	if (!CDHUD)
		CDHUD = Cast<ACDHUD>(GetHUD());
	if (!CDHUD) return;
	UCharacterOverlay* CharacterOverlay = CDHUD->CharacterOverlay;
	if (!CharacterOverlay || !CharacterOverlay->MiniMapImage)
		return;
	
	for (APlayerState* playerState : GetWorld()->GetGameState()->PlayerArray)
	{
		if (!playerState) continue;
		ACDCharacter* OtherCharacter = Cast<ACDCharacter>(playerState->GetPawn());
		if (OtherCharacter && OtherCharacter != MyCharacter) 
        {
            if (OtherCharacter->GetTeam() == MyCharacter->GetTeam())
            {
            	if (OtherCharacter->GetAttributeSet()->GetHealth() == 0.f)
            		continue;
            	
                CurrentTeamMembers.Add(OtherCharacter);
            	UUserWidget** FoundMarkerWidget = PlayerMarkers.Find(OtherCharacter);
            	UUserWidget* MarkerWidget = FoundMarkerWidget ? *FoundMarkerWidget : nullptr;
            	if (!MarkerWidget)
                {
                    if (!PlayerMarkerWidgetClass) continue;
                    MarkerWidget = CreateWidget<UUserWidget>(this, PlayerMarkerWidgetClass);
                	
                	if (MarkerWidget)
                    {
                		CharacterOverlay->MinimapBox->AddChild(MarkerWidget);
                        PlayerMarkers.Add(OtherCharacter, MarkerWidget);
                        // MarkerWidget->MarkerIcon->SetBrushFromTexture(TeamIconTexture); // 마커 아이콘 설정
                    }
                }

                if (MarkerWidget)
                {
                	// UE_LOG(LogTemp, Warning, TEXT("Origin: %s  OrthoWidth: %f"), *CaptureOrigin.ToString(), OrthoWidth);

                    FVector2D MinimapUV = ConvertWorldLocationToMinimapUV(
                        OtherCharacter->GetActorLocation(),
                        CaptureOrigin,
                        OrthoWidth,
                        TextureSize
                    );
                	// UE_LOG(LogTemp, Warning, TEXT("UV: %s"), *MinimapUV.ToString());
                	
                    // UV 좌표를 위젯의 픽셀 좌표로 변환 (0~1 범위 -> 위젯 크기)
                	FVector2D ImgSize = CharacterOverlay->MiniMapImage->GetDesiredSize();
                	// UE_LOG(LogTemp, Warning, TEXT("TextureSize: %f  ImageSize: %s"), TextureSize, *ImgSize.ToString());

                    FVector2D WidgetPosition = MinimapUV * FVector2D(
                    	300, 
						300);
                	// UE_LOG(LogTemp, Warning, TEXT("OutputPosX: %f  OutputPosY: %f"), WidgetPosition.X, WidgetPosition.Y);

                    // 위젯의 위치 설정 (UMG Canvas Panel의 위치 조정)
                    // Pivot을 고려하여 마커가 중앙에 오도록 오프셋 조정
                    UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(MarkerWidget);
                    if (CanvasSlot)
                    {
	                    CanvasSlot->SetSize({10,10});
                        // 마커 위젯의 크기를 고려하여 중앙에 배치
                        FVector2D MarkerSize = MarkerWidget->GetDesiredSize(); // 실제 마커 위젯의 크기
                        CanvasSlot->SetPosition(WidgetPosition - MarkerSize / 2.0f);
                    	
                        //CanvasSlot->SetVisibility(ESlateVisibility::Visible); // 보이게 설정
                    }
                }
            }
        }
    }

    // 더 이상 존재하지 않는 팀원 마커 제거
    TArray<APawn*> PawnsToRemove;
    for (auto& Elem : PlayerMarkers)
    {
        if (!CurrentTeamMembers.Contains(Elem.Key))
        {
            if (Elem.Value)
            {
            	UE_LOG(LogTemp, Warning, TEXT("Removing marker for %s"), *Elem.Key->GetName());
            	//CharacterOverlay->MinimapBox->RemoveChild(Elem.Value);
            	Elem.Value->RemoveFromParent();
                //Elem.Value->RemoveFromParent();
            }
            PawnsToRemove.Add(Elem.Key);
        }
    }
    for (APawn* pawn : PawnsToRemove)
    {
        PlayerMarkers.Remove(pawn);
    }
}

FVector2D ACDPlayerController::ConvertWorldLocationToMinimapUV(
	const FVector& InWorldLocation,
	const FVector& CaptureOrigin,
	float OrthoWidth,
	float TextureSize)
{
	// 1. 캡처 원점 기준 상대 위치
	FVector RelativeLocation = InWorldLocation - CaptureOrigin;

	// 2. Z축 기준으로 90° 왼쪽(반시계) 회전
	RelativeLocation = RelativeLocation.RotateAngleAxis(90.0f, FVector::UpVector);

	// 3. OrthoWidth 절반
	float HalfOrthoWidth = OrthoWidth * 0.5f;

	// 4. 기존 정규화 (X→U, Y→V 매핑)
	float NormX = -(RelativeLocation.X / HalfOrthoWidth) * 0.5f + 0.5f;
	float NormY = -(RelativeLocation.Y / HalfOrthoWidth) * 0.5f + 0.5f;

	// 5. 0~1 클램프
	return {
		FMath::Clamp(NormX, 0.f, 1.f),
		FMath::Clamp(NormY, 0.f, 1.f)
	};
}

void ACDPlayerController::SetGold(int32 NewGold)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	PS=PS==nullptr?Cast<ACDPlayerState>(GetPlayerState<ACDPlayerState>()):PS;
	
	// if (CDHUD && CDHUD->CharacterOverlay && PS)
	// {
	// 	HUDGoldCount = PS->GetGold();
	// 	FText GoldText = FText::AsNumber(HUDGoldCount); 
	// 	CDHUD->CharacterOverlay->Gold->SetText(GoldText);
	// }
	if (CDHUD && CDHUD->CharacterOverlay && PS)
	{
		HUDGoldCount = NewGold;
		FText GoldText = FText::AsNumber(HUDGoldCount); 
		CDHUD->CharacterOverlay->Gold->SetText(GoldText);
	}
}

void ACDPlayerController::SetKDOverlayUI()
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&KDOverlay)
	{
		CDHUD->KDOverlay->SetupScoreboard();
	}
}

void ACDPlayerController::UpdateKDOverlayData()
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&KDOverlay)
	{
		CDHUD->KDOverlay->SetupScoreboard();
	}
}

void ACDPlayerController::CreateKillLog(const FString& Killer, const FString& Victim)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && CDHUD->CharacterOverlay)
	{
		CDHUD->CharacterOverlay->CreateKillLog(Killer, Victim);
	}
}

void ACDPlayerController::ShowHitOverlay()
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && CDHUD->CharacterOverlay && CDHUD->CharacterOverlay->Hit_Anim)
	{
		if (CDHUD->CharacterOverlay->IsPlayingAnimation())
		{
			CDHUD->CharacterOverlay->PlayAnimation(CDHUD->CharacterOverlay->Hit_Anim, 0.1f);
		}
		else
		{
			CDHUD->CharacterOverlay->PlayAnimation(CDHUD->CharacterOverlay->Hit_Anim);
		}
	}
}

void ACDPlayerController::Client_ShowStoreWidget_Implementation(bool IsActivate)
{
	ShowStoreWidget(IsActivate);
}


void ACDPlayerController::ShowStoreWidget(bool bActivate)
{
	if (!IsLocalController()) return;
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		CDHUD->AddStore(bActivate);
	}
	else
	{
		FTimerDelegate TimerDel;
		TimerDel.BindUFunction(this, FName("RetryShowStoreWidget"), bActivate);
		GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDel);
		//GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ACDPlayerController::ShowStoreWidget(bActivate));
	}
}
void ACDPlayerController::RetryShowStoreWidget(bool bActivate)
{
	ShowStoreWidget(bActivate);
}

void ACDPlayerController::HideRoundScore(bool IsHide)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD&&CDHUD->CharacterOverlay&&CDHUD->CharacterOverlay->StateBox&&IsHide)
	{
		CDHUD->CharacterOverlay->StateBox->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		CDHUD->CharacterOverlay->StateBox->SetVisibility(ESlateVisibility::Visible);
	}
}

void ACDPlayerController::SetHUDATeam(int32 RedScore)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && CDHUD->KDOverlay)
	{
		FString ScoreText=FString::Printf(TEXT("%d"), RedScore);
		//CDHUD->KDOverlay->SetupScoreboard();
		CDHUD->KDOverlay->ARound->SetText(FText::FromString(ScoreText));
	}
	
	// if (CDHUD&&CDHUD->CharacterOverlay&&CDHUD->CharacterOverlay->RedTeamScore&&CDHUD->KDOverlay)
	// {
	// 	FString ScoreText=FString::Printf(TEXT("%d"), RedScore);
	// 	CDHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	// 	CDHUD->KDOverlay->ARound->SetText(FText::FromString(ScoreText));
	// }
}

void ACDPlayerController::SetHUDBTeam(int32 BlueScore)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && CDHUD->KDOverlay)
	{
		FString ScoreText=FString::Printf(TEXT("%d"), BlueScore);
		//CDHUD->KDOverlay->SetupScoreboard();
		CDHUD->KDOverlay->BRound->SetText(FText::FromString(ScoreText));
	}
	
	// CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	// if (CDHUD&&CDHUD->CharacterOverlay&&CDHUD->CharacterOverlay->BlueTeamScore&&CDHUD->KDOverlay)
	// {
	// 	FString ScoreText=FString::Printf(TEXT("%d"), BlueScore);
	//CDHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	// 	CDHUD->KDOverlay->BRound->SetText(FText::FromString(ScoreText));
	// }
}

void ACDPlayerController::SetTeamUIColor()
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && CDHUD->KDOverlay)
	{
		CDHUD->KDOverlay->UpdateTeamColor();
	}
}

void ACDPlayerController::ShowAnnounceText(bool bShow)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD)
	{
		if (bShow)
		{
			CDHUD->AddAnnouncement();
		}
		else
		{
			if (CDHUD->Announcement)
				CDHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACDPlayerController::AcknowledgePossession(class APawn* P)
{
	Super::AcknowledgePossession(P);
	if (IsLocalController()) 
	{
		UEnhancedInputLocalPlayerSubsystem* subSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()); 
		if(ACDCharacter* CDCharacter = dynamic_cast<ACDCharacter*>(P); IsValid(CDCharacter))
		{
			if (subSystem && CDCharacter)
			{
				subSystem->AddMappingContext(CDCharacter->GetInputMapping(), 1);
				subSystem->AddMappingContext(DefaultInputMappingContext, 0);
			}

			if (CDCharacter->GetAbilitySystemComponent())
			{
				CDCharacter->GetAbilitySystemComponent()->InitAbilityActorInfo(P, P);
			}
			CDCharacter->GetSpringArmComponent()->bUsePawnControlRotation = true;
			OwnedCharacter = CDCharacter;

			BindHUDWidget(OwnedCharacter);
		}
	}
	
	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);
	SetShowMouseCursor(false);
}

void ACDPlayerController::OnMatchStateSet(ECurMatchState State, float time)
{
	MatchState=State;
	ServerRPC_UpdateMatchState();
	if (MatchState==ECurMatchState::EMS_Waiting)
	{
		CountStartTime = time;
		ClientSetPlayerAlive(true);
		ClientSetEnableInput(false);
	}
	else if (MatchState==ECurMatchState::EMS_InGame)
	{
		CountStartTime = time;
		ClientSetEnableInput(true);
	}
	else if (MatchState==ECurMatchState::EMS_CoolDown)
	{
		CountStartTime = time;
	}
	else if (MatchState==ECurMatchState::EMS_GameEnd)
	{
		//?
	}
	
	if (IsLocalController() && HasAuthority()) //for Listen Server
		OnRep_MatchState();
}

void ACDPlayerController::OnRep_MatchState()
{
	if (MatchState==ECurMatchState::EMS_Waiting)
	{
		HandleWaiting();
		//Show Store HUD
	}
	else if (MatchState==ECurMatchState::EMS_InGame)
	{
		//Hide Store HUD
		HandleMatchHasStarted();
	}
	else if (MatchState==ECurMatchState::EMS_CoolDown)
	{
		HandleCooldown();
	}
	else if (MatchState == ECurMatchState::EMS_GameEnd)
	{
		//Show Game End UI
		if (CDHUD->Announcement)
		{
			CDHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			CDHUD->Announcement->AnnouncementText->SetText(FText::FromString("Game Ended...\nWaiting For Travel..."));
		}
	}
}

void ACDPlayerController::OnRep_HUDGoldCount()
{
	int32 NewGold=0;
	SetGold(NewGold);
}

void ACDPlayerController::OnRep_HUDKillCount()
{
	
}

void ACDPlayerController::OnRep_HUDDeathCount()
{
	
}

void ACDPlayerController::LeaveGame()
{
	//temp
	UGameplayStatics::OpenLevel(this, FName("Menu"));
	//Super::LeaveGame();
}

void ACDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (enhancedInputComponent)
	{
		enhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &ACDPlayerController::LMouseDown);
		enhancedInputComponent->BindAction(_tabAction, ETriggerEvent::Started, this, &ACDPlayerController::TabStart);
		enhancedInputComponent->BindAction(_tabAction, ETriggerEvent::Completed, this, &ACDPlayerController::TabEnd);
	}
}

void ACDPlayerController::ClientSetEnableInput_Implementation(bool tf)
{
	if (IsValid(GetCharacter()))
	{
		if (tf)
			GetCharacter()->EnableInput(this);
		else
			GetCharacter()->DisableInput(this);
	}
}

void ACDPlayerController::ClientSetPlayerAlive_Implementation(bool isAlive)
{
	if (!isAlive)
	{
		UEnhancedInputLocalPlayerSubsystem* subSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()); 
		if (subSystem)
		{
			if(ACDCharacter* CDCharacter = Cast<ACDCharacter>(GetPawn()); IsValid(CDCharacter))
			{
				subSystem->RemoveMappingContext(CDCharacter->GetInputMapping());
			}
		}
		if(ACDGameState* GameState = Cast<ACDGameState>(GetWorld()->GetGameState()))
		{
			for (int i = 0; i < GameState->PlayerArray.Num(); i++)
			{
				if (ACDCharacter* character = Cast<ACDCharacter>(GameState->PlayerArray[i]->GetPawn()))
				{
					OwnedCharacter = OwnedCharacter==nullptr ? Cast<ACDCharacter>(GetCharacter()) : OwnedCharacter;
					if (OwnedCharacter && character->GetTeam() == OwnedCharacter->GetTeam())
					{
						TeamCharacters.Push(character);
						if (character == OwnedCharacter)
						{
							CurPlayerIndex = i;
						}
					}
				}
			}
		}
	}
	else
	{
		UEnhancedInputLocalPlayerSubsystem* subSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()); 
		if (subSystem)
		{
			if(ACDCharacter* CDCharacter = Cast<ACDCharacter>(GetPawn()); IsValid(CDCharacter))
			{
				subSystem->AddMappingContext(CDCharacter->GetInputMapping(), 1);
			}
		}
		TeamCharacters.Empty();
		CurPlayerIndex = 0;
		if (OwnedCharacter)
		{
			SetViewTarget(OwnedCharacter);
			BindHUDWidget(OwnedCharacter);
		}
	}
}

void ACDPlayerController::LMouseDown()
{
	if (TeamCharacters.Num() == 0) return;

	int32 StartIndex = CurPlayerIndex;
	do
	{
		CurPlayerIndex = (CurPlayerIndex + 1) % TeamCharacters.Num();

		if (TeamCharacters[CurPlayerIndex] && !TeamCharacters[CurPlayerIndex]->_isDead)
		{
			SetViewTarget(TeamCharacters[CurPlayerIndex]);
			BindHUDWidget(TeamCharacters[CurPlayerIndex]);
			//ADD HUD LOGIC
			return;
		}
	}
	while (CurPlayerIndex != StartIndex);
}

void ACDPlayerController::TabStart()
{
	ShowKDOverlay(true);
}

void ACDPlayerController::TabEnd()
{
	ShowKDOverlay(false);
}

void ACDPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ACDPlayerController::ShowSniperScope(bool bIsAiming, bool bIsForce)
{
	CDHUD=CDHUD==nullptr?Cast<ACDHUD>(GetHUD()):CDHUD;
	if (CDHUD && !CDHUD->SniperScope)
	{
		CDHUD->AddSniperScope();
	}
	if (CDHUD&&CDHUD->SniperScope&&CDHUD->SniperScope->ScopeZoomIn)
	{
		ACDCharacter* CDCharacter=Cast<ACDCharacter>(GetCharacter());
		CDHUD->SniperScope->SetVisibility(ESlateVisibility::Visible);
		if (bIsAiming)
		{
			CDHUD->SniperScope->SetVisibility(ESlateVisibility::Visible);
			if (!bIsForce)
				CDHUD->SniperScope->PlayAnimation(CDHUD->SniperScope->ScopeZoomIn, 0.0f);
			else
				CDHUD->SniperScope->PlayAnimation(CDHUD->SniperScope->ScopeZoomIn, 0.25f);
		}
		else
		{
			if (!bIsForce)
				CDHUD->SniperScope->PlayAnimation(CDHUD->SniperScope->ScopeZoomIn, 0.25f,1,EUMGSequencePlayMode::Reverse);
			else
				CDHUD->SniperScope->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACDPlayerController::ShowC4PlantingProgress(bool isPlanting, float time)
{
	CDHUD = CDHUD == nullptr ? Cast<ACDHUD>(GetHUD()) : CDHUD;
	if (CDHUD == nullptr) return;

	if (!CDHUD->C4InteractProgress)
	{
		CDHUD->AddC4Progress();
	}
	if (CDHUD && CDHUD->C4InteractProgress)
	{
		CDHUD->C4InteractProgress->Reset(true);
		if (isPlanting)
		{
			CDHUD->C4InteractProgress->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CDHUD->C4InteractProgress->SetVisibility(ESlateVisibility::Hidden);
		}
		CDHUD->C4InteractProgress->SetProgressTime(time);
	}
}
void ACDPlayerController::ShowC4DefusingProgress(bool isDefusing, float time)
{
	CDHUD = CDHUD == nullptr ? Cast<ACDHUD>(GetHUD()) : CDHUD;
	if (CDHUD == nullptr) return;

	if (!CDHUD->C4InteractProgress)
	{
		CDHUD->AddC4Progress();
	}
	if (CDHUD && CDHUD->C4InteractProgress)
	{
		CDHUD->C4InteractProgress->Reset(false);
		if (isDefusing)
		{
			CDHUD->C4InteractProgress->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CDHUD->C4InteractProgress->SetVisibility(ESlateVisibility::Hidden);
		}
		CDHUD->C4InteractProgress->SetProgressTime(time);
	}
}

void ACDPlayerController::ShowC4InteractProgress(float time)
{
	CDHUD = CDHUD == nullptr ? Cast<ACDHUD>(GetHUD()) : CDHUD;
	if (CDHUD == nullptr) return;

	if (!CDHUD->C4InteractProgress)
	{
		CDHUD->AddC4Progress();
	}
	if (CDHUD && CDHUD->C4InteractProgress)
	{
		ACDCharacter* CDCharacter = Cast<ACDCharacter>(GetCharacter());
		if (CDCharacter)
		{
			bool isRedTeam = CDCharacter->GetTeam() == ETeam::ET_RedTeam;
		    CDHUD->C4InteractProgress->Reset(isRedTeam);
			if (time > 0.f)
			{
				CDHUD->C4InteractProgress->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				CDHUD->C4InteractProgress->SetVisibility(ESlateVisibility::Hidden);
			}
			CDHUD->C4InteractProgress->SetProgressTime(time);
		}
	}
}
