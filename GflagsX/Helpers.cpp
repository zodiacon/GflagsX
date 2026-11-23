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

std::optional<uint32_t> Helpers::ParseNumber(std::wstring_view text) {
	auto trimmed = Trim(text);
	std::wstring_view digits = trimmed;
	int base = 10;
	if (digits.size() > 2 && digits[0] == L'0' && (digits[1] == L'x' || digits[1] == L'X')) {
		digits.remove_prefix(2);
		base = 16;
	}
	if (digits.empty())
		return std::nullopt;
	uint64_t value = 0;
	for (auto c : digits) {
		int digit;
		if (c >= L'0' && c <= L'9')
			digit = c - L'0';
		else if (base == 16 && c >= L'a' && c <= L'f')
			digit = c - L'a' + 10;
		else if (base == 16 && c >= L'A' && c <= L'F')
			digit = c - L'A' + 10;
		else
			return std::nullopt;
		value = value * base + digit;
		if (value > 0xffffffff)
			return std::nullopt;
	}
	return uint32_t(value);
}

std::vector<std::wstring> Helpers::SplitWords(std::wstring_view text) {
	std::vector<std::wstring> words;
	std::wstring word;
	for (auto c : text) {
		if (iswspace(c) || c == L',') {
			if (!word.empty())
				words.push_back(std::move(word));
			word.clear();
		}
		else {
			word += c;
		}
	}
	if (!word.empty())
		words.push_back(std::move(word));
	return words;
}

std::wstring Helpers::JoinWords(std::vector<std::wstring> const& words) {
	std::wstring text;
	for (auto& word : words) {
		if (word.empty())
			continue;
		if (!text.empty())
			text += L' ';
		text += word;
	}
	return text;
}
