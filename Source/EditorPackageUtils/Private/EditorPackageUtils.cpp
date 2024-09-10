// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorPackageUtils.h"
#include "UnrealEd.h"  // GUnrealEd 사용을 위해 필요
#include <Misc/HotReloadInterface.h>
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

/**
 * 파일 경로에서 모듈명을 추출하는 함수.
 * 주어진 파일 경로에서 "Source" 디렉토리 이후의 첫 번째 폴더명을 모듈명으로 간주하고 추출합니다.
 *
 * @param FilePath 모듈명을 추출할 파일 경로.
 * @return 추출된 모듈명을 반환. 만약 "Source" 경로가 포함되지 않은 잘못된 경로라면 빈 문자열을 반환.
 */
FString EditorPackageUtils::ExtractModuleNameFromPath(const FString& FilePath)
{
    // 경로에서 "Source" 이후의 경로 추출
    FString PathAfterSource;
    if (FilePath.Split(TEXT("/Source/"), nullptr, &PathAfterSource))
    {
        // "Source/" 이후의 첫 번째 폴더명을 모듈명으로 간주
        FString ModuleName = PathAfterSource.Left(PathAfterSource.Find(TEXT("/")));

        // 모듈명 반환
        return ModuleName;
    }

    // 경로가 올바르지 않을 경우 빈 문자열 반환
    return FString();
}

/**
 * 경로에 .uasset 확장자가 붙어 있는지 확인하고, 없으면 붙여주는 함수.
 *
 * @param FilePath 확인할 파일 경로.
 * @return .uasset 확장자가 붙어있는 올바른 파일 경로.
 */
FString EditorPackageUtils::EnsureUAssetExtension(const FString& FilePath)
{
    // -- ".uasset" 확장자가 있는지 확인
    if (!FilePath.EndsWith(TEXT(".uasset")))
    {
        // -- 없으면 ".uasset" 확장자를 추가
        return FilePath + TEXT(".uasset");
    }

    // -- 이미 확장자가 있으면 그대로 반환
    return FilePath;
}

/**
 * 파일 시스템 경로를 Unreal Engine의 패키지 경로로 변환하는 함수.
 * 프로젝트의 콘텐츠 디렉터리 또는 플러그인의 콘텐츠 디렉터리에서 주어진 파일 시스템 경로를
 * Unreal Engine이 사용하는 패키지 경로로 변환합니다.
 * 예를 들어, "/Content/MyAsset/MyFile" 파일 시스템 경로는 "/Game/MyAsset/MyFile" 패키지 경로로,
 * 플러그인의 콘텐츠 경로는 "/PluginName/Content/MyAsset"으로 변환됩니다.
 *
 * @param FilePath 실제 파일 시스템 경로. 예: "C:/Unreal Projects/YourProject/Content/MyAsset/MyFile"
 * @return Unreal Engine 패키지 경로. 예: "/Game/MyAsset/MyFile" 또는 "/PluginName/Content/MyAsset/MyFile"
 */
FString EditorPackageUtils::ConvertFilePathToPackagePath(const FString& FilePath)
{
    // -- 프로젝트의 콘텐츠 디렉터리 경로를 절대 경로로 변환
    FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    UE_LOG(LogTemp, Log, TEXT("Absolute Project Content Dir: %s"), *ProjectContentDir);

    // -- 프로젝트 콘텐츠 폴더 내에 있는지 확인
    if (FilePath.StartsWith(ProjectContentDir))
    {
        // 콘텐츠 폴더 내에 있을 경우 경로 변환
        FString RelativePath = FilePath.RightChop(ProjectContentDir.Len());

        // -- 슬래시 중복 방지
        RelativePath.RemoveFromStart(TEXT("/"));

        // -- Unreal 경로로 변환
        FString PackagePath = FString::Printf(TEXT("/Game/%s"), *RelativePath);
        UE_LOG(LogTemp, Log, TEXT("Converted Package Path: %s"), *PackagePath);
        return PackagePath;
    }

    // -- 플러그인의 콘텐츠 디렉터리에서 확인
    const TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetEnabledPlugins();
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FString PluginContentDir = FPaths::ConvertRelativePathToFull(Plugin->GetContentDir());
        if (FilePath.StartsWith(PluginContentDir))
        {
            // 플러그인의 콘텐츠 폴더 내에 있을 경우 경로 변환
            FString RelativePath = FilePath.RightChop(PluginContentDir.Len());

            // -- 슬래시 중복 방지
            RelativePath.RemoveFromStart(TEXT("/"));

            // 플러그인 경로로 변환
            FString PackagePath = FString::Printf(TEXT("/%s/%s"), *Plugin->GetName(), *RelativePath);
            UE_LOG(LogTemp, Log, TEXT("Converted Plugin Package Path: %s"), *PackagePath);
            return PackagePath;
        }
    }

    // 경로가 인식되지 않으면 에러 로그 출력 및 빈 문자열 반환
    UE_LOG(LogTemp, Error, TEXT("FilePath is not inside any recognized content or plugin directory: %s"), *FilePath);
    return FString();
}

/**
 /**
 * 플러그인의 언리얼 패키지 경로 (예: "/Game/Plugins/PluginName/...")를 실제 파일 시스템 경로로 변환하는 함수.
 * 패키지 경로가 플러그인 콘텐츠 폴더에 있는 경우, 프로젝트의 플러그인 디렉터리를 기준으로 풀 경로를 계산합니다.
 * 그 외에는 기본적으로 FPackageName::LongPackageNameToFilename 함수를 사용하여 프로젝트 콘텐츠 디렉터리를 기준으로 경로를 변환합니다.
 *
 * @param FullPackagePath 언리얼 패키지 경로 (예: "/Game/Plugins/PluginName/...").
 * @return Unreal 패키지 경로에 해당하는 실제 파일 시스템 경로 (예: "C:/Unreal Projects/YourProject/Plugins/PluginName/Content/.../Asset.uasset").
 */
FString EditorPackageUtils::PluginLongPackageNameToFilename(const FString& FullPackagePath)
{
    // "Plugins/"를 기준으로 앞의 모든 경로를 제거
    int32 PluginsIndex = FullPackagePath.Find(TEXT("Plugins/"));
    if (PluginsIndex != INDEX_NONE)
    {
        // "Plugins/" 이후의 경로를 추출
        FString PluginRelativePath = FullPackagePath.RightChop(PluginsIndex + 8); // "Plugins/" 제거

        // 플러그인의 이름을 추출 ("/PluginName/..."의 형태에서 첫 번째 부분 추출)
        int32 SlashIndex;
        if (PluginRelativePath.FindChar(TEXT('/'), SlashIndex))
        {
            FString PluginName = PluginRelativePath.Left(SlashIndex);

            // PluginName과 Content를 합친 경로 생성
            FString PluginContentDir = FPaths::Combine(FPaths::ProjectPluginsDir(), PluginName, TEXT("Content"));

            // PluginRelativePath에서 PluginName 제거하고 남은 경로 얻기
            FString RemainingPath = PluginRelativePath.RightChop(SlashIndex + 1);

            // 최종 경로 생성 (Plugins/PluginName/Content/RemainingPath)
            return FPaths::Combine(PluginContentDir, RemainingPath + TEXT(".uasset"));
        }
        else
        {
            // 플러그인 이름 뒤에 경로가 없을 경우 에러 처리
            UE_LOG(LogTemp, Error, TEXT("Invalid plugin path: %s"), *FullPackagePath);
            return FString();
        }
    }
    else
    {
        // 플러그인 폴더에 속하지 않으면 기본적인 Unreal 방식으로 경로를 변환
        return FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
    }
}

/**
 * 주어진 에셋 경로를 기준으로 에셋이 이미 Asset Registry에 등록되어 있는지 확인하는 함수.
 *
 * @param AssetPath 확인할 에셋의 경로 (예: "/Game/MyFolder/MyAsset").
 * @return 에셋이 이미 등록되어 있으면 true, 그렇지 않으면 false를 반환.
 */
bool EditorPackageUtils::IsAssetAlreadyRegistered(const FString& AssetPath)
{
    // AssetRegistry 모듈을 가져옴
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // AssetRegistry를 통해 에셋 정보를 가져옴
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*AssetPath);

    // 에셋 데이터가 유효하면 이미 등록된 에셋임
    return AssetData.IsValid();
}

/**
 * 동적으로 모듈명과 구조체명을 기반으로 구조체 정의(타입 정보)를 로드합니다.
 * 이 함수는 구조체의 인스턴스가 아닌, 해당 구조체의 정의를 로드합니다.
 *
 * @param ModuleName 로드할 구조체가 속한 모듈의 이름.
 * @param StructName 로드할 구조체의 이름.
 * @return UScriptStruct* 구조체의 정의가 성공적으로 로드되면 해당 구조체의 포인터를 반환, 실패하면 nullptr을 반환.
 */
UScriptStruct* EditorPackageUtils::LoadStructDefinitionByName(const FString& ModuleName, const FString& StructName)
{
    // RowStructPath 설정 (구조체 정의를 동적으로 찾기 위한 경로)
    FString RowStructPath = FString::Printf(TEXT("/Script/%s.%s"), *ModuleName, *StructName);

    // UScriptStruct 정의 로드
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
 * 동적으로 모듈명과 클래스명을 기반으로 클래스 정의(타입 정보)를 로드합니다.
 * 이 함수는 클래스 인스턴스가 아닌, 해당 클래스의 정의를 로드합니다.
 *
 * @param ModuleName 로드할 클래스가 속한 모듈의 이름.
 * @param ClassName 로드할 클래스의 이름.
 * @return UClass* 클래스 정의가 성공적으로 로드되면 해당 클래스의 포인터를 반환, 실패하면 nullptr을 반환.
 */
UClass* EditorPackageUtils::LoadClassDefinitionByName(const FString& ModuleName, const FString& ClassName)
{
    // ClassPath 설정 (클래스 정의를 동적으로 찾기 위한 경로)
    FString ClassPath = FString::Printf(TEXT("/Script/%s.%s"), *ModuleName, *ClassName);

    // UClass 정의 로드
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
 * 빌드를 실행하고 핫 리로드를 처리하는 함수.
 * 빌드 중에는 상태를 알리기 위한 노티피케이션을 표시하며, 빌드 완료 후 핫 리로드를 실행합니다.
 */
void EditorPackageUtils::ExecuteBuildAndHotReload()
{
    // 빌드 진행 상태를 알리기 위한 노티피케이션 생성
    FNotificationInfo Info(FText::FromString(TEXT("Build in progress...")));
    Info.bFireAndForget = false;  // 자동으로 사라지지 않도록 설정
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 0.0f;
    Info.bUseThrobber = true;  // 계속 도는 아이콘 사용
    Info.bUseSuccessFailIcons = true;

    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);  // 진행 중 상태로 설정
    }

    // 빌드 실행
    GUnrealEd->Exec(NULL, TEXT("Build"));

    // 빌드 완료 후 상태 업데이트
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetText(FText::FromString(TEXT("Build completed successfully!")));
        NotificationItem->SetCompletionState(SNotificationItem::CS_Success);  // 성공 상태로 설정
        NotificationItem->ExpireAndFadeout();  // 완료 후 알림을 서서히 사라지게 함
    }

    // 핫 리로드 트리거
    IHotReloadInterface& HotReload = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
    HotReload.DoHotReloadFromEditor(EHotReloadFlags::None);
}

/**
 * 비동기적으로 빌드를 실행하고, 빌드 완료 후 에디터를 다시 시작하는 함수.
 * 빌드 진행 중에는 노티피케이션을 통해 상태를 표시하고, 빌드가 성공적으로 완료되면 에디터를 다시 시작합니다.
 * 실패 시에는 실패 메시지를 표시합니다.
 *
 * @note 빌드 프로세스는 비동기적으로 실행되며, 완료되면 에디터를 재시작합니다.
 */
void EditorPackageUtils::StartBuildAndRestartEditor()
{
    // 빌드 진행 상태를 알리기 위한 노티피케이션 생성
    FNotificationInfo Info(FText::FromString(TEXT("Build in progress...")));
    Info.bFireAndForget = false;  // 자동으로 사라지지 않도록 설정
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 0.0f;
    Info.bUseThrobber = true;  // 계속 도는 아이콘 사용
    Info.bUseSuccessFailIcons = true;

    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);  // 빌드 진행 중 상태로 설정
    }

    // 비동기 작업으로 빌드 프로세스를 실행
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
        {
            // UnrealBuildTool.exe 절대 경로 설정 (동적으로 엔진 설치 경로 가져오기)
            FString UBTPath = FPaths::Combine(
                FPaths::EngineDir(),  // 현재 엔진 설치 경로를 자동으로 가져옴
                TEXT("Binaries"),
                TEXT("DotNET"),
                TEXT("UnrealBuildTool"),
                TEXT("UnrealBuildTool.exe")  // 정확한 파일 경로 (혹시 OS 나 엔진 버전에 따라 다르면 추가 처리 필요)
            );
            UE_LOG(LogTemp, Log, TEXT("UBTPath: %s"), *UBTPath);  // 디버깅 로그 출력

            // 실제로 해당 경로에 파일이 있는지 확인
            if (!FPaths::FileExists(UBTPath))
            {
                UE_LOG(LogTemp, Error, TEXT("UnrealBuildTool.exe file does not exist at: %s"), *UBTPath);
                return;
            }

            // 프로젝트 경로
            FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
            UE_LOG(LogTemp, Log, TEXT("ProjectPath: %s"), *ProjectPath);  // 디버깅 로그 출력

            // 빌드 명령어 인자 구성 (프로젝트 경로 포함)
            FString Arguments = FString::Printf(TEXT("\"%s\" -projectfiles"), *ProjectPath);
            UE_LOG(LogTemp, Log, TEXT("Arguments: %s"), *Arguments);  // 디버깅 로그 출력

            // 빌드 프로세스 실행
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

            // 빌드 결과를 대기 및 감시
            while (FPlatformProcess::IsProcRunning(BuildProcess))
            {
                FPlatformProcess::Sleep(1);  // 빌드 중일 때 대기
            }

            int32 ReturnCode;
            FPlatformProcess::GetProcReturnCode(BuildProcess, &ReturnCode);

            // 메인 스레드에서 후속 작업 실행
            AsyncTask(ENamedThreads::GameThread, [=]()
                {
                    if (ReturnCode == 0)
                    {
                        // 빌드 성공
                        UE_LOG(LogTemp, Log, TEXT("Build completed successfully!"));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build completed successfully!")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Success);  // 성공 상태로 업데이트
                            NotificationItem->ExpireAndFadeout();
                        }

                        // 에디터 재시작
                        RestartEditorWithProject(ProjectPath);  // 현재 프로젝트로 재시작
                    }
                    else
                    {
                        // 빌드 실패
                        UE_LOG(LogTemp, Error, TEXT("Build failed. Check the logs for more details."));

                        if (NotificationItem.IsValid())
                        {
                            NotificationItem->SetText(FText::FromString(TEXT("Build failed!")));
                            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);  // 실패 상태로 업데이트
                            NotificationItem->ExpireAndFadeout();
                        }
                    }
                });
        });
}

/**
 * 에디터를 종료하고, 프로젝트를 다시 실행하는 함수.
 * 현재 에디터를 종료한 후, 해당 프로젝트 파일을 다시 실행합니다.
 *
 * @param ProjectPath 다시 실행할 프로젝트의 파일 경로.
 */
void EditorPackageUtils::RestartEditorWithProject(const FString& ProjectPath)
{
    // 현재 실행 중인 에디터의 경로
    FString EditorPath = FPlatformProcess::ExecutablePath();

    // 인자로 현재 프로젝트의 경로를 전달
    FString Arguments = FString::Printf(TEXT("\"%s\""), *ProjectPath);

    // 에디터 재시작 (현재 프로젝트로)
    FPlatformProcess::CreateProc(*EditorPath, *Arguments, true, false, false, nullptr, 0, nullptr, nullptr);

    // 에디터 종료 (에디터 종료 후 새로운 프로세스가 실행됨)
    FPlatformMisc::RequestExit(false);
}

/**
 * SaveObject를 주어진 디렉터리와 파일 이름에 맞게 Unreal Engine 패키지로 저장하는 함수.
 * SaveObject가 이미 존재하는 패키지에 속하지 않으면 새로운 패키지를 생성하고,
 * 이후 이 패키지를 저장합니다. 패키지 경로는 Unreal Engine에서 사용되는
 * "/Game" 또는 "/PluginName" 형식의 패키지 경로로 변환됩니다.
 *
 * @param SaveObject 저장할 UObject.
 * @param SaveDirectory 파일 시스템 상의 저장할 디렉터리 경로 (예: "C:/Unreal Projects/YourProject/Content/...").
 * @param FileName 저장할 파일 이름 (확장자는 필요하지 않음).
 */
UPackage* EditorPackageUtils::SaveAssetToPackage(UObject* const SaveObject, const FString& SaveDirectory, const FString& FileName, EObjectFlags TopLevelFlags)
{
    if (!SaveObject)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveObject is null!"));
        return nullptr;
    }

    // -- 프로젝트의 콘텐츠 디렉터리 경로를 절대 경로로 변환
    // SaveDirectory 에서 PackagePath 로 경로 변환
    FString RelativePath = EditorPackageUtils::ConvertFilePathToPackagePath(SaveDirectory);
    UE_LOG(LogTemp, Log, TEXT("Convert RelativePath: %s"), *RelativePath);
    if (RelativePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SaveDirectory is not inside any recognized content directory: %s (%s)"), *SaveDirectory, *RelativePath);
        return nullptr;
    }

    // -- 패키지 경로에 SaveObject 이름을 추가하여 최종 패키지 경로를 생성
    FString FullPackagePath = FPaths::Combine(RelativePath, FileName);
    UE_LOG(LogTemp, Log, TEXT("Full Package Path: %s"), *FullPackagePath);

    // -- 패키지 생성
    UPackage* ExistingPackage = FindPackage(nullptr, *FullPackagePath);
    if (!ExistingPackage)
    {
        // -- 패키지 생성
        ExistingPackage = CreatePackage(*FullPackagePath);
        if (!ExistingPackage)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPackagePath);
            return nullptr;
        }
        UE_LOG(LogTemp, Log, TEXT("Package created: %s"), *FullPackagePath);
    }
    SaveObject->Rename(*FileName, ExistingPackage);

    // -- Asset 등록
    if (EditorPackageUtils::IsAssetAlreadyRegistered(FullPackagePath) == false)
    {
        FAssetRegistryModule::AssetCreated(SaveObject);
    }

    // -- 패키지 저장
    SaveObject->MarkPackageDirty();

    if (!IFileManager::Get().DirectoryExists(*SaveDirectory))
    {
        IFileManager::Get().MakeDirectory(*SaveDirectory, true);
        UE_LOG(LogTemp, Log, TEXT("Created directory: %s"), *SaveDirectory);
    }

    FString FilePath = FPaths::Combine(SaveDirectory, FileName);
    FilePath = EditorPackageUtils::EnsureUAssetExtension(FilePath);

    // -- 패키지 저장 처리
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
