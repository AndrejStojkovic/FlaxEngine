#pragma once

#include "Editor/Scripting/CodeEditor.h"

/// <summary>
/// Implementation of code editor utility that is using Neovide.
/// </summary>
class NeovideEditor : public CodeEditor
{
private:

	String _execPath;
	String _solutionPath;
	
public:

	NeovideEditor(const String& execPath);

	static void FindEditors(Array<CodeEditor*>* output);

	CodeEditorTypes GetType() const override;
	String GetName() const override;
	void OpenFile(const String& path, int32 line) override;
	void OpenSolution() override;
	void OnFileAdded(const String& path) override;
};