//Matthew Rosen
#include "WindowsFileBrowsing.h"
#include "Graphics\Graphics.h"
#include <PvDefinitions.h>

#if !defined(_WIN32)
#error
#endif
#include <Windows.h>
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

//@param outFile is the return filename of the open or save window
//@param defExtension should be your expected extension without the . ie "wav"
//@param fileExtensions should be in the form: "Extension Name\0*.extname\*Any File\0*.*\0"
//@param save : true for a window to save a file, false for opening a file
//@return if the operation succeeded, false if user clicks the "Cancel" button
bool SaveOrOpenFile(std::string& outfile, const char* defExtension, const char* fileExtensions, bool save)
{
	char fileName[MAX_PATH] = { 0 };
	OPENFILENAME file;
	ZeroMemory(&file, sizeof(file));

	file.lStructSize = sizeof(file);
	auto* window = Graphics::Instance().GetWindow();
	file.hwndOwner = glfwGetWin32Window(window);
	file.lpstrFilter = fileExtensions;
	file.lpstrDefExt = defExtension;
	file.lpstrFile = fileName;
	file.nMaxFile = MAX_PATH;
	file.Flags = OFN_DONTADDTORECENT | OFN_NOCHANGEDIR;
	if (save) file.Flags |= OFN_OVERWRITEPROMPT;
	else file.Flags |= OFN_FILEMUSTEXIST;

	bool success = false;
	if (save) success = GetSaveFileName(&file);
	else success = GetOpenFileName(&file);

	if (success)
	{
		outfile.assign(fileName);
	}

	return success;
}

WindowsCDAResult ConfirmationOfDestructiveAction(const std::string& prompt)
{
	int messageBoxID = MessageBox(NULL, prompt.c_str(), "", MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3);

	switch (messageBoxID)
	{
	case IDCANCEL:
		return wcr_cancel;
	case IDYES:
		return wcr_yes;
	case IDNO:
		return wcr_no;
	}

	// shouldn't get here!
	PV_ASSERT(false);
	return wcr_cancel;
}
