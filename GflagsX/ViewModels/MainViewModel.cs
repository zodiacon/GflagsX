using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Prism.Mvvm;
using Zodiacon.WPF;

namespace GflagsX.ViewModels {
	class MainViewModel : BindableBase {
		ObservableCollection<BindableBase> _tabItems = new ObservableCollection<BindableBase>();

		public IList<BindableBase> TabItems => _tabItems;

		public MainViewModel(IUIServices ui) {
			AddTab(new RegistryTabViewModel());
			AddTab(new KernelTabViewModel(), false);
		}

		private void AddTab(BindableBase tab, bool select = true) {
			_tabItems.Add(tab);
			if(select)
				SelectedTab = tab;
		}

		private BindableBase _selectedTab;

		public BindableBase SelectedTab {
			get { return _selectedTab; }
			set { SetProperty(ref _selectedTab, value); }
		}

		public string Title => "Global Flags X (C)2017 by Pavel Yosifovich";
	}
}
