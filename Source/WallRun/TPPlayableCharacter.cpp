// Fill out your copyright notice in the Description page of Project Settings.

#include "TPPlayableCharacter.h"

#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
	GetCharacterMovement()->JumpZVelocity = 600.f;

	bIsRunningOnWall = false;
	ForwardInputAxis = 0.f;

	MinFacingAngle = 5.f; // 5 degree.
	MaxFacingAngle = 80.f; // 80 degree.

	CurrentRunDirection = FVector::ZeroVector;
	CurrentWallInfo = {}; // Zero-initialisation

	RotSpeed = 50.f;

	DesiredFacingDirection = FVector::ZeroVector;
}

// Called when the game starts or when spawned
void ATPPlayableCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ATPPlayableCharacter::OnCollided);
	LandedDelegate.AddDynamic(this, &ATPPlayableCharacter::ResetSomething);
}

void ATPPlayableCharacter::OnCollided(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Hit.bBlockingHit && Hit.Actor.IsValid())
	{
		DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 30.f, 4.f, FColor::Yellow, false, 10.f, 0, 1.f);

		if (bIsRunningOnWall && (Hit.Actor != CurrentWallInfo.Wall))
		{
			// If the character, while wall running, runs into anything else besides the current wall, stop wall running.
			EndWallRun();
			return;
		}

		// When the character made a jump from the ground towards a wall.
		// TODO Use a better method than using a "Wall" tag.
 		if (!bIsRunningOnWall && GetCharacterMovement()->IsFalling() && Hit.Actor->ActorHasTag("Wall"))
		{
			const auto FacingAngle = GetActorForwardVector() | -Hit.ImpactNormal;
			UE_LOG(LogTemp, Log, TEXT("Facing Angle From Wall Normal = %f"), FMath::RadiansToDegrees(FMath::Acos(FacingAngle)));
			const auto MaxDotProduct = FMath::Cos(FMath::DegreesToRadians(MinFacingAngle));
			const auto MinDotProduct = FMath::Cos(FMath::DegreesToRadians(MaxFacingAngle));
			// Be careful with the following inequalities. Due to the inversion of cosine, you wouldn't want to mess with the conditional. 
			if (FacingAngle > MinDotProduct && FacingAngle < MaxDotProduct)
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

void ATPPlayableCharacter::ResetSomething(const FHitResult&)
{
	DesiredFacingDirection = FVector::ZeroVector;
	GetController()->SetIgnoreMoveInput(false);
}

// Called every frame
void ATPPlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsRunningOnWall)
	{
		TickWallRunning();
	}

	if (!DesiredFacingDirection.IsZero())
	{
		const auto NewDir = FMath::VInterpNormalRotationTo(GetActorForwardVector(), DesiredFacingDirection, DeltaTime, RotSpeed);
		SetActorRotation(FRotationMatrix::MakeFromX(NewDir).Rotator());
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
		// TODO We should parameterise the angles of the launch direction.
		const auto XYPlane = (CurrentRunDirection * 0.25f + CurrentWallInfo.WallNormal).GetSafeNormal();
		const auto ZElevation = GetActorUpVector() * GetCharacterMovement()->JumpZVelocity * 0.6f;
		const auto LaunchVelocity = XYPlane * GetCharacterMovement()->GetMaxSpeed() + ZElevation;
		LaunchCharacter(LaunchVelocity, false, false);
		DesiredFacingDirection = XYPlane;
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

	DesiredFacingDirection = CurrentRunDirection;

	bIsRunningOnWall = true;

	GetController()->SetIgnoreMoveInput(true);
	ConsumeMovementInputVector();

	GetWorldTimerManager().SetTimer(RunningTimer, [this]()->void
	{
		UE_LOG(LogTemp, Log, TEXT("Enable 25%% Gravity"));
		GetCharacterMovement()->GravityScale = 0.25f;
	}, 1.5f, false);
}

void ATPPlayableCharacter::EndWallRun()
{
	GetCharacterMovement()->GravityScale = GetDefault<UCharacterMovementComponent>()->GravityScale;

	bIsRunningOnWall = false;
	CurrentWallInfo.Clear();

	GetWorldTimerManager().ClearTimer(RunningTimer);
}

void ATPPlayableCharacter::TickWallRunning()
{
	// We check if the character's still "touching" the same wall.
	// If it no longer does, stop wall running.
	if (CurrentWallInfo.Wall.IsValid())
	{
		const float Scale = GetCapsuleComponent()->GetUnscaledCapsuleRadius() + 2.f;
		const FVector TraceLineEnd = GetActorLocation() - CurrentWallInfo.WallNormal * Scale;
		
		FCollisionQueryParams CollParams;
		CollParams.AddIgnoredActor(this);

		FHitResult WallTestResult;
		
		if (CurrentWallInfo.Wall->ActorLineTraceSingle(WallTestResult, GetActorLocation(), TraceLineEnd, ECC_Visibility, CollParams)
			&& (WallTestResult.Actor == CurrentWallInfo.Wall))
		{
			GetCharacterMovement()->Velocity.X = CurrentRunDirection.X * GetCharacterMovement()->GetMaxSpeed();
			GetCharacterMovement()->Velocity.Y = CurrentRunDirection.Y * GetCharacterMovement()->GetMaxSpeed();
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Update: We hit something else besides wall %s"), *(CurrentWallInfo.Wall->GetName()));
			EndWallRun();
		}
	}
}
