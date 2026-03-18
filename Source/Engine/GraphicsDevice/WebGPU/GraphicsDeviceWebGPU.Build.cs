// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build.NativeCpp;
using Flax.Build.Platforms;

/// <summary>
/// WebGPU graphics backend module.
/// </summary>
public class GraphicsDeviceWebGPU : GraphicsDeviceBaseModule
{
    /// <summary>
    /// Using ASYNCIFY leads to simple code by waiting on async WebGPU API callbacks with emscripten_sleep but doubles the code size and adds some overhead.
    /// https://emscripten.org/docs/porting/asyncify.html <br/>
    /// 0 - no async <br/>
    /// 1 - via Asyncify (causes the WASM to be much larger) <br/>
    /// 2 - via JSPI (experimental) <br/>
    /// </summary>
    public int WithAsyncify = 2;

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var port = "--use-port=emdawnwebgpu:cpp_bindings=false";
        options.OutputFiles.Add(port);
        options.CompileEnv.CustomArgs.Add(port);
        if (WithAsyncify == 2)
        {
            options.PrivateDefinitions.Add("WEBGPU_ASYNCIFY=2");
            options.LinkEnv.CustomArgs.Add("-sJSPI");
            options.LinkEnv.CustomArgs.Add("-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$getWasmTableEntry");
        }
        else if (WithAsyncify == 1)
        {
            options.PrivateDefinitions.Add("WEBGPU_ASYNCIFY");
            options.LinkEnv.CustomArgs.Add("-sASYNCIFY");
            options.LinkEnv.CustomArgs.Add("-sASYNCIFY_STACK_SIZE=8192");
            //options.LinkEnv.CustomArgs.Add("-sASYNCIFY_ONLY=[\"main\",\"WebGPUAsyncWait(AsyncWaitParamsWebGPU)\"]"); // TODO: try indirect calls only to reduce the code size
            options.LinkEnv.CustomArgs.Add("-sEXPORT_ALL"); // This bloats JS but otherwise dynamic calls don't work properly
        }
        options.PublicDefinitions.Add("GRAPHICS_API_WEBGPU");
        options.PrivateIncludePaths.Add(Path.Combine(EmscriptenSdk.Instance.EmscriptenPath, "emscripten/cache/ports/emdawnwebgpu/emdawnwebgpu_pkg/webgpu/include"));
        options.PrivateDependencies.Add("lz4");
    }
}
