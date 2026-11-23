#pragma once

namespace Helpers {
	// the system's message for a Win32 error, without the trailing line break
	std::wstring ErrorText(DWORD error);
	// a list typed one per line (or separated by commas), without blanks and surrounding spaces
	std::vector<std::wstring> SplitList(std::wstring_view text);
	// one per line, with CR LF (an edit control's)
	std::wstring JoinLines(std::vector<std::wstring> const& items);
	// the text without surrounding white space
	std::wstring Trim(std::wstring_view text);
	// the last component of a path ("C:\Windows\notepad.exe" -> "notepad.exe")
	std::wstring FileName(std::wstring const& path);
}
