using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GflagsX.Models {
	static class GlobalFlags {
		static List<GlobalFlag> _flags = new List<GlobalFlag> {
			new GlobalFlag("Stop on Exception", 1, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Show Loader Snaps", 2, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Debug Initial Command", 4, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Stop on Hung GUI", 8, GlobalFlagUsage.Kernel) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Heap Tail Checking", 0x10, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Heap Free Checking", 0x20, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Heap Parameter Checking", 0x40, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Heap Validation on Call", 0x80, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Application Verifier", 0x100, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Pool Tagging", 0x400, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.KernelModeOnly },
			new GlobalFlag("Enable Heap Tagging", 0x800, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Create User Mode Stack Trace Database", 0x1000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Create Kernel Mode Stack Trace Database", 0x2000, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.KernelModeOnly },
			new GlobalFlag("Maintain a List of Objects for Each Type", 0x4000, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.UserModeAndKernelMode },
			new GlobalFlag("Enable Heap Tagging by DLL", 0x8000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Debugging of the Windows Subsystem", 0x20000, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.UserModeAndKernelMode },
			new GlobalFlag("Disable Paging of Kernel Stacks", 0x80000, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.KernelModeOnly },
			new GlobalFlag("Enable System Critical Breaks", 0x100000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeAndKernelMode },
			new GlobalFlag("Disable Heap Coalesce on Free", 0x200000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Object Handle Type Tagging", 0x01000000, GlobalFlagUsage.Kernel | GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.UserModeAndKernelMode },
			new GlobalFlag("Enable Page Heap", 0x02000000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Debug WinLogon", 0x04000000, GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.UserModeAndKernelMode },
			new GlobalFlag("Buffer DbgPrint Output", 0x08000000, GlobalFlagUsage.Registry | GlobalFlagUsage.Kernel) { Category = GlobalFlagCategory.KernelModeOnly },
			new GlobalFlag("Early Critical Section Event Creation", 0x10000000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Stop on Unhandled User Mode Exception", 0x20000000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly },
			new GlobalFlag("Enable Bad Handles Detection", 0x40000000, GlobalFlagUsage.Kernel | GlobalFlagUsage.Registry) { Category = GlobalFlagCategory.UserModeAndKernelMode },
			new GlobalFlag("Disable Protected DLL Verification", 0x80000000, GlobalFlagUsage.All) { Category = GlobalFlagCategory.UserModeOnly }
		};

		public static IReadOnlyList<GlobalFlag> Flags => _flags;

	}
}
