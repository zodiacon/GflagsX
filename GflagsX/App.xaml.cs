using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using GflagsX.ViewModels;
using Zodiacon.WPF;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace GflagsX {
	public partial class App : Application {
		internal static MainViewModel MainViewModel { get; private set; }

		public App() {
		}

		protected override void OnStartup(StartupEventArgs e) {
			base.OnStartup(e);

			var vm = MainViewModel = new MainViewModel(new UIServicesDefaults());
			var win = new MainWindow() { DataContext = vm };
			win.Show();
			vm.UI.MessageBoxService.SetOwner(win);
		}
	}
}
