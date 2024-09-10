// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorPackageUtils.h"
#include "UnrealEd.h"  // GUnrealEd ����� ���� �ʿ�
#include <Misc/HotReloadInterface.h>
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

/**
 * ���� ��ο��� ������ �����ϴ� �Լ�.
 * �־��� ���� ��ο��� "Source" ���丮 ������ ù ��° �������� �������� �����ϰ� �����մϴ�.
 *
 * @param FilePath ������ ������ ���� ���.
 * @return ����� ������ ��ȯ. ���� "Source" ��ΰ� ���Ե��� ���� �߸��� ��ζ�� �� ���ڿ��� ��ȯ.
 */
FString EditorPackageUtils::ExtractModuleNameFromPath(const FString& FilePath)
{
    // ��ο��� "Source" ������ ��� ����
    FString PathAfterSource;
    if (FilePath.Split(TEXT("/Source/"), nullptr, &PathAfterSource))
    {
        // "Source/" ������ ù ��° �������� �������� ����
        FString ModuleName = PathAfterSource.Left(PathAfterSource.Find(TEXT("/")));

        // ���� ��ȯ
        return ModuleName;
    }

    // ��ΰ� �ùٸ��� ���� ��� �� ���ڿ� ��ȯ
    return FString();
}

/**
 * ��ο� .uasset Ȯ���ڰ� �پ� �ִ��� Ȯ���ϰ�, ������ �ٿ��ִ� �Լ�.
 *
 * @param FilePath Ȯ���� ���� ���.
 * @return .uasset Ȯ���ڰ� �پ��ִ� �ùٸ� ���� ���.
 */
FString EditorPackageUtils::EnsureUAssetExtension(const FString& FilePath)
{
    // -- ".uasset" Ȯ���ڰ� �ִ��� Ȯ��
    if (!FilePath.EndsWith(TEXT(".uasset")))
    {
        // -- ������ ".uasset" Ȯ���ڸ� �߰�
        return FilePath + TEXT(".uasset");
    }

    // -- �̹� Ȯ���ڰ� ������ �״�� ��ȯ
    return FilePath;
}

/**
 * ���� �ý��� ��θ� Unreal Engine�� ��Ű�� ��η� ��ȯ�ϴ� �Լ�.
 * ������Ʈ�� ������ ���͸� �Ǵ� �÷������� ������ ���͸����� �־��� ���� �ý��� ��θ�
 * Unreal Engine�� ����ϴ� ��Ű�� ��η� ��ȯ�մϴ�.
 * ���� ���, "/Content/MyAsset/MyFile" ���� �ý��� ��δ� "/Game/MyAsset/MyFile" ��Ű�� ��η�,
 * �÷������� ������ ��δ� "/PluginName/Content/MyAsset"���� ��ȯ�˴ϴ�.
 *
 * @param FilePath ���� ���� �ý��� ���. ��: "C:/Unreal Projects/YourProject/Content/MyAsset/MyFile"
 * @return Unreal Engine ��Ű�� ���. ��: "/Game/MyAsset/MyFile" �Ǵ� "/PluginName/Content/MyAsset/MyFile"
 */
FString EditorPackageUtils::ConvertFilePathToPackagePath(const FString& FilePath)
{
    // -- ������Ʈ�� ������ ���͸� ��θ� ���� ��η� ��ȯ
    FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    UE_LOG(LogTemp, Log, TEXT("Absolute Project Content Dir: %s"), *ProjectContentDir);

    // -- ������Ʈ ������ ���� ���� �ִ��� Ȯ��
    if (FilePath.StartsWith(ProjectContentDir))
    {
        // ������ ���� ���� ���� ��� ��� ��ȯ
        FString RelativePath = FilePath.RightChop(ProjectContentDir.Len());

        // -- ������ �ߺ� ����
        RelativePath.RemoveFromStart(TEXT("/"));

        // -- Unreal ��η� ��ȯ
        FString PackagePath = FString::Printf(TEXT("/Game/%s"), *RelativePath);
        UE_LOG(LogTemp, Log, TEXT("Converted Package Path: %s"), *PackagePath);
        return PackagePath;
    }

    // -- �÷������� ������ ���͸����� Ȯ��
    const TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetEnabledPlugins();
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FString PluginContentDir = FPaths::ConvertRelativePathToFull(Plugin->GetContentDir());
        if (FilePath.StartsWith(PluginContentDir))
        {
            // �÷������� ������ ���� ���� ���� ��� ��� ��ȯ
            FString RelativePath = FilePath.RightChop(PluginContentDir.Len());

            // -- ������ �ߺ� ����
            RelativePath.RemoveFromStart(TEXT("/"));

            // �÷����� ��η� ��ȯ
            FString PackagePath = FString::Printf(TEXT("/%s/%s"), *Plugin->GetName(), *RelativePath);
            UE_LOG(LogTemp, Log, TEXT("Converted Plugin Package Path: %s"), *PackagePath);
            return PackagePath;
        }
    }

    // ��ΰ� �νĵ��� ������ ���� �α� ��� �� �� ���ڿ� ��ȯ
    UE_LOG(LogTemp, Error, TEXT("FilePath is not inside any recognized content or plugin directory: %s"), *FilePath);
    return FString();
}

/**
 /**
 * �÷������� �𸮾� ��Ű�� ��� (��: "/Game/Plugins/PluginName/...")�� ���� ���� �ý��� ��η� ��ȯ�ϴ� �Լ�.
 * ��Ű�� ��ΰ� �÷����� ������ ������ �ִ� ���, ������Ʈ�� �÷����� ���͸��� �������� Ǯ ��θ� ����մϴ�.
 * �� �ܿ��� �⺻������ FPackageName::LongPackageNameToFilename �Լ��� ����Ͽ� ������Ʈ ������ ���͸��� �������� ��θ� ��ȯ�մϴ�.
 *
 * @param FullPackagePath �𸮾� ��Ű�� ��� (��: "/Game/Plugins/PluginName/...").
 * @return Unreal ��Ű�� ��ο� �ش��ϴ� ���� ���� �ý��� ��� (��: "C:/Unreal Projects/YourProject/Plugins/PluginName/Content/.../Asset.uasset").
 */
FString EditorPackageUtils::PluginLongPackageNameToFilename(const FString& FullPackagePath)
{
    // "Plugins/"�� �������� ���� ��� ��θ� ����
    int32 PluginsIndex = FullPackagePath.Find(TEXT("Plugins/"));
    if (PluginsIndex != INDEX_NONE)
    {
        // "Plugins/" ������ ��θ� ����
        FString PluginRelativePath = FullPackagePath.RightChop(PluginsIndex + 8); // "Plugins/" ����

        // �÷������� �̸��� ���� ("/PluginName/..."�� ���¿��� ù ��° �κ� ����)
        int32 SlashIndex;
        if (PluginRelativePath.FindChar(TEXT('/'), SlashIndex))
        {
            FString PluginName = PluginRelativePath.Left(SlashIndex);

            // PluginName�� Content�� ��ģ ��� ����
            FString PluginContentDir = FPaths::Combine(FPaths::ProjectPluginsDir(), PluginName, TEXT("Content"));

            // PluginRelativePath���� PluginName �����ϰ� ���� ��� ���
            FString RemainingPath = PluginRelativePath.RightChop(SlashIndex + 1);

            // ���� ��� ���� (Plugins/PluginName/Content/RemainingPath)
            return FPaths::Combine(PluginContentDir, RemainingPath + TEXT(".uasset"));
        }
        else
        {
            // �÷����� �̸� �ڿ� ��ΰ� ���� ��� ���� ó��
            UE_LOG(LogTemp, Error, TEXT("Invalid plugin path: %s"), *FullPackagePath);
            return FString();
        }
    }
    else
    {
        // �÷����� ������ ������ ������ �⺻���� Unreal ������� ��θ� ��ȯ
        return FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
    }
}

/**
 * �־��� ���� ��θ� �������� ������ �̹� Asset Registry�� ��ϵǾ� �ִ��� Ȯ���ϴ� �Լ�.
 *
 * @param AssetPath Ȯ���� ������ ��� (��: "/Game/MyFolder/MyAsset").
 * @return ������ �̹� ��ϵǾ� ������ true, �׷��� ������ false�� ��ȯ.
 */
bool EditorPackageUtils::IsAssetAlreadyRegistered(const FString& AssetPath)
{
    // AssetRegistry ����� ������
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // AssetRegistry�� ���� ���� ������ ������
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*AssetPath);

    // ���� �����Ͱ� ��ȿ�ϸ� �̹� ��ϵ� ������
    return AssetData.IsValid();
}

/**
 * �������� ����� ����ü���� ������� ����ü ����(Ÿ�� ����)�� �ε��մϴ�.
 * �� �Լ��� ����ü�� �ν��Ͻ��� �ƴ�, �ش� ����ü�� ���Ǹ� �ε��մϴ�.
 *
 * @param ModuleName �ε��� ����ü�� ���� ����� �̸�.
 * @param StructName �ε��� ����ü�� �̸�.
 * @return UScriptStruct* ����ü�� ���ǰ� ���������� �ε�Ǹ� �ش� ����ü�� �����͸� ��ȯ, �����ϸ� nullptr�� ��ȯ.
 */
UScriptStruct* EditorPackageUtils::LoadStructDefinitionByName(const FString& ModuleName, const FString& StructName)
{
    // RowStructPath ���� (����ü ���Ǹ� �������� ã�� ���� ���)
    FString RowStructPath = FString::Printf(TEXT("/Script/%s.%s"), *ModuleName, *StructName);

    // UScriptStruct ���� �ε�
    UScriptStruct* RowStruct = LoadObject<UScriptStruct>(nullptr, *RowStructPath);

    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load struct definition: %s from module: %s"), *StructName, *ModuleName);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully loaded struct definition: %s from module: %s"), *StructName, *ModuleName);
    }

    return RowStruct;
}

/**
 * �������� ����� Ŭ�������� ������� Ŭ���� ����(Ÿ�� ����)�� �ε��մϴ�.
 * �� �Լ��� Ŭ���� �ν��Ͻ��� �ƴ�, �ش� Ŭ������ ���Ǹ� �ε��մϴ�.
 *
 * @param ModuleName �ε��� Ŭ������ ���� ����� �̸�.
 * @param ClassName �ε��� Ŭ������ �̸�.
 * @return UClass* Ŭ���� ���ǰ� ���������� �ε�Ǹ� �ش� Ŭ������ �����͸� ��ȯ, �����ϸ� nullptr�� ��ȯ.
 */
UClass* EditorPackageUtils::LoadClassDefinitionByName(const FString& ModuleName, const FString& ClassName)
{
    // ClassPath ���� (Ŭ���� ���Ǹ� �������� ã�� ���� ���)
    FString ClassPath = FString::Printf(TEXT("/Script/%s.%s"), *ModuleName, *ClassName);

    // UClass ���� �ε�
    UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);

    if (!LoadedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load class definition: %s from module: %s"), *ClassName, *ModuleName);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully loaded class definition: %s from module: %s"), *ClassName, *ModuleName);
    }

    return LoadedClass;
}

/**
 * ���带 �����ϰ� �� ���ε带 ó���ϴ� �Լ�.
 * ���� �߿��� ���¸� �˸��� ���� ��Ƽ�����̼��� ǥ���ϸ�, ���� �Ϸ� �� �� ���ε带 �����մϴ�.
 */
void EditorPackageUtils::ExecuteBuildAndHotReload()
{
    // ���� ���� ���¸� �˸��� ���� ��Ƽ�����̼� ����
    FNotificationInfo Info(FText::FromString(TEXT("Build in progress...")));
    Info.bFireAndForget = false;  // �ڵ����� ������� �ʵ��� ����
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 0.0f;
    Info.bUseThrobber = true;  // ��� ���� ������ ���
    Info.bUseSuccessFailIcons = true;

    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);  // ���� �� ���·� ����
    }

    // ���� ����
    GUnrealEd->Exec(NULL, TEXT("Build"));

    // ���� �Ϸ� �� ���� ������Ʈ
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetText(FText::FromString(TEXT("Build completed successfully!")));
        NotificationItem->SetCompletionState(SNotificationItem::CS_Success);  // ���� ���·� ����
        NotificationItem->ExpireAndFadeout();  // �Ϸ� �� �˸��� ������ ������� ��
    }

    // �� ���ε� Ʈ����
    IHotReloadInterface& HotReload = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
    HotReload.DoHotReloadFromEditor(EHotReloadFlags::None);
}

/**
 * �񵿱������� ���带 �����ϰ�, ���� �Ϸ� �� �����͸� �ٽ� �����ϴ� �Լ�.
 * ���� ���� �߿��� ��Ƽ�����̼��� ���� ���¸� ǥ���ϰ�, ���尡 ���������� �Ϸ�Ǹ� �����͸� �ٽ� �����մϴ�.
 * ���� �ÿ��� ���� �޽����� ǥ���մϴ�.
 *
 * @note ���� ���μ����� �񵿱������� ����Ǹ�, �Ϸ�Ǹ� �����͸� ������մϴ�.
 */
void EditorPackageUtils::StartBuildAndRestartEditor()
{
    // ���� ���� ���¸� �˸��� ���� ��Ƽ�����̼� ����
    FNotificationInfo Info(FText::FromString(TEXT("Build in progress...")));
    Info.bFireAndForget = false;  // �ڵ����� ������� �ʵ��� ����
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 0.0f;
    Info.bUseThrobber = true;  // ��� ���� ������ ���
    Info.bUseSuccessFailIcons = true;

    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);  // ���� ���� �� ���·� ����
    }

    // �񵿱� �۾����� ���� ���μ����� ����
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
        {
            // UnrealBuildTool.exe ���� ��� ���� (�������� ���� ��ġ ��� ��������)
            FString UBTPath = FPaths::Combine(
                FPaths::EngineDir(),  // ���� ���� ��ġ ��θ� �ڵ����� ������
                TEXT("Binaries"),
                TEXT("DotNET"),
                TEXT("UnrealBuildTool"),
                TEXT("UnrealBuildTool.exe")  // ��Ȯ�� ���� ��� (Ȥ�� OS �� ���� ������ ���� �ٸ��� �߰� ó�� �ʿ�)
            );
            UE_LOG(LogTemp, Log, TEXT("UBTPath: %s"), *UBTPath);  // ����� �α� ���

            // ������ �ش� ��ο� ������ �ִ��� Ȯ��
            if (!FPaths::FileExists(UBTPath))
            {
                UE_LOG(LogTemp, Error, TEXT("UnrealBuildTool.exe file does not exist at: %s"), *UBTPath);
                return;
            }

            // ������Ʈ ���
            FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
            UE_LOG(LogTemp, Log, TEXT("ProjectPath: %s"), *ProjectPath);  // ����� �α� ���

            // ���� ��ɾ� ���� ���� (������Ʈ ��� ����)
            FString Arguments = FString::Printf(TEXT("\"%s\" -projectfiles"), *ProjectPath);
            UE_LOG(LogTemp, Log, TEXT("Arguments: %s"), *Arguments);  // ����� �α� ���

            // ���� ���μ��� ����
            FProcHandle BuildProcess = FPlatformProcess::CreateProc(*UBTPath, *Arguments, true, false, false, nullptr, 0, nullptr, nullptr);

            if (!BuildProcess.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to start build process."));
                AsyncTask(ENamedThreads::GameThread, [NotificationItem]()
                    {
                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build failed to start!")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
                            NotificationItem->ExpireAndFadeout();
                        }
                    });
                return;
            }

            // ���� ����� ��� �� ����
            while (FPlatformProcess::IsProcRunning(BuildProcess))
            {
                FPlatformProcess::Sleep(1);  // ���� ���� �� ���
            }

            int32 ReturnCode;
            FPlatformProcess::GetProcReturnCode(BuildProcess, &ReturnCode);

            // ���� �����忡�� �ļ� �۾� ����
            AsyncTask(ENamedThreads::GameThread, [=]()
                {
                    if (ReturnCode == 0)
                    {
                        // ���� ����
                        UE_LOG(LogTemp, Log, TEXT("Build completed successfully!"));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build completed successfully!")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Success);  // ���� ���·� ������Ʈ
                            NotificationItem->ExpireAndFadeout();
                        }

                        // ������ �����
                        RestartEditorWithProject(ProjectPath);  // ���� ������Ʈ�� �����
                    }
                    else
                    {
                        // ���� ����
                        UE_LOG(LogTemp, Error, TEXT("Build failed. Check the logs for more details."));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build failed!")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);  // ���� ���·� ������Ʈ
                            NotificationItem->ExpireAndFadeout();
                        }
                    }
                });
        });
}

/**
 * �����͸� �����ϰ�, ������Ʈ�� �ٽ� �����ϴ� �Լ�.
 * ���� �����͸� ������ ��, �ش� ������Ʈ ������ �ٽ� �����մϴ�.
 *
 * @param ProjectPath �ٽ� ������ ������Ʈ�� ���� ���.
 */
void EditorPackageUtils::RestartEditorWithProject(const FString& ProjectPath)
{
    // ���� ���� ���� �������� ���
    FString EditorPath = FPlatformProcess::ExecutablePath();

    // ���ڷ� ���� ������Ʈ�� ��θ� ����
    FString Arguments = FString::Printf(TEXT("\"%s\""), *ProjectPath);

    // ������ ����� (���� ������Ʈ��)
    FPlatformProcess::CreateProc(*EditorPath, *Arguments, true, false, false, nullptr, 0, nullptr, nullptr);

    // ������ ���� (������ ���� �� ���ο� ���μ����� �����)
    FPlatformMisc::RequestExit(false);
}

/**
 * SaveObject�� �־��� ���͸��� ���� �̸��� �°� Unreal Engine ��Ű���� �����ϴ� �Լ�.
 * SaveObject�� �̹� �����ϴ� ��Ű���� ������ ������ ���ο� ��Ű���� �����ϰ�,
 * ���� �� ��Ű���� �����մϴ�. ��Ű�� ��δ� Unreal Engine���� ���Ǵ�
 * "/Game" �Ǵ� "/PluginName" ������ ��Ű�� ��η� ��ȯ�˴ϴ�.
 *
 * @param SaveObject ������ UObject.
 * @param SaveDirectory ���� �ý��� ���� ������ ���͸� ��� (��: "C:/Unreal Projects/YourProject/Content/...").
 * @param FileName ������ ���� �̸� (Ȯ���ڴ� �ʿ����� ����).
 */
UPackage* EditorPackageUtils::SaveAssetToPackage(UObject* const SaveObject, const FString& SaveDirectory, const FString& FileName, EObjectFlags TopLevelFlags)
{
    if (!SaveObject)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveObject is null!"));
        return nullptr;
    }

    // -- ������Ʈ�� ������ ���͸� ��θ� ���� ��η� ��ȯ
    // SaveDirectory ���� PackagePath �� ��� ��ȯ
    FString RelativePath = EditorPackageUtils::ConvertFilePathToPackagePath(SaveDirectory);
    UE_LOG(LogTemp, Log, TEXT("Convert RelativePath: %s"), *RelativePath);
    if (RelativePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SaveDirectory is not inside any recognized content directory: %s (%s)"), *SaveDirectory, *RelativePath);
        return nullptr;
    }

    // -- ��Ű�� ��ο� SaveObject �̸��� �߰��Ͽ� ���� ��Ű�� ��θ� ����
    FString FullPackagePath = FPaths::Combine(RelativePath, FileName);
    UE_LOG(LogTemp, Log, TEXT("Full Package Path: %s"), *FullPackagePath);

    // -- ��Ű�� ����
    UPackage* ExistingPackage = FindPackage(nullptr, *FullPackagePath);
    if (!ExistingPackage)
    {
        // -- ��Ű�� ����
        ExistingPackage = CreatePackage(*FullPackagePath);
        if (!ExistingPackage)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPackagePath);
            return nullptr;
        }
        UE_LOG(LogTemp, Log, TEXT("Package created: %s"), *FullPackagePath);
    }
    SaveObject->Rename(*FileName, ExistingPackage);

    // -- Asset ���
    if (EditorPackageUtils::IsAssetAlreadyRegistered(FullPackagePath) == false)
    {
        FAssetRegistryModule::AssetCreated(SaveObject);
    }

    // -- ��Ű�� ����
    SaveObject->MarkPackageDirty();

    if (!IFileManager::Get().DirectoryExists(*SaveDirectory))
    {
        IFileManager::Get().MakeDirectory(*SaveDirectory, true);
        UE_LOG(LogTemp, Log, TEXT("Created directory: %s"), *SaveDirectory);
    }

    FString FilePath = FPaths::Combine(SaveDirectory, FileName);
    FilePath = EditorPackageUtils::EnsureUAssetExtension(FilePath);

    // -- ��Ű�� ���� ó��
    bool bSaved = UPackage::SavePackage(ExistingPackage, SaveObject, TopLevelFlags, *FilePath);
    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save package: %s"), *FilePath);
        return nullptr;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved package: %s"), *FilePath);
    }

    return ExistingPackage;
}
