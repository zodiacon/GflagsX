using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using GflagsX.Models;
using Prism.Commands;
using Prism.Mvvm;
using System.Windows.Data;
using GflagsX.Converters;

namespace GflagsX.ViewModels {
	abstract class GlobalFlagsTabViewModelBase : BindableBase {
		List<GlobalFlagViewModel> _flags; 
		//GlobalFlagViewModel[] _flags1, _flags2;

		//public GlobalFlagViewModel[] Flags1 => _flags1;
		//public GlobalFlagViewModel[] Flags2 => _flags2;

		public IEnumerable<GlobalFlagViewModel> Flags => _flags;

		protected GlobalFlagsTabViewModelBase(GlobalFlagUsage usage) {
			_flags = GlobalFlags.Flags.Where(flag => (flag.Usage & usage) == usage)
				.Select(flag => new GlobalFlagViewModel(this, flag, usage)).ToList();
			CollectionViewSource.GetDefaultView(Flags).GroupDescriptions.Add(
				new PropertyGroupDescription("Category"));
			CalculateFlags();
		}

		internal void UpdateValue() {
			RaisePropertyChanged(nameof(FlagsValue));
		}

		public abstract void Apply();

		protected abstract void CalculateFlags();

		public uint FlagsValue => (uint)Flags.Where(flag => flag.IsEnabled).Sum(flag => flag.Flag.Value);

		public ICommand ApplyCommand => new DelegateCommand(() => Apply());

		public ICommand ReloadFlagsCommand => new DelegateCommand(() => CalculateFlags());
	}
}
