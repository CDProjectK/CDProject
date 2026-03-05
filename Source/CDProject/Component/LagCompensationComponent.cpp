// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

#include "CDProject/Character/CDCharacter.h"


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
}

void ULagCompensationComponent::SaveFramePackage()
{
}
void ULagCompensationComponent::ServerScoreRequest_Implementation(ACDCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
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

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ACDCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter==nullptr) return FServerSideRewindResult();
	
	FFramePackage CurrentFrame;
}


void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

