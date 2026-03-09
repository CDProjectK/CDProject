// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

#include "CDProject/CDProject.h"
#include "CDProject/Character/CDCharacter.h"
#include "Components/BoxComponent.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}



void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ACDCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			if (!IsValid(BoxPair.Value)) continue;
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
	
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::CacheBoxPositions(ACDCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter==nullptr) return;
	for (auto& HitBoxPair:HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value!=nullptr)//==if(UBoxComponent* Box=HitBoxPair.Value) -> Scope-based 방식
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location=HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation=HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent=HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key,BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ACDCharacter* HitCharacter, const FFramePackage& Package)
{
}

void ULagCompensationComponent::ResetHitBoxes(ACDCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter==nullptr) return;
	for (auto& HitBoxPair:HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value!=nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ACDCharacter* HitCharacter,
	ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter&&HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ACDCharacter* HitCharacter,
                                                                  const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult Confirm=ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	// if (Character&&HitCharacter&&&Confirm.bHitConfirmed)
	// {
	// 	//	const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();
	// 	UGameplayStatics::ApplyDamage(
	// 		HitCharacter,
	// 		Damage,
	// 		Character->Controller,
	// 		Character->GetEquippedWeapon(),
	// 		UDamageType::StaticClass()
	// 	);
	// }
}


FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, 
	const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance=YoungerFrame.Time-OlderFrame.Time;
	const float InterpFraction=FMath::Clamp((HitTime-OlderFrame.Time)/Distance,0.f,1.f);
	
	FFramePackage InterpFramePackage;
	InterpFramePackage.Time=HitTime;
	
	for (auto& YoungerPair:YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName=YoungerPair.Key;
		//FName은 문자열비교가 아니라 정수비교를 한다.
		const FBoxInformation& OlderBox=OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox=YoungerFrame.HitBoxInfo[BoxInfoName];
		
		FBoxInformation InterpBoxInfo;
		
		InterpBoxInfo.Location=FMath::VInterpTo(OlderBox.Location,YoungerBox.Location,1.f, InterpFraction);
		InterpBoxInfo.Rotation=FMath::RInterpTo(OlderBox.Rotation,YoungerBox.Rotation,1.f,InterpFraction);
		InterpBoxInfo.BoxExtent=YoungerBox.BoxExtent;
		
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName,InterpBoxInfo);
	}
	return InterpFramePackage;
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ACDCharacter* HitCharacter, float HitTime)
{
	bool bReturn=
		HitCharacter==nullptr||
		HitCharacter->GetLagCompensation()==nullptr||
			HitCharacter->GetLagCompensation()->FrameHistory.GetHead()==nullptr||
				HitCharacter->GetLagCompensation()->FrameHistory.GetTail()==nullptr;
	if (bReturn) return FFramePackage();//현재의 Frame 전후가 존재하지 않는다면 바로 반환.
	
	FFramePackage FrameToCheck;
	
	bool bShouldInterpolate=true;
	const TDoubleLinkedList<FFramePackage>& History=HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime=History.GetTail()->GetValue().Time;//History -> Pair(Name, Value)=(FName,FFrame)
	const float NewestHistoryTime=History.GetHead()->GetValue().Time;
	
	if (OldestHistoryTime>HitTime)
	{
		return FFramePackage();
	}
	if (OldestHistoryTime==HitTime)
	{
		FrameToCheck=History.GetTail()->GetValue();
		bShouldInterpolate=false;
	}
	if (NewestHistoryTime<=HitTime)
	{
		FrameToCheck=History.GetHead()->GetValue();
		bShouldInterpolate=false;
	}
	
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger=History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older=Younger;
	while (Older->GetValue().Time>HitTime)//Older이 시간이 더 적기에, 최근까지 이동해야 함
	{
		if (Older->GetNextNode()==nullptr) break;
		Older=Older->GetNextNode();
		if (Older->GetValue().Time>HitTime)
		{
			Younger=Older;
		}
	}
	
	if (Older->GetValue().Time==HitTime)
	{
		FrameToCheck=Older->GetValue();
		bShouldInterpolate=false;
	}
	if (bShouldInterpolate)
	{
		FrameToCheck=InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character=HitCharacter;
	return FrameToCheck;
}

//Rewind {1.CacheBox(현재 서버위치저장) 2. MoveBox(과거 패키지 박스 이동) 3.기존 캐릭터 충돌끄기}
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ACDCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter==nullptr) return FServerSideRewindResult();
	
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);
	
	UBoxComponent* HeadBox=HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
	
	FHitResult ConfirmHitResult;
	const FVector TraceEnd=TraceStart+(HitLocation-TraceStart)*1.25f;
	UWorld* World=GetWorld();
	if (World){
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
			);
		if (ConfirmHitResult.bBlockingHit)
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult(true,true);
		}
		else
		{
			for (auto& HitBoxPair:HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value!=nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox,ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
				);
			if (ConfirmHitResult.bBlockingHit)
			{
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult(true,false);
			}
		}
	}
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}


void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Test
	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);
	FrameHistory.AddHead(ThisFrame);
	if (GFrameCounter % 3 == 0)//엔진 내부의 프레임
	{
		ShowFramePackage(ThisFrame, FColor::Red);
	}
	
	//
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo:Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ACDCharacter* HitCharacter,
                                                                    const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

