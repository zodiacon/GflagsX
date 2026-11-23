#include "pch.h"
#include "PageHeap.h"

using namespace PageHeap;

namespace {
	const Option s_Options[] = {
		{ L"Collect stack traces", StackTraces, false },
		{ L"Catch backward overruns", BackwardOverruns, true },
		{ L"Unaligned allocations", Unaligned, true },
		{ L"Decommit guard pages (less memory)", Decommit, true },
		{ L"Protect the heap's internal structures", Protect, true },
		{ L"No critical section (lock) checks", NoLockChecks, false },
	};

	// the summary's names, short
	struct Named {
		uint32_t Value;
		PCWSTR Name;
	};
	const Named s_Names[] = {
		{ StackTraces, L"stack traces" },
		{ BackwardOverruns, L"backward overruns" },
		{ Unaligned, L"unaligned" },
		{ Decommit, L"decommit" },
		{ Protect, L"protect" },
		{ NoLockChecks, L"no lock checks" },
		{ SizeRange, L"size range" },
		{ DllRange, L"address range" },
		{ Random, L"random" },
		{ TargetDlls, L"DLLs" },
		{ Faults, L"faults" },
	};
}

std::span<const Option> PageHeap::Options() {
	return s_Options;
}

std::wstring PageHeap::Summary(uint32_t flags) {
	std::wstring text = flags & Full ? L"Full" : L"Normal";
	bool first = true;
	for (auto& named : s_Names) {
		if (flags & named.Value) {
			text += first ? L": " : L", ";
			text += named.Name;
			first = false;
		}
	}
	return text;
}
