//Matthew Rosen
#pragma once

#include <string>

//@param outFile is the return filename of the open or save window
//@param defExtension should be your expected extension without the . ie "wav"
//@param fileExtensions should be in the form: "Extension Name\0*.extname\*Any File\0*.*\0"
//@param save : true for a window to save a file, false for opening a file
//@return if the operation succeeded, false if user clicks the "Cancel" button
bool SaveOrOpenFile(std::string& outfile, const char* defExtension, const char* fileExtensions, bool save);

enum WindowsCDAResult
{
	wcr_yes,
	wcr_no,
	wcr_cancel
};

WindowsCDAResult ConfirmationOfDestructiveAction(const std::string& prompt);
