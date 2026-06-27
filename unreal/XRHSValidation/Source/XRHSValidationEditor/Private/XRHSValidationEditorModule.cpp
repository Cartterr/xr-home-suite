#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/Commands/UIAction.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "PlayInEditorDataTypes.h"
#include "Styling/AppStyle.h"
#include "Subsystems/EditorSubsystemBlueprintLibrary.h"
#include "ToolMenus.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "XRHSValidationEditor"

namespace XRHSValidationEditor
{
	const FName LinkPreviewPlatform(TEXT("VULKAN_ES3_1_ANDROID Android_OpenXR"));

	bool IsPlaySessionActive()
	{
		return GEditor && GEditor->PlayWorld;
	}

	bool CanStartVRPreview()
	{
		return GUnrealEd && GEditor && !IsPlaySessionActive() && !GEditor->IsLightingBuildCurrentlyRunning();
	}

	bool CanStopVRPreview()
	{
		return IsPlaySessionActive();
	}

	void SetCVar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Variable->Set(Value, ECVF_SetByCode);
		}
	}

	void PrepareLinkPreview()
	{
		UEditorSubsystemBlueprintLibrary::SetPreviewPlatform(LinkPreviewPlatform);
		SetCVar(TEXT("r.Mobile.PropagateAlpha"), 1);
		SetCVar(TEXT("r.PostProcessing.PropagateAlpha"), 1);
		SetCVar(TEXT("r.Mobile.TonemapSubpass"), 1);
	}

	void StartVRPreview()
	{
		if (!CanStartVRPreview())
		{
			return;
		}

		PrepareLinkPreview();

		FRequestPlaySessionParams SessionParams;
		SessionParams.SessionPreviewTypeOverride = EPlaySessionPreviewType::VRPreview;
		GUnrealEd->RequestPlaySession(SessionParams);
	}

	void StopVRPreview()
	{
		if (GEditor && IsPlaySessionActive())
		{
			GEditor->RequestEndPlayMap();
		}
	}

	FUIAction StartAction()
	{
		return FUIAction(
			FExecuteAction::CreateStatic(&StartVRPreview),
			FCanExecuteAction::CreateStatic(&CanStartVRPreview));
	}

	FUIAction StopAction()
	{
		return FUIAction(
			FExecuteAction::CreateStatic(&StopVRPreview),
			FCanExecuteAction::CreateStatic(&CanStopVRPreview));
	}
}

class FXRHSValidationEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FXRHSValidationEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		if (UToolMenu* StatusBar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.StatusBar.ToolBar")))
		{
			FToolMenuSection& Section = StatusBar->AddSection(
				TEXT("XRHomeSuite"),
				FText::GetEmpty(),
				FToolMenuInsert(TEXT("SourceControl"), EToolMenuInsertType::Before));

			Section.AddEntry(FToolMenuEntry::InitToolBarButton(
				TEXT("XRHSStartVRPreview"),
				XRHSValidationEditor::StartAction(),
				LOCTEXT("StartVRPreviewLabel", "XRHS Start VR"),
				LOCTEXT("StartVRPreviewTooltip", "Start XR Home Suite VR Preview using Android OpenXR Link preview settings."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("PlayWorld.PlayInVR"))));

			Section.AddEntry(FToolMenuEntry::InitToolBarButton(
				TEXT("XRHSStopVRPreview"),
				XRHSValidationEditor::StopAction(),
				LOCTEXT("StopVRPreviewLabel", "XRHS Stop"),
				LOCTEXT("StopVRPreviewTooltip", "Stop the active XR Home Suite VR Preview session."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("PlayWorld.StopPlaySession.Small"))));
		}

		if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
		{
			FToolMenuSection& Section = ToolsMenu->AddSection(
				TEXT("XRHomeSuite"),
				LOCTEXT("XRHomeSuiteMenuSection", "XR Home Suite"));

			Section.AddEntry(FToolMenuEntry::InitMenuEntry(
				TEXT("XRHSStartVRPreview"),
				LOCTEXT("StartVRPreviewMenuLabel", "Start XRHS VR Preview"),
				LOCTEXT("StartVRPreviewMenuTooltip", "Start VR Preview with Android OpenXR Link passthrough settings."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("PlayWorld.PlayInVR")),
				XRHSValidationEditor::StartAction()));

			Section.AddEntry(FToolMenuEntry::InitMenuEntry(
				TEXT("XRHSStopVRPreview"),
				LOCTEXT("StopVRPreviewMenuLabel", "Stop XRHS VR Preview"),
				LOCTEXT("StopVRPreviewMenuTooltip", "Stop the active XR Home Suite VR Preview session."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("PlayWorld.StopPlaySession.Small")),
				XRHSValidationEditor::StopAction()));
		}
	}
};

IMPLEMENT_MODULE(FXRHSValidationEditorModule, XRHSValidationEditor)

#undef LOCTEXT_NAMESPACE
