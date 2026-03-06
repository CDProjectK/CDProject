// Fill out your copyright notice in the Description page of Project Settings.


#include "CDCharacter.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemComponent.h"
#include "CDCharacterAttributeSet.h"
#include "CDCharacterMovementComponent.h"
#include "CDProject/CDProject.h"
#include "CDProject/AI/CDAIController.h"
#include "CDProject/Anim/CDAnimInstance.h"
#include "CDProject/Component//FootIKComponent.h"
#include "CDProject/Component/CDSpringArmComponent.h"
#include "CDProject/Component/CombatComponent.h"
#include "CDProject/Component/LagCompensationComponent.h"
#include "CDProject/Controller/CDPlayerController.h"
#include "CDProject/GameMode/DeathMatchGameMode.h"
#include "CDProject/GameMode/RoundGameMode.h"
#include "CDProject/PlayerState/CDPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CDProject/Weapon/Weapon.h"
#include "CDProject/Weapon/DamageType/DamageType_Explode.h"
#include "CDServer/Player/Team.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

// Sets default values
ACDCharacter::ACDCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	_springArm = CreateDefaultSubobject<UCDSpringArmComponent>(TEXT("Spring Arm"));
	_springArm->SetupAttachment(RootComponent);
	_springArm->bUsePawnControlRotation = false;
	
	_camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	_camera->SetupAttachment(_springArm);
	_camera->SetRelativeLocation(FVector(0, 0, _eyeHeight));
	
	_armMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arm Mesh"));
	_armMesh->SetupAttachment(_camera);
	_armMesh->bOnlyOwnerSee = true;
	_armMesh->bCastDynamicShadow = false;
	_armMesh->CastShadow = false;
	_armMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	GetMesh()->SetOwnerNoSee(true);
	
	_textRenderer = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Name Text"));
	_textRenderer->SetupAttachment(RootComponent);
	
	_combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	_combat->SetIsReplicated(true);
	
	_footIK = CreateDefaultSubobject<UFootIKComponent>(TEXT("FootIK"));
	
	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));
	
	AttributeSet = CreateDefaultSubobject<UCDCharacterAttributeSet>(TEXT("AttributeSet"));
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());

	//Minimap
	MiniMapSpringArm=CreateDefaultSubobject<USpringArmComponent>(TEXT("Minimap Spring Arm"));
	MiniMapSpringArm->SetupAttachment(RootComponent);
	MiniMapSpringArm->TargetArmLength = 1000.f; 
	MiniMapSpringArm->SetRelativeLocation(FVector(0.f, 0.f, 1000.f));
	MiniMapSpringArm->SetWorldRotation(FRotator(-90.f, 0, 0));
	MiniMapSpringArm->bUsePawnControlRotation = false;	
	MiniMapSpringArm->bInheritPitch = false;
	MiniMapSpringArm->bInheritRoll = false;
	MiniMapSpringArm->bInheritYaw = false;
	
	SceneCapture2D=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture2D"));
	SceneCapture2D->SetupAttachment(MiniMapSpringArm);
	SceneCapture2D->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture2D->OrthoWidth = 2048.f; 
	SceneCapture2D->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture2D->bCaptureEveryFrame = false;
	SceneCapture2D->bCaptureOnMovement = true;
	
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);
	
	body = CreateDefaultSubobject<UBoxComponent>(TEXT("body"));
	body->SetupAttachment(GetMesh(), FName("body"));
	HitCollisionBoxes.Add(FName("body"), head);
	
	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);
	
	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);
	
	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);
	
	for (auto Box: HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);//HitBox라는 특수 판정 채널로 등록
			Box.Value->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);//상호작용 무시
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);//앞선 설정에서 모든 채널에대해 상호작용무시해놨지만, HitBox는 Ignore->Block으로 바꿔라
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);//성능최적화를 위한  Block
		}
	}
}

// Called when the game starts or when spawned
void ACDCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (!MiniMapRenderTarget)
	{
		MiniMapRenderTarget = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), TEXT("MiniMapRenderTarget"));
		if (MiniMapRenderTarget)
		{
			MiniMapRenderTarget->RenderTargetFormat = RTF_RGBA8;
			MiniMapRenderTarget->InitAutoFormat(256, 256);
			MiniMapRenderTarget->ClearColor = FLinearColor::Transparent;
			SceneCapture2D->TextureTarget = MiniMapRenderTarget;//Frame Drop
		}
	}
	
	if (GetPlayerState() && Cast<ACDPlayerState>(GetPlayerState()))
	{
		SetTeam(Cast<ACDPlayerState>(GetPlayerState())->GetTeam());
	}
	
	if (HasAuthority() && GetMesh())
	{
		GetMesh()->SetVisibility(false, true);
		GetMesh()->bOwnerNoSee = true;
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		GetMesh()->bRenderCustomDepth = false;
		GetMesh()->bVisibleInRayTracing = false;
		UE_LOG(LogTemp, Log, TEXT("!Authority Char begin Play1%s"), *this->GetName());
	}

	OnRep_Team();
	OnRep_UserName();
}

// Called every frame
void ACDCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		if (Controller != nullptr)
			_controlRotation = Controller->GetControlRotation();
		ServerSetControlRotation(_controlRotation);
	}
	if (!this->IsLocallyControlled())
	{
		//for Spector Update?
		//Controller->SetControlRotation(_controlRotation); => 이게 안되서 직접 수정을 해야함..
		FRotator CurrentRotation = _springArm->GetFakeRotation();
		FRotator TargetRotation = _controlRotation;
		FQuat CurrentQuat = CurrentRotation.Quaternion();
		FQuat TargetQuat = TargetRotation.Quaternion();
		FQuat SmoothedQuat = FQuat::Slerp(CurrentQuat, TargetQuat, FMath::Clamp(DeltaTime * 10, 0.f, 1.f));
		FRotator SmoothedRotation = SmoothedQuat.Rotator();
		_springArm->SetFakeRotation(SmoothedRotation);
	}

	if (HasAuthority())
	{
		//if >= 90 degree character rotate
		FRotator ControlRot = _controlRotation;
		FRotator ActorRot = GetActorRotation();
		float AimYaw = FMath::UnwindDegrees(ControlRot.Yaw - ActorRot.Yaw);
	
		if (AimYaw <= -45.f || AimYaw >= 45.f)
		{
			FRotator TargetRotation = FRotator(0.f, ControlRot.Yaw, 0.f);
			FRotator SmoothRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 2.5f); // 회전 속도 조절
			SetActorRotation(SmoothRotation);
		}
	}

	if (IsValid(_textRenderer))
	{
		if (APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController(); IsValid(LocalPlayerController))
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			LocalPlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

			FVector TextLocation = _textRenderer->GetComponentLocation();
			FVector DirectionToCamera = CameraLocation - TextLocation;
			FRotator NewTextRotation = DirectionToCamera.Rotation();
			
			FRotator FlatRotation = FRotator(0.f, NewTextRotation.Yaw, 0.f);
			_textRenderer->SetWorldRotation(FlatRotation);
		}
	}

	//MiniMapSpringArm->SetWorldRotation({-90, 0, 0});
	//Update Arm Mesh Location
	UpdateArmMeshLocation(DeltaTime);
}

void ACDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (enhancedInputComponent)
	{
		//Auto Replicate
		enhancedInputComponent->BindAction(_moveAction, ETriggerEvent::Triggered, this, &ACDCharacter::Move);
		enhancedInputComponent->BindAction(_lookAction, ETriggerEvent::Triggered, this, &ACDCharacter::Look);
		enhancedInputComponent->BindAction(_jumpAction, ETriggerEvent::Triggered, this, &ACDCharacter::Jump);
		enhancedInputComponent->BindAction(_jumpAction, ETriggerEvent::Completed, this, &ACDCharacter::StopJumping);
		enhancedInputComponent->BindAction(_crouchAction, ETriggerEvent::Triggered, this, &ACDCharacter::Crouch, false);
		enhancedInputComponent->BindAction(_crouchAction, ETriggerEvent::Completed, this, &ACDCharacter::UnCrouch, false);
		enhancedInputComponent->BindAction(_walkAction, ETriggerEvent::Triggered, this, &ACDCharacter::Walk);
		enhancedInputComponent->BindAction(_walkAction, ETriggerEvent::Completed, this, &ACDCharacter::UnWalk);

		//Need Server Request
		enhancedInputComponent->BindAction(_fireAction, ETriggerEvent::Started, this, &ACDCharacter::RequestFireStart);
		enhancedInputComponent->BindAction(_fireAction, ETriggerEvent::Completed, this, &ACDCharacter::RequestFireEnd);
		enhancedInputComponent->BindAction(_aimAction, ETriggerEvent::Completed, this, &ACDCharacter::RequestAim);
		enhancedInputComponent->BindAction(_reloadAction, ETriggerEvent::Completed, this, &ACDCharacter::RequestReload);
		enhancedInputComponent->BindAction(_changeWeaponActions[0], ETriggerEvent::Started, this, &ACDCharacter::RequestChangeWeapon, 0);
		enhancedInputComponent->BindAction(_changeWeaponActions[1], ETriggerEvent::Started, this, &ACDCharacter::RequestChangeWeapon, 1);
		enhancedInputComponent->BindAction(_changeWeaponActions[2], ETriggerEvent::Started, this, &ACDCharacter::RequestChangeWeapon, 2);
		enhancedInputComponent->BindAction(_changeWeaponActions[3], ETriggerEvent::Started, this, &ACDCharacter::RequestChangeWeapon, 3);
		enhancedInputComponent->BindAction(_changeWeaponActions[4], ETriggerEvent::Started, this, &ACDCharacter::RequestChangeWeapon, 4);
		enhancedInputComponent->BindAction(_dropWeaponAction, ETriggerEvent::Completed, this, &ACDCharacter::RequestDropWeapon);
		enhancedInputComponent->BindAction(_interactAction, ETriggerEvent::Started, this, &ACDCharacter::RequestInteractStart);
		enhancedInputComponent->BindAction(_interactAction, ETriggerEvent::Completed, this, &ACDCharacter::RequestInteractEnd);
	}
}

float ACDCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	if (_isDead)
	{
		return Super::TakeDamage(0.f, DamageEvent, EventInstigator, DamageCauser);
	}
	
	if (DamageEvent.DamageTypeClass->IsChildOf(UDamageType_Explode::StaticClass()))
	{
		HandleDamage(DamageAmount, EventInstigator, false);
	}
	else
	{
		if (!EventInstigator)
			return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
		ACDPlayerState* causerPlayerState = EventInstigator->GetPlayerState<ACDPlayerState>();
		ACDPlayerState* playerState = GetPlayerState<ACDPlayerState>();
		if (!playerState || !causerPlayerState)
		{
			return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		}
		ETeam playerTeam = playerState->GetTeam();
		ETeam causerTeam = causerPlayerState->GetTeam();
		if (GetWorld() && !Cast<ADeathMatchGameMode>(GetWorld()->GetAuthGameMode()))
		{
			if (playerTeam == causerTeam)
			{
				return Super::TakeDamage(0.f, DamageEvent, EventInstigator, DamageCauser);
			}
		}

		//cur Health Check
		if (AttributeSet && AttributeSet->GetHealth() == 0)
		{
			return 0.f;
		}

		float finalDamage = DamageAmount;
		bool bIsHeadShot = false;
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			const FPointDamageEvent* pointEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
			if (IsValid(pointEvent->HitInfo.Component.Get()) && GetMesh())
			{
				USkeletalMeshComponent* MeshComp = GetMesh();
				FName Bone = pointEvent->HitInfo.BoneName;
			
				UE_LOG(LogTemp, Log, TEXT("Comp: %s, Bone: %s"), *pointEvent->HitInfo.Component->GetName(), *Bone.ToString());
		
				FName ParentBone = MeshComp->GetParentBone(Bone);
				while (ParentBone != NAME_None)
				{
					if (ParentBone.ToString().Contains("head") ||
						ParentBone.ToString().Contains("neck"))
					{
						finalDamage *= 2.f;
						bIsHeadShot = true;
						break;
					}
					if (ParentBone.ToString().Contains("upperarm"))
					{
						finalDamage *= 0.5f;
						break;
					}
					if (ParentBone.ToString().Contains("thigh"))
					{
						finalDamage *= 0.75f;
						break;
					}
			
					ParentBone = MeshComp->GetParentBone(ParentBone);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("TakeDamage: Missing component or mesh on server"));
			}
		}

		HandleDamage(finalDamage, EventInstigator, bIsHeadShot);
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ACDCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACDCharacter, _controlRotation);
	DOREPLIFETIME(ACDCharacter, _team);
	DOREPLIFETIME(ACDCharacter, UserName);
}

void ACDCharacter::PostInitializeComponents()
{//Controller등의 생성자 단계에서 완벽하게 로딩되지 않은것들 조립.
	Super::PostInitializeComponents();
	
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ACDPlayerController>(Controller);
		}
	}
}

void ACDCharacter::PossessedBy(AController* NewController)
{ //Server Part
	Super::PossessedBy(NewController);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		InitializeAttributes();
	}
}

void ACDCharacter::Reset()
{
	//ServerCall
	if (AttributeSet->GetHealth() > 0)
	{ //Alive
		_combat->Reset(false);
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		Multicast_Reset(true);
		//ServerPart
	}
	else
	{ //Dead
		_combat->Reset(true);
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		Multicast_Reset(false);
	}

	if (GetPlayerState() && Cast<ACDPlayerState>(GetPlayerState()))
	{
		SetTeam(Cast<ACDPlayerState>(GetPlayerState())->GetTeam());
	}
}

void ACDCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
		GetWorld()->GetTimerManager().ClearAllTimersForObject(GetCombatComponent());
	}
}

void ACDCharacter::UpdateVisibilityForSpectator(bool isWatching)
{
	if (isWatching)
	{
		_armMesh->SetOnlyOwnerSee(false);
		_armMesh->SetVisibility(true);
		
		GetMesh()->SetVisibility(false);
	}
	else
	{
		_armMesh->SetOnlyOwnerSee(true);
		_armMesh->SetVisibility(false);
		
		GetMesh()->SetVisibility(true);
	}
}

void ACDCharacter::SetTeam(ETeam team)
{
	_team = team;
	if (!GetMesh() || !GetArmMesh() || GetNetMode() == NM_DedicatedServer)
		return;
	
	if (!RedMaterial || !BlueMaterial)
		return;
	
	switch (_team)
	{
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		GetArmMesh()->SetMaterial(0, RedMaterial);
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		GetArmMesh()->SetMaterial(0, BlueMaterial);
		break;
	default:
		break;
	}
}

void ACDCharacter::SetUserName(const FString& Name)
{
	UserName = Name;
	if (HasAuthority())
	{
		OnRep_UserName();
	}
}

void ACDCharacter::PlayFootStepSound()
{
	if (_footstepSound && _soundAttenuaion)
		UGameplayStatics::PlaySoundAtLocation(
			this,
			_footstepSound,
			GetActorLocation(),
			1.f,
			1.f,
			0.f,
			_soundAttenuaion);
}

void ACDCharacter::ServerPlayFootStepSound_Implementation()
{
	PlayFootStepSound();	
}

void ACDCharacter::Multicast_Dead_Implementation(class AController* instigatorController, bool bIsHeadShot)
{
	UCDAnimInstance* bodyAnim = Cast<UCDAnimInstance>(GetMesh()->GetAnimInstance());
	UCDAnimInstance* armAnim = Cast<UCDAnimInstance>(GetArmMesh()->GetAnimInstance());

	if (IsValid(_textRenderer))
		_textRenderer->SetVisibility(false);
	
	if (IsLocallyControlled())
	{
		//UnVisible Arm Mesh
		if (GetArmMesh())
		{
			GetArmMesh()->SetVisibility(false);
		}
		ACDPlayerController* CDPlayerController = Cast<ACDPlayerController>(GetController());
		if (IsValid(CDPlayerController))
		{
			CDPlayerController->ShowHitOverlay();
		}
	}
	if (HasAuthority())
	{
		//Drop All Weapon & Reset Tag & Clear Timer
		_combat->DeadAction();
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//Update Mode
		if(GetWorld() && GetWorld()->GetAuthGameMode())
		{
			ARoundGameMode* GameMode = Cast<ARoundGameMode>(GetWorld()->GetAuthGameMode());
			if (GameMode)
			{
				GameMode->PlayerEliminated(GetController(), instigatorController);
				
				if (GameMode->GetCurMatchState() == ECurMatchState::EMS_InGame)
				{
					ACDPlayerState* CDPlayerState = instigatorController->GetPlayerState<ACDPlayerState>();
					if (IsValid(CDPlayerState))
					{
						CDPlayerState->AddShot();
						if (bIsHeadShot)
							CDPlayerState->AddHeadShot();
					}
				}
			}
		}
	}

	if (GetWorld())
	{
		if (ACDPlayerController* CDPlayerController = Cast<ACDPlayerController>(GetWorld()->GetFirstPlayerController()); IsValid(CDPlayerController))
		{
			if (ACDCharacter* KillerCharacter = Cast<ACDCharacter>(CDPlayerController->GetPawn()); IsValid(KillerCharacter))
			{
				CDPlayerController->CreateKillLog(KillerCharacter->UserName, this->UserName);
			}
		}
	}
	
	if (bodyAnim)
		bodyAnim->PlayDeadMontage();
	if (armAnim)
		armAnim->PlayDeadMontage();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	_isDead = true;
}

void ACDCharacter::Multicast_Hit_Implementation(class AController* instigatorController, bool bIsHeadShot)
{
	UCDAnimInstance* bodyAnim = Cast<UCDAnimInstance>(GetMesh()->GetAnimInstance());
	UCDAnimInstance* armAnim = Cast<UCDAnimInstance>(GetArmMesh()->GetAnimInstance());
	if (bodyAnim)
	{
		bodyAnim->PlayHitMontage();
	}
	if (armAnim)
	{
		armAnim->PlayHitMontage();
	}
	
	if (IsLocallyControlled())
	{
		ACDPlayerController* CDPlayerController = Cast<ACDPlayerController>(GetController());
		if (IsValid(CDPlayerController))
		{
			CDPlayerController->ShowHitOverlay();
		}
	}
	
	ARoundGameMode* GameMode = Cast<ARoundGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		if (ACDPlayerController* attackerPlayerController = Cast<ACDPlayerController>(instigatorController))
		{
			if (GameMode->GetCurMatchState() == ECurMatchState::EMS_InGame)
			{
				ACDPlayerState* CDPlayerState = attackerPlayerController->GetPlayerState<ACDPlayerState>();
				if (IsValid(CDPlayerState))
				{
					CDPlayerState->AddShot();
					if (bIsHeadShot)
						CDPlayerState->AddHeadShot();
				}
			}
		}
	}
}

void ACDCharacter::Multicast_Reset_Implementation(bool isAlive)
{
	if (IsValid(_textRenderer))
		_textRenderer->SetVisibility(true);
	if (IsValid(_armMesh))
		_armMesh->SetVisibility(true);
	
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	GetMesh()->GetAnimInstance()->Montage_Stop(0.f);
	
	_isDead = false;
}

void ACDCharacter::HandleDamage(float FinalDamage, AController* instigatorController, bool bIsHeadShot)
{
	if (AttributeSet == nullptr) return;

	float CurShield = AttributeSet->GetShield();
	float CurHealth = AttributeSet->GetHealth();

	if (CurShield > 0.f)
	{
		CurShield = FMath::Clamp(CurShield - FinalDamage, 0.f, 100.f);
		AttributeSet->SetShield(CurShield);
	}
	else
	{
		CurHealth = FMath::Clamp(CurHealth - FinalDamage, 0.f, 100.f);
		AttributeSet->SetHealth(CurHealth);
	}
	
	if (CurHealth == 0.f)
	{
		Multicast_Dead(instigatorController, bIsHeadShot);
	}
	else
	{
		Multicast_Hit(instigatorController, bIsHeadShot);
	}
}

void ACDCharacter::UpdateArmMeshLocation(float DeltaTime)
{
	if (!_combat || !_combat->GetCurWeapon())
		return;
	
	FTransform nextTransform = FTransform::Identity;
	switch (_combat->GetCurWeaponType())
	{
	case EWeaponType::EWT_Rifle:
	case EWeaponType::EWT_Shotgun:
	case EWeaponType::EWT_Sniper:
	case EWeaponType::EWT_Pistol:
		if (_combat->IsAiming())
			nextTransform = _weaponAimArmTransform;
		else
			nextTransform = _weaponDefaultArmTransform;
		break;
	case EWeaponType::EWT_Hand:
	case EWeaponType::EWT_C4:
		nextTransform = _handWeaponArmTransform;
		break;
	case EWeaponType::EWT_Knife:
		nextTransform = _knifeArmTransform;
		break;
	case EWeaponType::EWT_Speical:
		nextTransform = _specialWeaponArmTransform;
		break;
	}
	float InterpSpeed = 10.0f;
		
	FTransform _currentArmTransform =
		UKismetMathLibrary::TInterpTo(_armMesh->GetRelativeTransform(), nextTransform, DeltaTime, InterpSpeed);
	_armMesh->SetRelativeTransform(_currentArmTransform);

	float NewFOV;
	if (_combat->IsAiming())
		NewFOV = FMath::FInterpTo(_camera->FieldOfView, _combat->GetCurWeapon()->GetZoomedFOV(), DeltaTime, InterpSpeed);
	else
		NewFOV = FMath::FInterpTo(_camera->FieldOfView, _defaultFOV, DeltaTime, InterpSpeed);
	_camera->SetFieldOfView(NewFOV);
}

void ACDCharacter::OnRep_Team()
{
	SetTeam(_team);	
}

void ACDCharacter::OnRep_UserName()
{
	if (IsValid(_textRenderer) && !UserName.IsEmpty())
		_textRenderer->SetText(FText::FromString(UserName));
}

UCDSpringArmComponent* ACDCharacter::GetSpringArmComponent()
{
	return _springArm;
}

void ACDCharacter::DestroyAllWeapon()
{
	UCombatComponent* CombatComponent = GetCombatComponent();
    if (IsValid(CombatComponent))
    {
    	for (int i = 0; i < CombatComponent->GetWeapons().Num(); i++)
    	{
    		if(IsValid(CombatComponent->GetWeapons()[i]))
    		{
    			CombatComponent->GetWeapons()[i]->Destroy();
    			CombatComponent->GetWeapons()[i] = nullptr;
    		}
    	}
    }
}

void ACDCharacter::Kill()
{
	GetAttributeSet()->SetHealth(0.f);
	if (_combat)
		_combat->DeadAction();
}

void ACDCharacter::GiveC4()
{
	if (_combat)
		_combat->CreateC4Weapon();
}

void ACDCharacter::Move(const FInputActionValue& value)
{
	if (!Controller ||
		!_combat ||
		_combat->IsInCombatState(CombatTags::State_Combat_PlantingC4) ||
		_combat->IsInCombatState(CombatTags::State_Combat_DefusingC4)
	)
		return;
	
	FVector inputVal = value.Get<FVector>();
	
	const FRotator rotation = Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	const FVector forwardDir = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
	const FVector rightDir = FRotationMatrix(rotation).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(forwardDir, inputVal.Y);
	AddMovementInput(rightDir, inputVal.X);
}

void ACDCharacter::Look(const FInputActionValue& value)
{
	FVector2D LookAxisVector = value.Get<FVector2D>();
	
	if (Controller)
	{
		// FOV에 비례한 감도 조절
		float CurrentFOV = _camera->FieldOfView;
		float DefaultFOV = 90.f; // 기본 FOV 값 (줌 아웃 상태)
		
		float FOVScale = CurrentFOV / DefaultFOV;
		float FinalSensitivity = FOVScale * _mouseSensitivity;

		AddControllerYawInput(LookAxisVector.X * FinalSensitivity);
		AddControllerPitchInput(-LookAxisVector.Y * FinalSensitivity);
	}
}

void ACDCharacter::Jump()
{
	if (!_combat ||
		_combat->IsInCombatState(CombatTags::State_Combat_PlantingC4) ||
		_combat->IsInCombatState(CombatTags::State_Combat_DefusingC4))
		return;
	
	Super::Jump();
}

// void ACDCharacter::Crouch(bool bClientSimulation)
// {
// 	if (GetCharacterMovement()->IsFalling())
// 		return;
// 	
// 	Super::Crouch(bClientSimulation);
// }



void ACDCharacter::Walk()
{
	UCDCharacterMovementComponent* CDCMC = Cast<UCDCharacterMovementComponent>(GetCharacterMovement());
	if (CDCMC)
		CDCMC->bWantsToWalk = true;
}

void ACDCharacter::UnWalk()
{
	UCDCharacterMovementComponent* CDCMC = Cast<UCDCharacterMovementComponent>(GetCharacterMovement());
	if (CDCMC)
		CDCMC->bWantsToWalk = false;
}

void ACDCharacter::RequestFireStart()
{
	if (!_combat) return;
	
	_combat->RequestFireStart();
}

void ACDCharacter::RequestFireEnd()
{
	if (!_combat) return;
	
	_combat->RequestFireEnd();
}

void ACDCharacter::RequestAim()
{
	if (!_combat)
		return;
	
	bool nextAiming = !_combat->IsAiming();
	_combat->ServerAim(nextAiming);
}

void ACDCharacter::RequestReload()
{
	if (!_combat)
		return;
	_combat->ServerAim(false);
	_combat->ServerReload();
}

void ACDCharacter::RequestChangeWeapon(int weaponIndex)
{
	if (!_combat)
		return;
	_combat->ServerAim(false);
	_combat->RequestChange(weaponIndex);
}

void ACDCharacter::RequestDropWeapon()
{
	if (!_combat)
		return;
	_combat->ServerAim(false);
	_combat->ServerDropWeapon();
}

void ACDCharacter::RequestInteractStart()
{
	if (!_combat)
		return;
	_combat->RequestInteractStart();
}

void ACDCharacter::RequestInteractEnd()
{
	if (!_combat)
		return;
	_combat->RequestInteractEnd();
}

//Always Called By Server
void ACDCharacter::GetWeapon(AWeapon* weapon, bool isForce)
{
	if (!_combat)
		return;
	_combat->ServerAim(false);
	_combat->GetWeapon(weapon, isForce);
}

void ACDCharacter::TryPurchase_Implementation(const FWeaponStruct& WeaponData)
{
	ACDPlayerState* PS = Cast<ACDPlayerState>(GetPlayerState());
	if (!PS) return;
	if (PS && PS->GetGold() >= WeaponData.Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can Purchase"));
		GiveItemToPlayer(WeaponData);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Purchase"));
	}
}

void ACDCharacter::GiveItemToPlayer(const FWeaponStruct& WeaponData)
{
	if (!WeaponData.WeaponClass)
	{
		if (WeaponData.WeaponName == FName(TEXT("Shield")))
		{
			ServerGiveSheild(WeaponData);
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("No WeaponClass"));
		return;
	}
	ServerGiveWeapon(WeaponData);
}

void ACDCharacter::ServerGiveSheild_Implementation(const FWeaponStruct& WeaponData)
{
	if (AttributeSet)
	{
		AttributeSet->SetShield(AttributeSet->GetMaxShield());
		ACDPlayerController* ACPC = Cast<ACDPlayerController>(Controller);
		ACDPlayerState* PS = GetPlayerState<ACDPlayerState>();
		if (ACPC && PS)
		{
			PS->SpendGold(WeaponData.Cost);
		}
	}
}

void ACDCharacter::ServerGiveWeapon_Implementation(const FWeaponStruct& WeaponData)
{
	if (!WeaponData.WeaponClass) return;
	ACDPlayerState* PS = GetPlayerState<ACDPlayerState>();
	if (!PS) return;

	UWorld* World = GetWorld();
	if (!World) return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AWeapon* SpawnedWeapon = World->SpawnActor<AWeapon>(
		WeaponData.WeaponClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (SpawnedWeapon)
	{
		UE_LOG(LogTemp,Display,TEXT("Spawn Weapon!"))
		GetCombatComponent()->GetWeapon(SpawnedWeapon, true);
		PS->SpendGold(WeaponData.Cost);
		
		if (GetWorld() && GetWorld()->GetAuthGameMode())
		{
			if (ARoundGameMode* GameMode = Cast<ARoundGameMode>(GetWorld()->GetAuthGameMode()))
			{
				GameMode->AddDestroyableActor(SpawnedWeapon);
			}
		}
	}
}

void ACDCharacter::ServerSetControlRotation_Implementation(FRotator control)
{
	_controlRotation = control;
}

void ACDCharacter::InvokeHUDDelegate()
{
	OnHealthChangedDelegate.Broadcast(GetAttributeSet()->GetHealth());
	OnShieldChangedDelegate.Broadcast(GetAttributeSet()->GetShield());
	
	_combat->C4InteractDelegate.Broadcast(_combat->C4InteractTime);
	_combat->OnWeaponAmmoChangedDelegate.Broadcast(_combat->GetCurAmmo(), _combat->GetCarriedAmmo());
	_combat->OnWeaponInfoChangedDelegate.Broadcast(_combat->GetCurWeapon());
	_combat->OnCrossHairInfoChangedDelegate.Broadcast(_combat->HUDPackage);
	_combat->OnScopeUIChangedDelegate.Broadcast(_combat->IsAiming(), true);
}

UAbilitySystemComponent* ACDCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

class UCDCharacterAttributeSet* ACDCharacter::GetAttributeSet()
{
	return AttributeSet;
}

void ACDCharacter::InitializeAttributes()
{
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(_defaultAttributeEffect, 0, EffectContext);
	if(NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveHandle = 
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
	}
}