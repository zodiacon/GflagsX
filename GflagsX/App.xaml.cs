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

namespace GflagsX {
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application {
		internal static MainViewModel MainViewModel { get; private set; }

		readonly Dictionary<string, Assembly> _assemblies = new Dictionary<string, Assembly>(4);

		private void LoadAssemblies() {
			var appAssembly = typeof(App).Assembly;
			foreach (var resourceName in appAssembly.GetManifestResourceNames()) {
				if (resourceName.EndsWith(".dll", StringComparison.InvariantCultureIgnoreCase)) {
					using (var stream = appAssembly.GetManifestResourceStream(resourceName)) {
						var assemblyData = new byte[(int)stream.Length];
						stream.Read(assemblyData, 0, assemblyData.Length);
						var assembly = Assembly.Load(assemblyData);
						_assemblies.Add(assembly.GetName().Name, assembly);
					}
				}
			}
			AppDomain.CurrentDomain.AssemblyResolve += OnAssemblyResolve;
		}

		Assembly OnAssemblyResolve(object sender, ResolveEventArgs args) {
			var shortName = new AssemblyName(args.Name).Name;
			if (_assemblies.TryGetValue(shortName, out var assembly)) {
				return assembly;
			}
			return null;
		}

		public App() {
			LoadAssemblies();
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
