#include "NeovideEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Core/Log.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Engine/Globals.h"
#if PLATFORM_LINUX
#include <stdio.h>
#elif PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif

NeovideEditor::NeovideEditor(const String& execPath)
	: _execPath(execPath)
	, _solutionPath(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln"))
{
}

void NeovideEditor::FindEditors(Array<CodeEditor*>* output)
{
    String result = TEXT("C:/Program Files/Neovide/neovide.exe");

    if (FileSystem::FileExists(result))
	{
		output->Add(New<NeovideEditor>(result));
	}
}

CodeEditorTypes NeovideEditor::GetType() const
{
	return CodeEditorTypes::Neovide;
}

String NeovideEditor::GetName() const
{
	return TEXT("Neovide");
}

void NeovideEditor::OpenSolution()
{
	if (!FileSystem::FileExists(_solutionPath))
	{
		ScriptsBuilder::GenerateProject();
	}

	CreateProcessSettings procSettings;

	procSettings.FileName = _execPath;
	procSettings.Arguments = String::Format(TEXT("\"{0}\""), _solutionPath);
	procSettings.HiddenWindow = false;
	procSettings.WaitForEnd = false;
	procSettings.LogOutput = false;
	procSettings.ShellExecute = true;

	Platform::CreateProcess(procSettings);
}

void NeovideEditor::OpenFile(const String& path, int32 line)
{
	if (!FileSystem::FileExists(_solutionPath))
	{
		ScriptsBuilder::GenerateProject();
	}

	line = line > 0 ? line : 1;
	CreateProcessSettings procSettings;

	procSettings.FileName = _execPath;
	procSettings.Arguments = String::Format(TEXT("\"{0}\" \"{1}\""), _solutionPath, path);
	//procSettings.Arguments = String::Format(TEXT("\"{0}\" --line {2} \"{1}\""), _solutionPath, path, line);
	procSettings.HiddenWindow = false;
	procSettings.WaitForEnd = false;
	procSettings.LogOutput = false;
	procSettings.ShellExecute = true;

	Platform::CreateProcess(procSettings);
}

void NeovideEditor::OnFileAdded(const String& path)
{
	ScriptsBuilder::GenerateProject();
}
