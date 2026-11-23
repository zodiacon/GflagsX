#pragma once

//
// The NT global flags: what each bit means and where it can be set. The table (GFlags::All) is the one place a
// flag is described; the views show the flags of their target from it.
//
namespace GFlags {
	// where a flag can be set (a flag may have several)
	enum class Target : uint32_t {
		None = 0,
		Registry = 1,	// the system's flags in the registry, read at boot (Session Manager's GlobalFlag)
		Kernel = 2,		// the running system's flags, until the next boot
		Image = 4,		// an executable's (Image File Execution Options), for its processes started from now on
		All = Registry | Kernel | Image,
	};

	constexpr Target operator|(Target a, Target b) {
		return Target(uint32_t(a) | uint32_t(b));
	}
	constexpr bool HasTarget(Target targets, Target target) {
		return (uint32_t(targets) & uint32_t(target)) != 0;
	}

	// what a flag affects
	enum class Category {
		KernelMode,
		UserMode,
		Both,
	};

	struct Flag {
		PCWSTR Name;
		uint32_t Value;
		Target Targets;
		Category Category;
		PCWSTR Abbreviation;	// gflags.exe's ("hpa"); nullptr: none
	};

	// Silent Process Exit (FLG_MONITOR_SILENT_PROCESS_EXIT): an image's only; its settings are in their own key
	constexpr uint32_t MonitorSilentProcessExit = 0x200;
	// the image flags that turn on Application Verifier and page heap (their settings are FlagStore's)
	constexpr uint32_t EnableApplicationVerifier = 0x100;
	constexpr uint32_t EnablePageHeap = 0x02000000;

	std::span<const Flag> All();
	// the target's flags, in the table's order
	std::vector<Flag const*> ForTarget(Target target);
	// the bits the target's flags cover: Apply writes the value with the others as they were
	uint32_t TargetMask(Target target);

	// the flag with the abbreviation (without case); nullptr: none
	Flag const* FindByAbbreviation(std::wstring_view abbreviation);

	// "Kernel Mode", "User Mode", "Kernel and User Mode"
	PCWSTR CategoryName(Category category);
	// "0x02000000"
	std::wstring FormatValue(uint32_t value);
}
