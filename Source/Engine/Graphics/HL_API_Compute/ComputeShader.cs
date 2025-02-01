using System;
using System.Collections.Generic;
using System.Xml.Linq;

namespace FlaxEngine
{
    /// <summary>
    /// </summary>
    /// <seealso cref="FlaxEngine.Shader" />
    public class ComputeShader(Shader shader)
    {
        private Shader shader = shader; //todo use shader as a base class and remove this
        private ComputeBuffer constantBuffer = null;
        private struct Bind
        {
            public int ID;
            public GPUBuffer b;
            public GPUTexture t;
            public bool IsUA;
            public GPUResourceView GetView()
            {
                if (t != null)
                    return t.View();
                if (b != null)
                    return b.View();
                return null;
            }
            public string GetName()
            {
                if (t != null)
                    return t.Name;
                if (b != null)
                    return b.Name;
                return "";
            }
            public bool IsBuffer() { return b != null; }
            public bool IsTexture() { return t != null; }
        }
        private readonly List<Bind> bindings = [];
        private int GetSlotID(int id)
        {
            foreach (var binding in bindings)
            {
                if (binding.ID == id)
                    return id;
            }
            var ret = bindings.Count;
            bindings.Add(new Bind() { ID = id });
            return ret;
        }

        public void SetConstantBuffer(ComputeBuffer buffer)
        {
            if (buffer == null)
            {
                Debug.Logger.LogException(new Exception("u forgot sometfing the ComputeBuffer is null"));
                return;
            }

            if (buffer.m_Type != ComputeBuffer.Type.Constant)
            {
                Debug.Logger.LogException(new Exception("Sorry wrong buffer type use ComputeBuffer.Type.Constant insted"));
                return;
            }
            constantBuffer = buffer;
        }
        public void SetBuffer(int id, ComputeBuffer buffer)
        {
            if (buffer == null)
            {
                Debug.Logger.LogException(new Exception("u forgot sometfing the ComputeBuffer is null"));
                return;
            }
            GPUBuffer b = buffer.GetGPUBuffer();
            if (b == null)
            {
                return;//cant bind do to errors
            }
            int bindid = GetSlotID(id);
            var bind = bindings[bindid];
            if (bind.IsTexture())
            {
                //avoid double bind.
                //SetTexture(0,...);
                //SetBuffer(0,...);
                Debug.Logger.LogException(new Exception(id.ToString() + " slot is bound to the texture allredy"));
                return;
            }
            var isUA = b.Description.Flags.HasFlag(GPUBufferFlags.UnorderedAccess);
            bind.b = b;
            bind.IsUA = isUA;
            bindings[bindid] = bind;
        }
        public void SetTexture(int id, GPUTexture texture)
        {
            if (texture == null)
            {
                Debug.Logger.LogException(new Exception("u forgot sometfing the GPUTexture is null"));
                return;
            }
            int bindid = GetSlotID(id);
            var bind = bindings[bindid];
            if (bind.IsBuffer())
            {
                //avoid double bind.
                //SetBuffer(0,...);
                //SetTexture(0,...);
                Debug.Logger.LogException(new Exception(id.ToString() + " slot is bound to the buffer allredy"));
                return;
            }
            var isUA = texture.Description.Flags.HasFlag(GPUTextureFlags.UnorderedAccess);
            bind.t = texture;
            bind.IsUA = isUA;
            bindings[bindid] = bind;
        }
        public bool HasKernel(string name)
        {
            return shader.GPU.GetCS(name) != nint.Zero;
        }
        /// <summary>
        /// shader goes brrrrrrrrr maybe<br/>
        /// </summary>
        /// <param name="kernelName">The kernelName.</param>
        /// <param name="threadGroupsX">The thread groups x.</param>
        /// <param name="threadGroupsY">The thread groups y.</param>
        /// <param name="threadGroupsZ">The thread groups z.</param>
        public void Dispatch(string kernelName, int threadGroupsX, int threadGroupsY, int threadGroupsZ)
        {
            var context = GPUDevice.Instance.MainContext;
            Dispatch(context, kernelName, (uint)threadGroupsX, (uint)threadGroupsY, (uint)threadGroupsZ);
        }
        /// <summary>
        /// shader goes brrrrrrrrr maybe<br/>
        /// </summary>
        /// <param name="kernelName">The kernelName.</param>
        /// <param name="threadGroupsX">The thread groups x.</param>
        /// <param name="threadGroupsY">The thread groups y.</param>
        /// <param name="threadGroupsZ">The thread groups z.</param>
        public void Dispatch(string kernelName, uint threadGroupsX, uint threadGroupsY = 1u, uint threadGroupsZ = 1u)
        {
            var context = GPUDevice.Instance.MainContext;
            Dispatch(context, kernelName, threadGroupsX, threadGroupsY, threadGroupsZ);
        }

        /// <summary>
        /// Dispatches the specified context.
        /// </summary>
        /// <param name="context">The context.</param>
        /// <param name="kernelName">The kernelName.</param>
        /// <param name="threadGroupsX">The thread groups x.</param>
        /// <param name="threadGroupsY">The thread groups y.</param>
        /// <param name="threadGroupsZ">The thread groups z.</param>
        private void Dispatch(GPUContext context, string kernelName, uint threadGroupsX, uint threadGroupsY, uint threadGroupsZ)
        {
            if (!GPUDevice.Instance.Limits.HasCompute)
            {
                Debug.Logger.LogException(new Exception("GPUDevice is not suporting Compute,can't run the Dispatch"));
                return;
            }
            if (!(shader && shader.IsLoaded))
            {
                Debug.Logger.LogException(new Exception("shader is null or not Loaded,can't run the Dispatch"));
                return;
            }

            var massange = string.Format("ComputeShader.Dispatch {0} ThreadGroupsCount[{1},{2},{3}]", kernelName, threadGroupsX.ToString(), threadGroupsY.ToString(), threadGroupsZ.ToString());
            Debug.Logger.Log(massange + " Has been called");
            Profiler.BeginEvent(massange);

            var kernal = shader.GPU.GetCS(kernelName);
            if (kernal == nint.Zero)
            {
                Debug.LogException(new System.Exception("Faled to find kernal with name of : " + kernelName));
                return;
            }
            if (constantBuffer != null)
            {
                var cb = shader.GPU.GetCB(0);
                if (cb != IntPtr.Zero)
                {
                    var ptr = constantBuffer.GetDataPointer();
                    if (ptr != IntPtr.Zero)
                    {
                        context.UpdateCB(cb, ptr);
                        context.BindCB(0, cb);
                    }
                    else
                    {
                        Debug.LogException(new System.Exception("Faled to Dispatch constantBuffer is empty"));
                    }
                }
                else
                {
                    Debug.LogException(new System.Exception("Faled to Dispatch reqested a constant but go noting"));
                }
            }
            foreach (var bind in bindings)
            {
                if (bind.IsUA)
                {
                    //bind unordered acces resource
                    context.BindUA(bind.ID, bind.GetView());
                    //Debug.Log("Resource has ben bound <" + bind.ID.ToString() + "," + bind.GetName() + "> as unordered acces ");
                }
                else
                {
                    //bind shader resource
                    context.BindSR(bind.ID, bind.GetView());
                }
            }

            //clamp threadGroups
            threadGroupsX = Mathf.Max(threadGroupsX, 1);
            threadGroupsY = Mathf.Max(threadGroupsY, 1);
            threadGroupsZ = Mathf.Max(threadGroupsZ, 1);
            context.Dispatch(kernal, threadGroupsX, threadGroupsY, threadGroupsZ);

            //clear state
            context.ResetUA();
            context.ResetSR();
            context.ResetCB();

            Profiler.EndEvent();
        }
    }
}
