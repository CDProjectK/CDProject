// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Cartridge.h"
#include "CDProject/Character/CDCharacter.h"
#include "CDProject/Component/CombatComponent.h"
#include "CDProject/Controller/CDPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates=true;
	SetReplicateMovement(true);
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	
	WeaponMesh->SetOnlyOwnerSee(true);
	WeaponMesh->SetOwnerNoSee(false);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	WeaponMesh->SetVisibility(false);

	WeaponMesh->bCastDynamicShadow = false;
	WeaponMesh->CastShadow = false;

	EnableCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();//renew Mesh Custom Depth Color
	
	
	WeaponMesh3p = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3p"));
	//WeaponMesh3p->SetupAttachment(RootComponent); 
	
	WeaponMesh3p->SetOnlyOwnerSee(false);
	WeaponMesh3p->SetOwnerNoSee(true);
	WeaponMesh3p->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh3p->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh3p->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh3p->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	WeaponMesh3p->SetVisibility(false);
	
	AreaSphere=CreateDefaultSubobject<USphereComponent>("AreaSphere");
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	AreaSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

bool AWeapon::AmmoIsEmpty()
{
	return Ammo<=0;
}

void AWeapon::SpendCarriedAmmo(int32 ReloadAmount)
{
	CarriedAmmo = FMath::Max(CarriedAmmo - ReloadAmount, 0);
}

FVector AWeapon::GetWeaponMuzzle()
{
	FVector muzzleLocation = FVector::ZeroVector;

	if (WeaponMesh && WeaponMesh->DoesSocketExist("MuzzleFlash"))
	{
		muzzleLocation = WeaponMesh->GetSocketLocation("MuzzleFlash");
	}
	else
	{
		return muzzleLocation = GetActorLocation();
	}
	return muzzleLocation;
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}


void AWeapon::SetWeaponVisible(bool tf)
{
	WeaponVisible = tf;
	if (HasAuthority())
	{
		OnRep_WeaponVisible();
	}
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	//DrawDebugSphere(GetWorld(), GetActorLocation(), 15, 10, FColor::Red, false);
	if (AreaSphere && HasAuthority())
	{
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereBeginOverlap);
	}
	EnableCustomDepth(false);
	InitAmmoCount=Ammo;
	InitCarriedAmmoCount=CarriedAmmo;

	OnRep_WeaponVisible();
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
		return;
		
	ACDCharacter* CDCharacter=Cast<ACDCharacter>(OtherActor);
	if (WeaponState ==  EWeaponState::EWS_Dropped && CDCharacter)
	{
		CDCharacter->GetWeapon(this);
	}
}

void AWeapon::OnRep_Ammo()
{
	SetWeaponAmmoHUD();
}

void AWeapon::OnRep_CarriedAmmo()
{
	SetWeaponAmmoHUD();
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
		break;
	case EWeaponState::EWS_Equipped:
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		
		EnableCustomDepth(false);
		break;
	}
}

void AWeapon::OnRep_WeaponVisible()
{
	GetWeaponMesh()->SetVisibility(WeaponVisible);
	GetWeaponMesh3p()->SetVisibility(WeaponVisible);
	if (WeaponVisible)
		SetWeaponAmmoHUD();
}

void AWeapon::SpendAmmo()
{
	Ammo=FMath::Clamp(Ammo-1,0,AmmoCapacity);
	SetWeaponAmmoHUD();
}

void AWeapon::ResetAmmo()
{
	Ammo=InitAmmoCount;
	CarriedAmmo=InitCarriedAmmoCount;
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		if (GetWorld())
		{
			if (GetWorld()->GetFirstPlayerController()->GetViewTarget() == OwnerCharacter)
				WeaponMesh->PlayAnimation(FireAnimation, false);
			else
				WeaponMesh3p->PlayAnimation(FireAnimation, false);
		}
	}
	if (CartridgeClass)
	{
		if (OwnerCharacter->IsLocallyControlled())
		{
			const USkeletalMeshSocket* AmmoEjectSocket=WeaponMesh->GetSocketByName("AmmoEject");
			if (AmmoEjectSocket)
			{
				FTransform AmmoEjectTransform=AmmoEjectSocket->GetSocketTransform(WeaponMesh);
				UWorld* World = GetWorld();
				if (World)
				{
					ACartridge* Cartridge = World->SpawnActor<ACartridge>(
						CartridgeClass,
						AmmoEjectTransform.GetLocation(),
						AmmoEjectTransform.GetRotation().Rotator()
					);
				}
			}
		}
		else
		{
			const USkeletalMeshSocket* AmmoEjectSocket=WeaponMesh3p->GetSocketByName("AmmoEject");
			if (AmmoEjectSocket)
			{
				FTransform AmmoEjectTransform=AmmoEjectSocket->GetSocketTransform(WeaponMesh3p);
				UWorld* World = GetWorld();
				if (World)
				{
					ACartridge* Cartridge = World->SpawnActor<ACartridge>(
						CartridgeClass,
						AmmoEjectTransform.GetLocation(),
						AmmoEjectTransform.GetRotation().Rotator()
					);
					if (Cartridge)
					{
						UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Cartridge->GetRootComponent());
						if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
						{
							FVector Impulse = FVector(-150.f, 100.f, 150.f);
							PrimitiveComp->AddImpulse(Impulse, NAME_None, true);
						}
					}
				}
			}
		}
	}
	SpendAmmo();
}

void AWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponVisible);
	DOREPLIFETIME(AWeapon, Ammo);
	DOREPLIFETIME(AWeapon, CarriedAmmo);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner==nullptr)
	{
		GetWeaponMesh()->SetOnlyOwnerSee(false);

		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		DetachFromActor(DetachRules);
		WeaponMesh->DetachFromComponent(DetachRules);
		WeaponMesh3p->DetachFromComponent(DetachRules);
		OwnerCharacter=nullptr;
		OwnerController=nullptr;
	}
	else
	{
		AttachToPlayer();
		SetWeaponAmmoHUD();
	}
}

void AWeapon::Dropped(FVector& impactDir)
{
	if (HasAuthority() && OwnerController && OwnerController->HighPingDelegate.IsBound())
	{
		OwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}
	
	SetOwner(nullptr);

	//Server Set (On Rep)
	GetWeaponMesh()->SetOnlyOwnerSee(false);
	
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	DetachFromActor(DetachRules);
	WeaponMesh->DetachFromComponent(DetachRules);
	WeaponMesh3p->DetachFromComponent(DetachRules);
	OwnerCharacter=nullptr;
	OwnerController=nullptr;

	SetWeaponState(EWeaponState::EWS_Dropped);
	
	//Add Impulse
	if (WeaponMesh->IsSimulatingPhysics())
	{
		FVector Impulse = impactDir.GetSafeNormal() * 150.0f;
		WeaponMesh->AddImpulse(Impulse, NAME_None, true);
	}
}

void AWeapon::AttachToPlayer()
{
	OwnerCharacter = Cast<ACDCharacter>(Owner);
	if (!OwnerCharacter)
		return;
	OwnerController = Cast<ACDPlayerController>(OwnerCharacter->Controller);
	
	SetWeaponState(EWeaponState::EWS_Equipped);
	
	AttachToComponent(
		OwnerCharacter->GetArmMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		TEXT("WeaponSocket")
	);
	GetWeaponMesh()->SetOnlyOwnerSee(true);
	
	GetWeaponMesh3p()->AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		TEXT("WeaponSocket")
	);
	GetWeaponMesh3p()->SetOwnerNoSee(true);
	
	if (HasAuthority()&&bUseServerSideRewind&&OwnerController->HighPingDelegate.IsBound())
	{
		OwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
	}
}

void AWeapon::SetWeaponAmmoHUD()
{
	if (OwnerCharacter == nullptr)
	{
		OwnerCharacter = Cast<ACDCharacter>(GetOwner());
	}
	if (OwnerCharacter && OwnerCharacter->GetCombatComponent())
	{
		OwnerCharacter->GetCombatComponent()->OnWeaponAmmoChangedDelegate.Broadcast(Ammo, CarriedAmmo);
	}
}

void AWeapon::SetWeaponInfoHUD()
{
	if (OwnerCharacter == nullptr)
	{
		OwnerCharacter = Cast<ACDCharacter>(GetOwner());
	}
	if (OwnerCharacter && OwnerCharacter->GetCombatComponent())
	{
		OwnerCharacter->GetCombatComponent()->OnWeaponInfoChangedDelegate.Broadcast(this);
	}
}

void AWeapon::SetWeaponState(EWeaponState state)
{
	WeaponState=state;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
		break;
	case EWeaponState::EWS_Equipped:
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		
		
		EnableCustomDepth(false);
		break;
	}
	

}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo=FMath::Clamp(Ammo-AmmoToAdd,0,AmmoCapacity);
	SetWeaponAmmoHUD();
}
void AWeapon::Reload()
{
	 CarriedAmmo=GetCarriedAmmo();
	 int32 AmmoNeed=AmmoCapacity-Ammo;
	 int32 ReloadAmount=FMath::Min(AmmoNeed, CarriedAmmo);
	 Ammo+=ReloadAmount;
	 SpendCarriedAmmo(ReloadAmount);
	 SetWeaponAmmoHUD();
}
