// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "CDProject/GameMode/RoundGameMode.h"
#include "DamageType/DamageType_Explode.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CollisionBox=CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);

	
	//CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	
}

void AProjectile::PostEditChangeProperty(struct FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	FName PropertyName=Event.Property!=nullptr?Event.Property->GetFName():NAME_None;
	
	if (PropertyName==GET_MEMBER_NAME_CHECKED(AProjectile, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed=InitialSpeed;
			ProjectileMovementComponent->MaxSpeed=InitialSpeed;
		}
	}
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (Tracer)
	{
		TracerComponent=UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition);
	}
	CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	
	if (GetWorld())
	{
		if (GetWorld()->GetAuthGameMode())
		{
			if (Cast<ARoundGameMode>(GetWorld()->GetAuthGameMode()))
			{
				Cast<ARoundGameMode>(GetWorld()->GetAuthGameMode())->AddDestroyableActor(this);	
			}
		}
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::FinishedDestroyTimer,
		DestroyTime
		);
}

void AProjectile::FinishedDestroyTimer()
{
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent=UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
			);
	}
}

void AProjectile::ExplodeDamage()
{//Explode need Instigator
	APawn* FiringPawn=GetInstigator();
	if (FiringPawn&&HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,
					Damage,
					10.f,
					GetActorLocation(),
					DamageInnerRadius,
					DamageOuterRadius,
					1.f,
					UDamageType_Explode::StaticClass(),
					TArray<AActor*>(),
					this,
					FiringController
				);
		}
	}
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	if (ImpactParticle) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorLocation(), FRotator::ZeroRotator, FVector(10,10,10));
	if (ImpactSound) UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
}


void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();//Destroy -> Destroyed
}

