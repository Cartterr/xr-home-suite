#include "XRHSValidationPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "InputCoreTypes.h"
#include "IXRTrackingSystem.h"
#include "MotionControllerComponent.h"
#include "OculusXRFunctionLibrary.h"
#include "OculusXRPersistentPassthroughInstance.h"
#include "OculusXRPassthroughLayerComponent.h"
#include "OculusXRPassthroughSubsystem.h"
#include "RHIGlobals.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float GridExtentCm = 300.0f;
constexpr float GridStepCm = 50.0f;
constexpr float GridLineThicknessCm = 1.0f;

const TCHAR* BoolText(bool bValue)
{
	return bValue ? TEXT("yes") : TEXT("no");
}

void SetCVar(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByCode);
	}
}
}

AXRHSValidationPawn::AXRHSValidationPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("HmdCamera"));
	Camera->SetupAttachment(SceneRoot);
	Camera->SetRelativeLocation(FVector::ZeroVector);
	Camera->bLockToHmd = true;

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(SceneRoot);
	LeftController->SetTrackingMotionSource(TEXT("Left"));

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(SceneRoot);
	RightController->SetTrackingMotionSource(TEXT("Right"));

	GridLines = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ValidationGrid"));
	GridLines->SetupAttachment(SceneRoot);
	GridLines->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		GridLines->SetStaticMesh(CubeMesh.Object);
	}

	DebugPanel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugPanel"));
	DebugPanel->SetupAttachment(Camera);
	DebugPanel->SetRelativeLocation(FVector(150.0f, -45.0f, -35.0f));
	DebugPanel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	DebugPanel->SetHorizontalAlignment(EHTA_Left);
	DebugPanel->SetVerticalAlignment(EVRTA_TextTop);
	DebugPanel->SetTextRenderColor(FColor(32, 255, 192, 255));
	DebugPanel->SetWorldSize(8.5f);
}

void AXRHSValidationPawn::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFloorTrackingOrigin();
	StartupRecenterDelaySeconds = 0.25f;
	ApplyPassthroughCompositingSettings();
	BuildValidationGrid();
	TryStartPassthrough();
	UpdateDebugPanel(0.0f);
}

void AXRHSValidationPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (StartupRecenterDelaySeconds > 0.0f)
	{
		StartupRecenterDelaySeconds -= DeltaSeconds;
		if (StartupRecenterDelaySeconds <= 0.0f)
		{
			RecenterFloorTrackingOrigin();
		}
	}
	UpdateDebugPanel(DeltaSeconds);
}

void AXRHSValidationPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AXRHSValidationPawn::ToggleDebugPanel);
	PlayerInputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AXRHSValidationPawn::TogglePassthroughPlacement);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &AXRHSValidationPawn::CycleDepthMode);
	PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &AXRHSValidationPawn::ToggleHandsRequested);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AXRHSValidationPawn::RecenterFloorTrackingOrigin);
}

void AXRHSValidationPawn::BuildValidationGrid()
{
	GridLines->ClearInstances();
	if (!GridLines->GetStaticMesh())
	{
		return;
	}

	const FVector XLineScale(GridExtentCm / 50.0f, GridLineThicknessCm / 100.0f, GridLineThicknessCm / 100.0f);
	const FVector YLineScale(GridLineThicknessCm / 100.0f, GridExtentCm / 50.0f, GridLineThicknessCm / 100.0f);

	for (float Offset = -GridExtentCm; Offset <= GridExtentCm; Offset += GridStepCm)
	{
		GridLines->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Offset, 0.0f), XLineScale));
		GridLines->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Offset, 0.0f, 0.0f), YLineScale));
	}
}

void AXRHSValidationPawn::CycleDepthMode()
{
	switch (DepthMode)
	{
	case EXRHSDepthMode::Off:
		DepthMode = EXRHSDepthMode::Low;
		break;
	case EXRHSDepthMode::Low:
		DepthMode = EXRHSDepthMode::Adaptive;
		break;
	case EXRHSDepthMode::Adaptive:
		DepthMode = EXRHSDepthMode::Off;
		break;
	}
}

void AXRHSValidationPawn::ToggleDebugPanel()
{
	bDebugPanelVisible = !bDebugPanelVisible;
	DebugPanel->SetVisibility(bDebugPanelVisible);
}

void AXRHSValidationPawn::ToggleHandsRequested()
{
	bHandsRequested = !bHandsRequested;
	LeftController->SetVisibility(bHandsRequested);
	RightController->SetVisibility(bHandsRequested);
}

void AXRHSValidationPawn::TogglePassthroughPlacement()
{
	bPassthroughOverlayMode = !bPassthroughOverlayMode;
	TryStartPassthrough();
}

void AXRHSValidationPawn::ConfigureFloorTrackingOrigin()
{
	Camera->SetRelativeLocation(FVector::ZeroVector);

	if (!GEngine || !GEngine->XRSystem.IsValid())
	{
		TrackingOriginState = TEXT("XR system unavailable");
		return;
	}

	GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
	TrackingOriginState = FString::Printf(TEXT("%s requested"), *TrackingOriginText());
}

void AXRHSValidationPawn::RecenterFloorTrackingOrigin()
{
	ConfigureFloorTrackingOrigin();

	if (GEngine && GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->ResetOrientationAndPosition(0.0f);
		TrackingOriginState = FString::Printf(TEXT("%s recentered"), *TrackingOriginText());
	}
}

void AXRHSValidationPawn::ApplyPassthroughCompositingSettings()
{
	SetCVar(TEXT("xr.OpenXRInvertAlpha"), 1);
	SetCVar(TEXT("xr.OpenXREnvironmentBlendMode"), 3);
	SetCVar(TEXT("r.Mobile.PropagateAlpha"), 1);
	SetCVar(TEXT("r.PostProcessing.PropagateAlpha"), 1);
	SetCVar(TEXT("OpenXR.AlphaInvertPass"), 1);

	const IConsoleVariable* PropagateAlpha = IConsoleManager::Get().FindConsoleVariable(TEXT("r.PostProcessing.PropagateAlpha"));
	const IConsoleVariable* MobilePropagateAlpha = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Mobile.PropagateAlpha"));
	const IConsoleVariable* EnvironmentBlend = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.OpenXREnvironmentBlendMode"));
	CompositingState = FString::Printf(
		TEXT("alpha pc=%s mobile=%s blend=%d"),
		BoolText(PropagateAlpha && PropagateAlpha->GetBool()),
		BoolText(MobilePropagateAlpha && MobilePropagateAlpha->GetBool()),
		EnvironmentBlend ? EnvironmentBlend->GetInt() : 0);
}

void AXRHSValidationPawn::TryStartPassthrough()
{
	if (!UOculusXRFunctionLibrary::IsPassthroughSupported())
	{
		PassthroughState = TEXT("not reported by runtime");
		return;
	}

	UOculusXRPassthroughSubsystem* PassthroughSubsystem = UOculusXRPassthroughSubsystem::GetPassthroughSubsystem(GetWorld());
	if (!PassthroughSubsystem)
	{
		PassthroughState = TEXT("subsystem unavailable");
		return;
	}

	FOculusXRPersistentPassthroughParameters Parameters;
	Parameters.bVisible = true;
	Parameters.Priority = 0;
	Parameters.Shape = NewObject<UOculusXRStereoLayerShapeReconstructed>(this);
	Parameters.Shape->LayerOrder = bPassthroughOverlayMode ? PassthroughLayerOrder_Overlay : PassthroughLayerOrder_Underlay;
	Parameters.Shape->TextureOpacityFactor = bPassthroughOverlayMode ? 0.85f : 1.0f;
	Parameters.ApplyShape();

	FOculusXRPassthrough_LayerResumed_Single ResumeDelegate;
	ResumeDelegate.BindDynamic(this, &AXRHSValidationPawn::HandlePassthroughResumed);
	PassthroughInstance = PassthroughSubsystem->InitializePersistentPassthrough(Parameters, ResumeDelegate);
	PassthroughState = PassthroughInstance && PassthroughInstance->IsVisible()
		? FString::Printf(TEXT("%s requested"), bPassthroughOverlayMode ? TEXT("overlay") : TEXT("underlay"))
		: TEXT("create failed");
}

void AXRHSValidationPawn::HandlePassthroughResumed()
{
	PassthroughState = FString::Printf(TEXT("%s resumed"), bPassthroughOverlayMode ? TEXT("overlay") : TEXT("underlay"));
}

void AXRHSValidationPawn::UpdateDebugPanel(float DeltaSeconds)
{
	const float FrameMs = DeltaSeconds > 0.0f ? DeltaSeconds * 1000.0f : 0.0f;
	SmoothedFrameMs = SmoothedFrameMs <= 0.0f ? FrameMs : FMath::Lerp(SmoothedFrameMs, FrameMs, 0.08f);
	const float Fps = SmoothedFrameMs > 0.0f ? 1000.0f / SmoothedFrameMs : 0.0f;
	const FString XRSystemName = GEngine && GEngine->XRSystem.IsValid()
		? GEngine->XRSystem->GetSystemName().ToString()
		: TEXT("none");
	const FString AdapterName = GRHIAdapterName.IsEmpty() ? TEXT("unknown") : GRHIAdapterName;
	const FVector CameraWorldLocation = Camera ? Camera->GetComponentLocation() : FVector::ZeroVector;
	const FVector CameraRelativeLocation = Camera ? Camera->GetRelativeLocation() : FVector::ZeroVector;

	const FString Panel = FString::Printf(
		TEXT("XR Home Suite\nXR %s connected=%s enabled=%s\nGPU %s\nFrame %.2f ms %.1f fps\nTracking %s\nCamera Z world=%.1f rel=%.1f\nPassthrough %s\nComposite %s\nHands requested=%s L=%s R=%s\nDepth %s"),
		*XRSystemName,
		UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected() ? TEXT("yes") : TEXT("no"),
		UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() ? TEXT("yes") : TEXT("no"),
		*AdapterName,
		SmoothedFrameMs,
		Fps,
		*TrackingOriginState,
		CameraWorldLocation.Z,
		CameraRelativeLocation.Z,
		*PassthroughState,
		*CompositingState,
		bHandsRequested ? TEXT("yes") : TEXT("no"),
		*TrackingText(LeftController),
		*TrackingText(RightController),
		*DepthModeText());

	DebugPanel->SetText(FText::FromString(Panel));
}

FString AXRHSValidationPawn::DepthModeText() const
{
	switch (DepthMode)
	{
	case EXRHSDepthMode::Off:
		return TEXT("off");
	case EXRHSDepthMode::Low:
		return TEXT("low requested");
	case EXRHSDepthMode::Adaptive:
		return TEXT("adaptive requested");
	}
	return TEXT("unknown");
}

FString AXRHSValidationPawn::TrackingOriginText() const
{
	if (!GEngine || !GEngine->XRSystem.IsValid())
	{
		return TEXT("none");
	}

	switch (GEngine->XRSystem->GetTrackingOrigin())
	{
	case EHMDTrackingOrigin::View:
		return TEXT("view");
	case EHMDTrackingOrigin::LocalFloor:
		return TEXT("local floor");
	case EHMDTrackingOrigin::Local:
		return TEXT("local");
	case EHMDTrackingOrigin::Stage:
		return TEXT("stage");
	case EHMDTrackingOrigin::CustomOpenXR:
		return TEXT("custom");
	}

	return TEXT("unknown");
}

FString AXRHSValidationPawn::TrackingText(const UMotionControllerComponent* MotionController) const
{
	return MotionController && MotionController->IsTracked() ? TEXT("tracked") : TEXT("waiting");
}
