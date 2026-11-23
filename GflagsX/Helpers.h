#pragma once

namespace Helpers {
	// the system's message for a Win32 error, without the trailing line break
	std::wstring ErrorText(DWORD error);
	// a list typed one per line (or separated by commas), without blanks and surrounding spaces
	std::vector<std::wstring> SplitList(std::wstring_view text);
	// words separated by spaces, tabs, commas or line breaks (as PageHeapTargetDlls and VerifierDlls have them)
	std::vector<std::wstring> SplitWords(std::wstring_view text);
	// separated by single spaces
	std::wstring JoinWords(std::vector<std::wstring> const& words);
	// one per line, with CR LF (an edit control's)
	std::wstring JoinLines(std::vector<std::wstring> const& items);
	// the text without surrounding white space
	std::wstring Trim(std::wstring_view text);
	// a number typed in decimal or in hex with 0x (surrounding spaces allowed); nullopt if it's not one or it's past 32 bits
	std::optional<uint32_t> ParseNumber(std::wstring_view text);
	// the last component of a path ("C:\Windows\notepad.exe" -> "notepad.exe")
	std::wstring FileName(std::wstring const& path);
}
