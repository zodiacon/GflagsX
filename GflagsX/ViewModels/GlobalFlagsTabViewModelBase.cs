using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using GflagsX.Models;
using Prism.Commands;
using Prism.Mvvm;

namespace GflagsX.ViewModels {
	abstract class GlobalFlagsTabViewModelBase : BindableBase {
		List<GlobalFlagViewModel> _flags; 
		GlobalFlagViewModel[] _flags1, _flags2;

		public GlobalFlagViewModel[] Flags1 => _flags1;
		public GlobalFlagViewModel[] Flags2 => _flags2;

		public IEnumerable<GlobalFlagViewModel> Flags => _flags;

		protected GlobalFlagsTabViewModelBase(GlobalFlagUsage usage) {
			_flags = GlobalFlags.Flags.Select(flag => new GlobalFlagViewModel(this, flag, usage)).ToList();
			//_flags1 = _flags.Take(_flags.Count / 2).ToArray();
			//_flags2 = _flags.Skip(_flags.Count / 2).Take(_flags.Count / 2).ToArray();
			CalculateFlags();
		}

		internal void UpdateValue() {
			OnPropertyChanged(nameof(FlagsValue));
		}

		public abstract void Apply();

		protected abstract void CalculateFlags();

		public uint FlagsValue => (uint)Flags.Where(flag => flag.IsEnabled).Sum(flag => flag.Flag.Value);

		public ICommand ApplyCommand => new DelegateCommand(() => Apply());
	}
}
