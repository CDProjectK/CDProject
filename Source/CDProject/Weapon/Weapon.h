// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
//Custom Depth Color
#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "DataWrappers/ChaosVDParticleDataWrapper.h"

#include "Weapon.generated.h"



UENUM(BlueprintType)
enum class EWeaponState:uint8
{
	EWS_Initial UMETA(DisplayName = "Initial"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
};

UENUM(BlueprintType)
enum class EWeaponType:uint8
{
	EWT_None UMETA(DisplayName = "None"),
	EWT_Rifle UMETA(DisplayName = "Rifle"),
	EWT_Sniper UMETA(DisplayName = "Sniper"),
	EWT_RocketLauncher UMETA(DisplayName = "RocketLauncher"),
	EWT_Pistol UMETA(DisplayName="Pistol"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_Speical UMETA(DisplayName = "Special"),
	EWT_Knife UMETA(DisplayName="Knife"),
	EWT_Hand UMETA(DisplayName="Hand"),
	EWT_C4 UMETA(DisplayName="C4")
};

UCLASS()
class CDPROJECT_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void Fire(const FVector& HitTarget);
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	//Using This function -> ProjectileWeapon
	
	//* Widget Set function
	void SetWeaponAmmoHUD();
	void SetWeaponInfoHUD();
	void SetWeaponState(EWeaponState state);
	void AddAmmo(int32 AmmoToAdd);
	void ResetAmmo();
	virtual void Reload();
	void Dropped(FVector& impactDir);
	void AttachToPlayer();

	//WeaponState
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* WeaponImage;

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_WeaponVisible)
	bool WeaponVisible = false;

	//Crosshair
	UPROPERTY(EditAnywhere, Category=Crosshair)
	class UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	UTexture2D* CrosshairBottom;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, Category="Camera")
	float ZoomedFOV=30.f;

	UPROPERTY(EditAnywhere, Category="Camera")
	float ZoomInterpSpeed=20.f;

	UPROPERTY(EditAnywhere, Category="Weapon")
	bool bAutomatic=true;
	
	UPROPERTY(EditAnywhere, Category="Weapon")
	float FireDelay=10.f;
	
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	UPROPERTY(EditAnywhere)
	USoundCue* ReloadSound;

	void EnableCustomDepth(bool bEnable);
	
	bool AmmoIsEmpty();
	void SpendCarriedAmmo(int32 ReloadAmount);
	
	//FORCEINLINE
	FORCEINLINE USphereComponent* GetAreaComponent() const {return AreaSphere;}
	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;}
	FORCEINLINE float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const {return WeaponMesh;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh3p() const {return WeaponMesh3p;}
	
	FORCEINLINE int32 GetAmmo() const {return Ammo;}
	FORCEINLINE int32 GetCarriedAmmo() const {return CarriedAmmo;}
	FORCEINLINE int32 GetAmmoCapacity() const {return AmmoCapacity;}
	FORCEINLINE int32 SetAmmoCapacity(int32 NewCapacity) {return AmmoCapacity=NewCapacity;}
	FORCEINLINE EWeaponType GetWeaponType() const {return WeaponType;}
	FORCEINLINE EWeaponState GetWeaponState() const {return WeaponState;}
	void SetWeaponVisible(bool tf);

	FVector GetWeaponMuzzle(); 
	
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponState,VisibleAnywhere, Category="Weapon Property")
	EWeaponState WeaponState;

	UPROPERTY(EditAnywhere, Category="Weapon Property")
	EWeaponType WeaponType;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;
	
	UPROPERTY(VisibleAnywhere)
	class ACDCharacter* OwnerCharacter;

	UPROPERTY()
	class ACDPlayerController* OwnerController;
	
	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_Ammo();

	UFUNCTION()
	void OnRep_CarriedAmmo();
	
	UFUNCTION()
	void OnRep_WeaponState();
	
	UFUNCTION()
	void OnRep_WeaponVisible();
	
	void SpendAmmo();
	
protected:
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* WeaponMesh3p;
	
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* AreaSphere;

	UPROPERTY(EditAnywhere, Category="Weapon Property")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACartridge> CartridgeClass;
//Ammo
	UPROPERTY(EditAnywhere,ReplicatedUsing=OnRep_Ammo)
	int32 Ammo;

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	
	UPROPERTY(EditAnywhere)
	int32 AmmoCapacity;
	//Init Ammo
	int32 InitAmmoCount;
	int32 InitCarriedAmmoCount;
	
};


