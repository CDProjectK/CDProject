// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ACDCharacter;

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Location;
	
	UPROPERTY()
	FRotator Rotation;
	
	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()
	
	UPROPERTY()
	float Time;
	
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;
	
	UPROPERTY()
	ACDCharacter* Character; 
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bHitConfirmed;
	
	UPROPERTY()
	bool bHeadShot;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CDPROJECT_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ACDCharacter;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	//Hitscan
	FServerSideRewindResult ServerSideRewind(
	ACDCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime);
	
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
	ACDCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	void SaveFramePackage();
	
	void CacheBoxPositions(ACDCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ACDCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ACDCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ACDCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	FFramePackage GetFrameToCheck(ACDCharacter* HitCharacter,float HitTime);
	
	
	
	//HitScan Weapon
	FServerSideRewindResult ConfirmHit(
	const FFramePackage& Package,
	ACDCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation);

private:
	UPROPERTY()
	ACDCharacter* Character;
	
	UPROPERTY()
	class ACDPlayerController* Controller;
	
	TDoubleLinkedList<FFramePackage> FrameHistory;
	
	UPROPERTY(EditAnywhere)
	float MaxRecordTime=4.f;

};
