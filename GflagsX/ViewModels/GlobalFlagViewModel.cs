using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GflagsX.Models;
using Prism.Mvvm;

namespace GflagsX.ViewModels {
	class GlobalFlagViewModel : BindableBase {
		public GlobalFlag Flag { get; }

		GlobalFlagsTabViewModelBase _vm;
		public GlobalFlagViewModel(GlobalFlagsTabViewModelBase vm, GlobalFlag flag, GlobalFlagUsage usage) {
			Flag = flag;
			_vm = vm;
			IsVisible = (usage & flag.Usage) == usage;
		}

		private bool _isEnabled;

		public bool IsEnabled {
			get { return _isEnabled; }
			set {
				if(SetProperty(ref _isEnabled, value))
					_vm.UpdateValue();
			}
		}

		public bool IsVisible { get; }
		public string Name => Flag.Name;
	}
}
