#include "pch.h"
#include "GlobalFlags.h"

using namespace GFlags;

namespace {
	constexpr auto Reg = Target::Registry;
	constexpr auto Ker = Target::Kernel;
	constexpr auto Img = Target::Image;
	constexpr auto All_ = Target::All;

	// as the WPF version had them, which follows GFlags: where each flag can be set, and what it affects
	const Flag s_Flags[] = {
		{ L"Stop on Exception", 0x1, All_, Category::UserMode },
		{ L"Show Loader Snaps", 0x2, All_, Category::UserMode },
		{ L"Debug Initial Command", 0x4, Reg, Category::UserMode },
		{ L"Stop on Hung GUI", 0x8, Ker, Category::UserMode },
		{ L"Enable Heap Tail Checking", 0x10, All_, Category::UserMode },
		{ L"Enable Heap Free Checking", 0x20, All_, Category::UserMode },
		{ L"Enable Heap Parameter Checking", 0x40, All_, Category::UserMode },
		{ L"Enable Heap Validation on Call", 0x80, All_, Category::UserMode },
		{ L"Enable Application Verifier", 0x100, All_, Category::UserMode },
		{ L"Silent Process Exit", MonitorSilentProcessExit, Img, Category::UserMode },
		{ L"Enable Pool Tagging", 0x400, Reg, Category::KernelMode },
		{ L"Enable Heap Tagging", 0x800, All_, Category::UserMode },
		{ L"Create User Mode Stack Trace Database", 0x1000, All_, Category::UserMode },
		{ L"Create Kernel Mode Stack Trace Database", 0x2000, Reg, Category::KernelMode },
		{ L"Maintain a List of Objects for Each Type", 0x4000, Reg, Category::Both },
		{ L"Enable Heap Tagging by DLL", 0x8000, All_, Category::UserMode },
		{ L"Enable Debugging of the Windows Subsystem", 0x20000, Reg, Category::UserMode },
		{ L"Enable Loading of Kernel Debugging Symbols", 0x40000, Reg | Ker, Category::KernelMode },
		{ L"Disable Paging of Kernel Stacks", 0x80000, Reg, Category::KernelMode },
		{ L"Enable System Critical Breaks", 0x100000, All_, Category::Both },
		{ L"Disable Heap Coalesce on Free", 0x200000, All_, Category::UserMode },
		{ L"Enable Object Handle Type Tagging", 0x01000000, Reg | Ker, Category::Both },
		{ L"Enable Page Heap", 0x02000000, All_, Category::UserMode },
		{ L"Debug WinLogon", 0x04000000, Reg, Category::Both },
		{ L"Buffer DbgPrint Output", 0x08000000, Reg | Ker, Category::KernelMode },
		{ L"Early Critical Section Event Creation", 0x10000000, All_, Category::UserMode },
		{ L"Stop on Unhandled User Mode Exception", 0x20000000, All_, Category::UserMode },
		{ L"Enable Bad Handles Detection", 0x40000000, Reg | Ker, Category::Both },
		{ L"Disable Protected DLL Verification", 0x80000000, All_, Category::UserMode },
	};
}

std::span<const Flag> GFlags::All() {
	return s_Flags;
}

std::vector<Flag const*> GFlags::ForTarget(Target target) {
	std::vector<Flag const*> flags;
	for (auto& flag : s_Flags)
		if (HasTarget(flag.Targets, target))
			flags.push_back(&flag);
	return flags;
}

uint32_t GFlags::TargetMask(Target target) {
	uint32_t mask = 0;
	for (auto& flag : s_Flags)
		if (HasTarget(flag.Targets, target))
			mask |= flag.Value;
	return mask;
}

PCWSTR GFlags::CategoryName(Category category) {
	switch (category) {
		case Category::KernelMode: return L"Kernel Mode";
		case Category::UserMode: return L"User Mode";
	}
	return L"Kernel and User Mode";
}

std::wstring GFlags::FormatValue(uint32_t value) {
	return std::format(L"0x{:08X}", value);
}
