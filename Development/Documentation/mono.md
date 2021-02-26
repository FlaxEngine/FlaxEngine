## Mono

Custom fork: [https://github.com/FlaxEngine/mono](https://github.com/FlaxEngine/mono) with custom features for C# assemblies hot-reloading at runtime without domain unload (more: [https://flaxengine.com/blog/flax-facts-16-scripts-hot-reload/](https://flaxengine.com/blog/flax-facts-16-scripts-hot-reload/)).

### Notes

Some useful notes and tips for devs:
* When working with mono fork set `localRepoPath` to local repo location in `Source\Tools\Flax.Build\Deps\Dependencies\mono.cs`
* To update mono deps when developing/updating use `.\Development\Scripts\Windows\CallBuildTool.bat -log -ReBuildDeps -verbose -depsToBuild=mono -platform=Windows`, then build engine and run it
* `MONO_GC_DEBUG=check-remset-consistency` - it will do additional checks at each collection to see if there are any missing write barriers
* `MONO_GC_DEBUG=nursery-canaries` - it might catch some buffer overflows in case of problems in code.
* Methods `mono_custom_attrs_from_property` and `mono_custom_attrs_get_attr` are internally cached
* If C++ mono call a method in c# that will throw an error, error will be handled but, not completly. Calling relase domain will return random `Access memory violation`. First search for error in c# code. No workaround yet.
