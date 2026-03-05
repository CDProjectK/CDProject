// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbilitySystemInterface.h"
#include "CDServer/Player/Team.h"
#include "CDProject/Types/WeaponStruct.h"
#include "CDCharacter.generated.h"

#define  MAXSPEED 470.f

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, CurHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldChanged, float, CurShield);


UCLASS()
class CDPROJECT_API ACDCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	// Sets default values for this character's properties
	ACDCharacter();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Reset() override;
	virtual void BeginDestroy() override;
	
	void UpdateVisibilityForSpectator(bool isWatching);
	void SetTeam(ETeam team);
	void SetUserName(const FString& Name);
	void PlayFootStepSound();
	UFUNCTION(Server, Reliable)
	void ServerPlayFootStepSound();
	class UCDSpringArmComponent* GetSpringArmComponent();

	void DestroyAllWeapon();
	void Kill();
	void GiveC4();

private:
	//Properties
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true), Category = "Sound")
	class USoundCue* _footstepSound;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true), Category = "Sound")
	class USoundAttenuation* _soundAttenuaion;
	UPROPERTY(ReplicatedUsing=OnRep_Team, VisibleAnywhere)
	ETeam _team = ETeam::ET_NoTeam;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true), Category = "Material")
	class UMaterialInterface* RedMaterial;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true), Category = "Material")
	class UMaterialInterface* BlueMaterial;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Dead(class AController* instigatorController, bool bIsHeadShot);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Hit(class AController* instigatorController, bool bIsHeadShot);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Reset(bool isAlive);
	void HandleDamage(float FinalDamage, class AController* instigatorController, bool bIsHeadShot);
	void UpdateArmMeshLocation(float DeltaTime);

	UFUNCTION()
	void OnRep_Team();
	UFUNCTION()
	void OnRep_UserName();
public:
	bool _isDead = false;
	bool bCanMove = true;;
	//State로 리팩터링 필요..

private:
	//Component
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UCDSpringArmComponent> _springArm;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UCameraComponent> _camera;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> _armMesh;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UCombatComponent> _combat;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class ULagCompensationComponent> LagCompensation;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UFootIKComponent> _footIK;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UTextRenderComponent> _textRenderer;
		UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_UserName)
		FString UserName = TEXT("DEFAULT");
	
	//MinimapComponent
	UPROPERTY(VisibleAnywhere, Category = "Minimap")
	class USpringArmComponent* MiniMapSpringArm;
	UPROPERTY(VisibleAnywhere, Category = "Minimap")
	class USceneCaptureComponent2D* SceneCapture2D;

	UPROPERTY(VisibleAnywhere, Category = "Minimap")
	class UTextureRenderTarget2D* MiniMapRenderTarget;
	
	
	UPROPERTY(EditAnywhere, Category = "Components")
	float _eyeHeight = 50.f;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	FTransform _weaponDefaultArmTransform;
	UPROPERTY(EditAnywhere, Category = "Components")
	FTransform  _weaponAimArmTransform;
	UPROPERTY(EditAnywhere, Category = "Components")
	FTransform  _handWeaponArmTransform;
	UPROPERTY(EditAnywhere, Category = "Components")
	FTransform  _knifeArmTransform;
	UPROPERTY(EditAnywhere, Category = "Components")
	FTransform  _specialWeaponArmTransform;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float _defaultFOV = 90.0f;
	
public:
	FORCEINLINE USkeletalMeshComponent* GetArmMesh() { return _armMesh; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCombatComponent* GetCombatComponent() { return _combat; }
	FORCEINLINE bool IsFirstPersonMesh(USkeletalMeshComponent* mesh) { return mesh == _armMesh; };
	FORCEINLINE UCameraComponent* GetCamera() { return _camera; }
	FORCEINLINE class UInputMappingContext* GetInputMapping() { return _inputMappingContext; }
	FORCEINLINE USceneCaptureComponent2D* GetCaptureTarget2D() { return SceneCapture2D; }
	FORCEINLINE UTextureRenderTarget2D* GetMiniMapTarget() { return MiniMapRenderTarget; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE ETeam GetTeam() { return _team; }
	
private:
	//Input
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputMappingContext> _inputMappingContext;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _moveAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _lookAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _jumpAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _crouchAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _walkAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _fireAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _aimAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _reloadAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<TObjectPtr<class UInputAction>> _changeWeaponActions;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _dropWeaponAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> _interactAction;
	
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void Jump() override;
	// void Crouch(bool bClientSimulation = false) override;
	void Walk();
	void UnWalk();

	void RequestFireStart();
	void RequestFireEnd();
	void RequestAim();
	void RequestReload();
	void RequestChangeWeapon(int weaponIndex);
	void RequestDropWeapon();
	void RequestInteractStart();
	void RequestInteractEnd();

	UPROPERTY(EditAnywhere, Category = "Input")
	float _mouseSensitivity = 1.f;
public:
	void GetWeapon(class AWeapon* weapon, bool isForce = false);
	UFUNCTION(Server, Reliable)
	void TryPurchase(const FWeaponStruct& WeaponData);
	void GiveItemToPlayer(const FWeaponStruct& WeaponData);
	UFUNCTION(Server, Reliable)
	void ServerGiveSheild(const FWeaponStruct& WeaponData);
	UFUNCTION(Server, Reliable)
	void ServerGiveWeapon(const FWeaponStruct& WeaponData);
	
private:
	//Network Property
	UPROPERTY(VisibleAnywhere, Replicated, Category = "Network")
	FRotator _controlRotation;
	UFUNCTION(Server, Reliable)
	void ServerSetControlRotation(FRotator control);
	
public:
	FORCEINLINE FRotator GetControlRotation() { return _controlRotation; }

public:
	//HUD
	FOnHealthChanged OnHealthChangedDelegate;
	FOnShieldChanged OnShieldChangedDelegate;
	
	void InvokeHUDDelegate();

private:
	//GAS
    UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCDCharacterAttributeSet> AttributeSet;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(AllowPrivateAccess), Category = "Abilities")
	TSubclassOf<class UGameplayEffect> _defaultAttributeEffect;

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	class UCDCharacterAttributeSet* GetAttributeSet();
	void InitializeAttributes();
	
};