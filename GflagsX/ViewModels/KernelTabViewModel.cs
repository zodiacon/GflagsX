using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GflagsX.Models;
using Prism.Mvvm;

namespace GflagsX.ViewModels {
	class KernelTabViewModel : GlobalFlagsTabViewModelBase {
		public string Text => "Kernel";
		public string Icon => "/icons/cpu.ico";

		public KernelTabViewModel() : base(GlobalFlagUsage.Kernel) {	
		}

		protected override void CalculateFlags() {
			var ntGlobalFlags = NativeMethods.RtlGetNtGlobalFlags();
			foreach(var vm in Flags) {
				vm.IsEnabled = (ntGlobalFlags & vm.Flag.Value) == vm.Flag.Value;
			}
		}

		public override void Apply() {
			var uvalue = FlagsValue;
			NativeMethods.NtSetSystemInformation(SystemInformationClass.SystemFlagsInformation, ref uvalue);
		}
	}
}
