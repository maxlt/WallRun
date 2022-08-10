// Fill out your copyright notice in the Description page of Project Settings.

#include "TPPlayableCharacter.h"

#include "DrawDebugHelpers.h"
#include "RunnableWall.h"
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

	bIsRunningOnWall = false;
	bJumped = false;

	MinFacingAngle = 5.f;
	MaxFacingAngle = 80.f;
	
	MaxDistance = GetDefault<UCharacterMovementComponent>()->MaxWalkSpeed;

	CurrentRunDirection = FVector::ZeroVector;
	DesiredFacingDirection = FVector::ZeroVector;

	HPlaneLaunchAngle = 45.f;
	ZElevationLaunchAngle = 75.f;

	Wall = nullptr;
}

// Called when the game starts or when spawned
void ATPPlayableCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ATPPlayableCharacter::OnCollided);
	LandedDelegate.AddDynamic(this, &ATPPlayableCharacter::HandleOnLanded);
}

void ATPPlayableCharacter::OnCollided(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 30.f, 4.f, FColor::Yellow, false, 10.f, 0, 1.f);

	if (bIsRunningOnWall && OtherActor && (OtherActor != Wall))
	{
		// If the character, while wall running, runs into anything else besides the current wall, stop wall running.
		EndWallRun();
		return;
	}

	Wall = Cast<ARunnableWall>(OtherActor);
	
	// When the character made a jump from the ground and towards a wall.
 	if (!bIsRunningOnWall && bJumped && Wall && Wall->IsOnFacingSide(GetActorForwardVector()))
	{
		const auto FacingAngle = GetActorForwardVector() | -Hit.ImpactNormal;
		UE_LOG(LogTemp, Log, TEXT("Facing Angle From Wall Normal = %f"), FMath::RadiansToDegrees(FMath::Acos(FacingAngle)));
 		
		const auto MaxDotProduct = FMath::Cos(FMath::DegreesToRadians(MinFacingAngle));
		const auto MinDotProduct = FMath::Cos(FMath::DegreesToRadians(MaxFacingAngle));
 		
		// Be careful with the following conditional. Due to the inversion of inequalities caused by cosine, you wouldn't want to mess with it. 
		if (FacingAngle > MinDotProduct && FacingAngle < MaxDotProduct)
		{
			UE_LOG(LogTemp, Log, TEXT("We begin Wall Running."));

			const auto ZDir = GetActorForwardVector() ^ Hit.ImpactNormal;
			const auto RunningDir = (Hit.ImpactNormal ^ ZDir).GetSafeNormal();
			CurrentRunDirection = RunningDir;

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + RunningDir * 100.f, 10.f, FColor::Magenta, false, 30.f, 0, 2.f);

			BeginWallRun();
			bJumped = false;
		}
	}
}

void ATPPlayableCharacter::HandleOnLanded(const FHitResult&)
{
	// When this character lands from wall running, we reset some states.
	DesiredFacingDirection = FVector::ZeroVector;
	// Best to call this specific function since SetIgnoreMoveInput may have been called more than once if the character jumps
	// from wall to wall.
	GetController()->ResetIgnoreMoveInput();
	bJumped = false;
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
		// We smoothly rotate the character as soon as wall running begins and when the character "jumps off" the wall.
		const auto InterpDir = FMath::VInterpNormalRotationTo(GetActorForwardVector(), DesiredFacingDirection, DeltaTime, 500.f);
		SetActorRotation(FRotationMatrix::MakeFromX(InterpDir).Rotator());
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
	bJumped = true;
	
	if (bIsRunningOnWall && Wall)
	{
		// We launch the character when a jump is called while wall running. Launching this way gives us a control over its "jump" direction.
		const auto HPlaneLaunchDir = CurrentRunDirection * FMath::Cos(FMath::DegreesToRadians(HPlaneLaunchAngle))
			+ Wall->FacingSide() * FMath::Sin(FMath::DegreesToRadians(HPlaneLaunchAngle));
		check(HPlaneLaunchDir.IsNormalized());
		
		const auto ZElevationLaunchDir = GetActorUpVector() * FMath::Sin(FMath::DegreesToRadians(ZElevationLaunchAngle));
		
		const auto LaunchVelocity =
			HPlaneLaunchDir * FMath::Cos(FMath::DegreesToRadians(ZElevationLaunchAngle)) * GetCharacterMovement()->GetMaxSpeed()
			+ ZElevationLaunchDir * GetCharacterMovement()->JumpZVelocity;
		
		LaunchCharacter(LaunchVelocity, false, true);
		DesiredFacingDirection = HPlaneLaunchDir;
	}
	else
	{
		Jump(); // ACharacter will internally check if this character *is able to* jump.
	}
}

void ATPPlayableCharacter::EndJump()
{
	StopJumping();
}

void ATPPlayableCharacter::BeginWallRun()
{
	// We set zero-gravity so that the character can "float" along the wall.
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->Velocity.Z = 0.f;

	DesiredFacingDirection = CurrentRunDirection;

	bIsRunningOnWall = true;

	GetController()->SetIgnoreMoveInput(true);
	ConsumeMovementInputVector();

	check(GetCharacterMovement()->GetMaxSpeed() != 0.f);
	const auto TriggerDelay = MaxDistance/GetCharacterMovement()->GetMaxSpeed();
	GetWorldTimerManager().SetTimer(GravitySuspendTimer, [this]()->void
	{
		UE_LOG(LogTemp, Log, TEXT("Set 25%% Gravity"));
		GetCharacterMovement()->GravityScale = 0.25f;
	}, TriggerDelay, false);
}

void ATPPlayableCharacter::EndWallRun()
{
	GetCharacterMovement()->GravityScale = GetDefault<UCharacterMovementComponent>()->GravityScale;

	bIsRunningOnWall = false;
	Wall = nullptr;
	
	GetWorldTimerManager().ClearTimer(GravitySuspendTimer);
}

void ATPPlayableCharacter::TickWallRunning()
{
	if (Wall)
	{
		// The extra 2.0 extends the trace line beyond the capsule radius so that the trace can hit-test a wall.
		const float Extend = GetCapsuleComponent()->GetUnscaledCapsuleRadius() + 2.f;
		const FVector TraceLineEnd = GetActorLocation() - Wall->FacingSide() * Extend;

		FCollisionQueryParams CollParams;
		CollParams.AddIgnoredActor(this);

		FHitResult WallTestResult;

		// We check if the character still "touches" the same wall. If it no longer does, or if there's no wall to "touch", we stop wall running.
		if (Wall->ActorLineTraceSingle(WallTestResult, GetActorLocation(), TraceLineEnd, ECC_Visibility, CollParams)
			&& (WallTestResult.GetActor() == Wall))
		{
			// As long as the character still does, we enforces its velocity.
			GetCharacterMovement()->Velocity.X = CurrentRunDirection.X * GetCharacterMovement()->GetMaxSpeed();
			GetCharacterMovement()->Velocity.Y = CurrentRunDirection.Y * GetCharacterMovement()->GetMaxSpeed();
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Update: We hit something else besides wall %s"), *(Wall->GetName()));
			EndWallRun();
		}
	}
}
