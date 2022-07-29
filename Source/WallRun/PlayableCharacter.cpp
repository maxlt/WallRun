// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayableCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values
APlayableCharacter::APlayableCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraEye = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera 1P"));
	CameraEye->SetupAttachment(GetRootComponent());
	CameraEye->bUsePawnControlRotation = true;
	CameraEye->AddLocalOffset(FVector{ 0, 0, BaseEyeHeight });

	GetMesh()->SetupAttachment(CameraEye);

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &APlayableCharacter::OnWallHit);

	bIsRunningOnWall = false;
	bSetUpdateWallRunningToTick = false;
	MoveForwardScale = 0.f;
	MoveRightScale = 0.f;

	GetCharacterMovement()->JumpZVelocity = 600.f;
}

// Called when the game starts or when spawned
void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayableCharacter::OnWallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Hit.bBlockingHit)
	{
		UE_LOG(LogTemp, Log, TEXT("Player's Capsule Hit A Something."));
		DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint, 35.f * Hit.ImpactNormal + Hit.ImpactPoint, 5.0f, FColor::Yellow, false, 5.0f, 0, 1.0f);

		if (bIsRunningOnWall)
		{
			// We shouldn't do anything if the player's currently running on wall.
			return;
		}

		if (IsWallRunnable(Hit))
		{
			// Is the player jumping?
			if (GetCharacterMovement()->IsFalling())
			{
				// We look for which side of the wall the player is jumping towards.
				// Then we get the desired wall run direction.
				DesiredWallRunDirection = GetRunDirection(Hit.ImpactNormal);
				CurrentWallOn = GetWallSide(Hit.ImpactNormal);

				if (AreInputsValid())
				{
					UE_LOG(LogTemp, Log, TEXT("We begin wall running!"));
					BeginWallRunning();
				}
			}
		}
	}
}

// Called every frame
void APlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSetUpdateWallRunningToTick)
	{
		UE_LOG(LogTemp, Log, TEXT("Wall running ticking."));
		if (GetCharacterMovement()->IsFalling())
			UE_LOG(LogTemp, Log, TEXT("Player is falling."));

		UpdateWallRunning();
	}
}

// Called to bind functionality to input
void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &APlayableCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayableCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APlayableCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayableCharacter::LookUp);
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &APlayableCharacter::BeginJump);
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &APlayableCharacter::EndJump);
}

void APlayableCharacter::MoveForward(float Scale)
{
	MoveForwardScale = Scale;
	AddMovementInput(GetActorForwardVector(), Scale);
}

void APlayableCharacter::MoveRight(float Scale)
{
	MoveRightScale = Scale;
	AddMovementInput(GetActorRightVector(), Scale);
}

void APlayableCharacter::Turn(float Scale)
{
	AddControllerYawInput(Scale);
}

void APlayableCharacter::LookUp(float Scale)
{
	AddControllerPitchInput(Scale);
}

void APlayableCharacter::BeginJump()
{
	if (Super::CanJump())
	{
		// Set this player to jump on the next movement tick.
		Super::Jump();

		if (bIsRunningOnWall)
		{
			EndWallRunning();
		}
	}
}

void APlayableCharacter::EndJump()
{
	Super::StopJumping();
}

bool APlayableCharacter::AreInputsValid() const
{
	const auto bIsForwardValid = MoveForwardScale > 0.f;
	const auto bIsStrafeValid = (CurrentWallOn == EWallSide::Left) ? (MoveRightScale > 0.f) : (MoveRightScale < 0.f);
	return bIsForwardValid && bIsStrafeValid;
}

bool APlayableCharacter::IsWallRunnable(const FHitResult& SurfaceWall) const
{
	if (!SurfaceWall.Actor.IsValid())
	{
		return false;
	}

	return SurfaceWall.Actor->ActorHasTag("Wall");
}

void APlayableCharacter::UpdateWallRunning()
{
	if (!AreInputsValid())
	{
		EndWallRunning();
	}
	else
	{
		FHitResult HitResult;
		const FVector ZDir = (CurrentWallOn == EWallSide::Left) ? -FVector::ZAxisVector : FVector::ZAxisVector;
		const FVector TraceLine = (DesiredWallRunDirection ^ ZDir) * (GetCapsuleComponent()->GetUnscaledCapsuleRadius() + 1.f);
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		GetWorld()->LineTraceSingleByChannel(HitResult, GetActorLocation(), GetActorLocation() + TraceLine, ECC_Visibility, QueryParams);

		if (HitResult.bBlockingHit)
		{
			if (GetWallSide(HitResult.ImpactNormal) == CurrentWallOn)
			{
				DesiredWallRunDirection = GetRunDirection(HitResult.ImpactNormal);
				// TODO Use GetMaxSpeed instead.
				const auto DesiredMaxSpeed = GetCharacterMovement()->GetMaxSpeed();
				UE_LOG(LogTemp, Log, TEXT("Desired Max Speed = %f"), DesiredMaxSpeed);
				GetCharacterMovement()->Velocity = DesiredMaxSpeed * DesiredWallRunDirection;
			}
			else
			{
				EndWallRunning();
			}
		}
		else
		{
			EndWallRunning();
		}
	}
}

FVector APlayableCharacter::GetRunDirection(const FVector& SurfaceWallNormal) const
{
	const FVector2D HorizontalWallNormal = FVector2D(SurfaceWallNormal);
	const FVector2D HorizontalPlayerRight = FVector2D(GetActorRightVector());
	const EWallSide WallSide = ((HorizontalWallNormal | HorizontalPlayerRight) > -0.05f) ? EWallSide::Right : EWallSide::Left;

	const FVector ZDir = (WallSide == EWallSide::Left) ? -FVector::ZAxisVector : FVector::ZAxisVector;

	return SurfaceWallNormal ^ ZDir;
}

EWallSide APlayableCharacter::GetWallSide(const FVector& SurfaceWallNormal) const
{
	const FVector2D HorizontalWallNormal = FVector2D(SurfaceWallNormal);
	const FVector2D HorizontalPlayerRight = FVector2D(GetActorRightVector());
	const EWallSide WallSide = ((HorizontalWallNormal | HorizontalPlayerRight) > -0.05f) ? EWallSide::Right : EWallSide::Left;
	return WallSide;
}

void APlayableCharacter::BeginWallRunning()
{
	GetCharacterMovement()->AirControl = 1.f;
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::ZAxisVector);
	bIsRunningOnWall = true;
	bSetUpdateWallRunningToTick = true;
}

void APlayableCharacter::EndWallRunning()
{
	GetCharacterMovement()->AirControl = 0.05f;
	GetCharacterMovement()->GravityScale = 1.f;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::ZeroVector);
	bIsRunningOnWall = false;
	bSetUpdateWallRunningToTick = false;
}
