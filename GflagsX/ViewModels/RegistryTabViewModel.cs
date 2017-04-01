using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GflagsX.Models;
using Microsoft.Win32;
using Prism.Mvvm;

namespace GflagsX.ViewModels {
	class RegistryTabViewModel : GlobalFlagsTabViewModelBase {
		const string NtGlobalFlagsKeyPath = @"System\CurrentControlSet\Control\Session Manager";

		public string Text => "Registry";
		public string Icon => "/icons/registry.ico";

		public RegistryTabViewModel() : base(GlobalFlagUsage.Registry) {
		}

		public override void Apply() {
			using(var key = Registry.LocalMachine.OpenSubKey(NtGlobalFlagsKeyPath, true)) {
				key.SetValue("GlobalFlag", FlagsValue, RegistryValueKind.DWord);
			}
		}

		protected override void CalculateFlags() {
			using(var key = Registry.LocalMachine.OpenSubKey(NtGlobalFlagsKeyPath)) {
				var ntGlobalFlags = (int)key.GetValue("GlobalFlag");
				foreach(var vm in Flags) {
					vm.IsEnabled = (ntGlobalFlags & vm.Flag.Value) == vm.Flag.Value;
				}
			}
		}
	}
}
