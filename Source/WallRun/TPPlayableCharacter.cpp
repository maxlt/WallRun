// Fill out your copyright notice in the Description page of Project Settings.

#include "TPPlayableCharacter.h"

#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace
{
	bool WallIsRunnable(const AActor& Wall)
	{
		return Wall.ActorHasTag("Wall");
	}
}

// Sets default values
ATPPlayableCharacter::ATPPlayableCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));

	Camera->SetupAttachment(SpringArm);
	SpringArm->SetupAttachment(GetRootComponent());

	SpringArm->bUsePawnControlRotation = true;
	SpringArm->SetUsingAbsoluteRotation(true);
	// Note: The followings are redundant if we're using Absolute Rotation.
	// SpringArm->bInheritRoll = false;
	// SpringArm->bInheritYaw = true;
	// SpringArm->bInheritPitch = true;

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->JumpZVelocity = 800.f;

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ATPPlayableCharacter::OnWallJumped);

	bIsRunningOnWall = false;
	ForwardInputAxis = 0.f;

	MinFacingAngle = 5.f; // 5 degree.
	MaxFacingAngle = 80.f; // 80 degree.

	CurrentRunDirection = FVector::ZeroVector;
	CurrentWallInfo = {}; // Zero-initialisation
}

// Called when the game starts or when spawned
void ATPPlayableCharacter::BeginPlay()
{
	Super::BeginPlay();

}

void ATPPlayableCharacter::OnWallJumped(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Hit.bBlockingHit && Hit.Actor.IsValid())
	{
		DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 30.f, 4.f, FColor::Yellow, false, 10.f, 0, 1.f);

		if (!(Hit.Actor->ActorHasTag("Wall")))
		{
			EndWallRun();
			return;
		}

		if (!bIsRunningOnWall && GetCharacterMovement()->IsFalling() /*&& IsMovingForward()*/)
		{
			UE_LOG(LogTemp, Log, TEXT("Preliminary Conditions: SUCCESS!"));

			// For now, we assume the angle is less than 90 and always facing at the wall.
			const auto FacingAngle = GetActorForwardVector() | Hit.ImpactNormal;
			if (FacingAngle < 0 && FacingAngle >= -0.985f) // We set -0.95 so that we don't wall run when facing directly at the wall.
			{
				UE_LOG(LogTemp, Log, TEXT("We begin Wall Running."));

				const auto ZDir = GetActorForwardVector() ^ Hit.ImpactNormal;
				const auto RunningDir = (Hit.ImpactNormal ^ ZDir).GetSafeNormal();
				CurrentRunDirection = RunningDir;
				CurrentWallInfo.Wall = Hit.Actor;
				CurrentWallInfo.WallNormal = Hit.ImpactNormal.GetSafeNormal();

				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + RunningDir * 100.f, 10.f, FColor::Magenta, false, 30.f, 0, 2.f);
				BeginWallRun();
			}
		}
	}
}

// Called every frame
void ATPPlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsRunningOnWall)
	{
		TickWallRunning();
	}
}

// Called to bind functionality to input
void ATPPlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("MoveForward", this, &ATPPlayableCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPPlayableCharacter::MoveRight);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATPPlayableCharacter::BeginJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ATPPlayableCharacter::EndJump);
}

void ATPPlayableCharacter::MoveForward(float Axis)
{
	ForwardInputAxis = Axis;

	const auto ViewRot = GetViewRotation();
	const auto YawOnlyViewRot = FRotator(0.f, ViewRot.Yaw, 0.f);
	const auto FacingViewDir = YawOnlyViewRot.Vector();
	AddMovementInput(FacingViewDir, Axis);
}

void ATPPlayableCharacter::MoveRight(float Axis)
{
	const auto ViewRot = GetViewRotation();
	const auto YawOnlyViewRot = FRotator(0.f, ViewRot.Yaw, 0.f);
	const auto RightwardViewDir = FRotationMatrix::Make(YawOnlyViewRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightwardViewDir, Axis);
}

void ATPPlayableCharacter::BeginJump()
{
	if (bIsRunningOnWall)
	{
		const auto XYPlane = (CurrentRunDirection * 0.25f + CurrentWallInfo.WallNormal).GetSafeNormal();
		const auto ZElevation = GetActorUpVector() * GetCharacterMovement()->JumpZVelocity * 0.6f;
		const auto LaunchVelocity = XYPlane * GetCharacterMovement()->GetMaxSpeed() + ZElevation;
		LaunchCharacter(LaunchVelocity, false, false);
		SetActorRotation(FRotationMatrix::MakeFromX(XYPlane).Rotator());
	}
	else
	{
		Jump(); // Note: ACharacter will internally check if this character *is able to* jump.
	}
}

void ATPPlayableCharacter::EndJump()
{
	StopJumping();
}

bool ATPPlayableCharacter::IsMovingForward() const
{
	return ForwardInputAxis > 0.f;
}

void ATPPlayableCharacter::BeginWallRun()
{
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->Velocity.Z = 0.f;

	bIsRunningOnWall = true;

	GetController()->SetIgnoreMoveInput(true);
	ConsumeMovementInputVector();

	SetActorRotation(FRotationMatrix::MakeFromX(CurrentRunDirection).Rotator());

	GetWorldTimerManager().SetTimer(RunningTimer, this, &ATPPlayableCharacter::EnableGravity, 1.5f);
}

void ATPPlayableCharacter::EndWallRun()
{
	GetCharacterMovement()->GravityScale = 1.f;

	bIsRunningOnWall = false;
	CurrentWallInfo.Clear();

	GetController()->SetIgnoreMoveInput(false);
	GetWorldTimerManager().ClearTimer(RunningTimer);
}

void ATPPlayableCharacter::TickWallRunning()
{
	const float Scale = GetCapsuleComponent()->GetUnscaledCapsuleRadius() + 2.f;
	const FVector TraceLineEnd = GetActorLocation() - CurrentWallInfo.WallNormal * Scale;

	FCollisionQueryParams CollParams;
	CollParams.AddIgnoredActor(this);

	FHitResult WallTestResult;
	GetWorld()->LineTraceSingleByChannel(WallTestResult, GetActorLocation(), TraceLineEnd, ECC_Visibility, CollParams);

	if (WallTestResult.bBlockingHit && WallTestResult.Actor.IsValid() && WallTestResult.Actor->ActorHasTag("Wall"))
	{
		UE_LOG(LogTemp, Log, TEXT("TickWallRunning(): We hit a wall!"));

		// Here the player didn't press to jump.
		if (WallTestResult.Actor == CurrentWallInfo.Wall)
		{
			GetCharacterMovement()->Velocity.X = CurrentRunDirection.X * GetCharacterMovement()->GetMaxSpeed();
			GetCharacterMovement()->Velocity.Y = CurrentRunDirection.Y * GetCharacterMovement()->GetMaxSpeed();
			UE_LOG(LogTemp, Log, TEXT("TickWallRunning(): This wall is the same as before!"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("TickWallRunning(): This wall is different now!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("TickWallRunning(): We hit nadda."));
		EndWallRun();
	}
}

void ATPPlayableCharacter::EnableGravity()
{
	UE_LOG(LogTemp, Log, TEXT("Enable Gravity"));
	GetCharacterMovement()->GravityScale = 0.25f;
}
