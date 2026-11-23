#include "pch.h"
#include "Helpers.h"

std::wstring Helpers::ErrorText(DWORD error) {
	wil::unique_hlocal_string text;
	auto len = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0,
		reinterpret_cast<PWSTR>(text.addressof()), 0, nullptr);
	if (len == 0)
		return std::format(L"Error {}", error);
	std::wstring result(text.get(), len);
	while (!result.empty() && (result.back() == L'\n' || result.back() == L'\r' || result.back() == L' '))
		result.pop_back();
	return result;
}

std::wstring Helpers::Trim(std::wstring_view text) {
	while (!text.empty() && iswspace(text.front()))
		text.remove_prefix(1);
	while (!text.empty() && iswspace(text.back()))
		text.remove_suffix(1);
	return std::wstring(text);
}

std::vector<std::wstring> Helpers::SplitList(std::wstring_view text) {
	std::vector<std::wstring> items;
	size_t start = 0;
	while (start <= text.size()) {
		auto end = text.find_first_of(L",\r\n", start);
		if (end == std::wstring_view::npos)
			end = text.size();
		if (auto item = Trim(text.substr(start, end - start)); !item.empty())
			items.push_back(std::move(item));
		start = end + 1;
	}
	return items;
}

std::wstring Helpers::JoinLines(std::vector<std::wstring> const& items) {
	std::wstring text;
	for (auto& item : items) {
		if (!text.empty())
			text += L"\r\n";
		text += item;
	}
	return text;
}

std::wstring Helpers::FileName(std::wstring const& path) {
	auto slash = path.find_last_of(L"\\/");
	return slash == std::wstring::npos ? path : path.substr(slash + 1);
}
