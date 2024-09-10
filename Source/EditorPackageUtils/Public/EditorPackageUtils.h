// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class EDITORPACKAGEUTILS_API EditorPackageUtils
{
public:
    static FString ExtractModuleNameFromPath(const FString& FilePath);
    static FString EnsureUAssetExtension(const FString& FilePath);
    static FString ConvertFilePathToPackagePath(const FString& FilePath);
    static FString PluginLongPackageNameToFilename(const FString& FullPackagePath);
    static bool IsAssetAlreadyRegistered(const FString& AssetPath);
    static UScriptStruct* LoadStructDefinitionByName(const FString& ModuleName, const FString& StructName);
    static UClass* LoadClassDefinitionByName(const FString& ModuleName, const FString& ClassName);
    static void ExecuteBuildAndHotReload();
    static void RestartEditorWithProject(const FString& ProjectPath);
    static void StartBuildAndRestartEditor();
    static UPackage* SaveAssetToPackage(UObject* SaveObject, const FString& SaveDirectory, const FString& FileName, EObjectFlags TopLevelFlags);
};
