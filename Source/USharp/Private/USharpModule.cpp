#include "USharpPCH.h"
#include "CSharpLoader.h"
#include "SharpClass.h"

#include "SharpSettings.h"
#include "Developer/Settings/Public/ISettingsModule.h"
#include "Developer/Settings/Public/ISettingsSection.h"

#define LOCTEXT_NAMESPACE "FUSharpModule"

class FUSharpModule : public IUSharp
{
private:
	TSharedPtr<ISettingsSection> SettingsSection;
public:
	void RegisterSettings()
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "USharp",
				LOCTEXT("USharpSettingsName", "USharp"),
				LOCTEXT("USharpSettingsDescription", "Configure USharp"),
				GetMutableDefault<USharpSettings>());
		}
	}
	
	void UnregisterSettings()
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "USharp");
		}
	}

	virtual void StartupModule() override
	{
		RegisterSettings();

		USharpSettings* Settings = GetMutableDefault<USharpSettings>();
		if (Settings == nullptr/* || !Settings->Validate()*/)
		{
			UE_LOG(LogTemp, Log, TEXT("USharp failed to validate settings"));
			return;
		}
		
		if (!Settings->bEnabled)
		{
#if WITH_EDITOR
			// If the project has a C# sln file, prompt to enable USharp.
			if (FPaths::IsProjectFilePathSet())
			{
				FString ProjectSln = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Managed"), FString(FApp::GetProjectName()) + TEXT(".Managed.sln"));
				if (FPaths::FileExists(ProjectSln))
				{
					FText Title = FText::FromString(TEXT("USharp"));
					FText Msg = FText::FromString(TEXT("USharp isn't enabled but the project contains C# code. Enable USharp?"));
					if (FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title) == EAppReturnType::Yes)
					{
						Settings->bEnabled = true;
						//Settings->SaveConfig();

						// Calling Setting->SaveConfig() SHOULD work but if there is no existing value in a .ini
						// file then it will save into /Saved/Config/Windows/Engine.ini and any following changes
						// to the value in the editor will also be saved there. The problem with this is that the
						// value in that .ini file doesn't get included in the packaged project (only the value in
						// DefaultEngine.ini gets included in the packaged project)
						//
						// NOTE: Subsequent calls to SaveConfig DO seem to work even when there is no config value 
						//       in any file (see USharpSettings::PostEditChangeProperty where SaveConfig() is called)
						if (SettingsSection)
						{
							SettingsSection->Save();
						}
					}
				}
			}
			if (!Settings->bEnabled)
			{
				return;
			}
#else
			return;
#endif
		}
	
		FString AssemblyPath = TEXT("UnrealEngine.Runtime.dll");
		FString LoaderPath = TEXT("Loader.dll");
		FString Args = TEXT("");
		bool bLoaderEnabled = true;
		
		// If USharpClass isn't referenced it doesn't get included? TODO: Look into
		if (USharpClass::StaticClass() == nullptr)
		{
			FText Title = FText::FromString(TEXT("Error"));
			FText Msg = FText::FromString(TEXT("Failed to find USharpClass!"));
			FMessageDialog::Open(EAppMsgType::Ok, Msg, &Title);
			return;
		}
		
		if (!CSharpLoader::GetInstance()->Load(AssemblyPath, Args, LoaderPath, bLoaderEnabled))
		{
			UE_LOG(LogTemp, Log, TEXT("USharp failed to load"));
		}
	}

	virtual void ShutdownModule() override
	{
		UnregisterSettings();
	}
};

IMPLEMENT_MODULE( FUSharpModule, USharp )

#undef LOCTEXT_NAMESPACE