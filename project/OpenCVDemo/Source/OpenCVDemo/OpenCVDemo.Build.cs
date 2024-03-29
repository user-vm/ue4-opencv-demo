using System.IO;
using UnrealBuildTool;
using System.Diagnostics;

//For Tools.DotNETCommon.JsonObject and Tools.DotNETCommon.FileReference
using EpicGames.Core;

public class OpenCVDemo : ModuleRules
{
	private string ModuleRoot {
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../..")); }
	}
	
	private bool IsWindows(ReadOnlyTargetRules target) {
		return target.Platform == UnrealTargetPlatform.Win64; //no win32 support in Unreal 5, checking target.Platform == UnrealTargetPlatform.Win32 gives an error
	}
	
	private void ProcessDependencies(string depsJson, ReadOnlyTargetRules target)
	{
		//We need to ensure libraries end with ".lib" under Windows
		string libSuffix = ((this.IsWindows(target)) ? ".lib" : "");
		
		//Attempt to parse the JSON file
		JsonObject deps = JsonObject.Read(new FileReference(depsJson));
		
		//Process the list of dependencies
		foreach (JsonObject dep in deps.GetObjectArrayField("dependencies"))
		{
			//Add the header and library paths for the dependency package
			PublicIncludePaths.AddRange(dep.GetStringArrayField("include_paths"));
			PublicSystemLibraryPaths.AddRange(dep.GetStringArrayField("lib_paths"));
			
			//Add the preprocessor definitions from the dependency package
			PublicDefinitions.AddRange(dep.GetStringArrayField("defines"));
			
			//Link against the libraries from the package
			string[] libs = dep.GetStringArrayField("libs");
			foreach (string lib in libs)
			{
				string libFull = lib + ((libSuffix.Length == 0 || lib.EndsWith(libSuffix)) ? "" : libSuffix);
				PublicAdditionalLibraries.Add(libFull);
			}
		}
	}
	
	public OpenCVDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		//Link against our engine dependencies
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PublicDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore", "Media", "MediaAssets" });

		if (Target.Platform != UnrealTargetPlatform.Android) //this might not be necessary, depends on what happens if the build.py in package/ is built for Android
		{
			//Install third-party dependencies using conan
			Process.Start(new ProcessStartInfo
				{
					FileName = "conan",
					Arguments = "install . --profile ue4",
					WorkingDirectory = ModuleRoot
				})
				.WaitForExit();
		}
		else
		{
			//Install third-party dependencies using conan
			Process.Start(new ProcessStartInfo
				{
					FileName = "conan",
					Arguments = "install . --profile:build ue4 --profile:host ue4.27-Android-armv8-unknown-linux-gnu_3",
					WorkingDirectory = ModuleRoot
				})
				.WaitForExit();
		}
		
		//Link against our conan-installed dependencies
		this.ProcessDependencies(Path.Combine(ModuleRoot, "conanbuildinfo.json"), Target);
    }
}
