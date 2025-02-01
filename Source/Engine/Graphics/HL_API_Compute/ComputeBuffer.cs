using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using FlaxEngine;

namespace FlaxEngine
{
    /// <summary>
    /// ComputeBuffer class.
    /// </summary>
    public class ComputeBuffer
    {
        private Dictionary<System.Type, PixelFormat> RWBufferFromats = new Dictionary<System.Type, PixelFormat>()
        {
            { typeof(float),    PixelFormat.R32_Float },
            { typeof(int),      PixelFormat.R32_SInt },
            { typeof(uint),     PixelFormat.R32_UInt },
        };


//can be null depends on the usage
private GPUBuffer m_GraphicStageingBuffer;
        //never null unless somfing went very wrong
        private GPUBuffer m_GraphicBuffer;
        internal readonly Type m_Type;
        internal readonly Mode m_Mode;
        private byte[] m_MemoryBuffer = null;
        private PixelFormat pixelFormat = PixelFormat.MAX;
        //--------------------------------------------------------------------
        public enum Type
        {
            // Default ComputeBuffer type maps to Buffer<> / RWBuffer<> in hlsl.
            Default,
            // ComputeBuffer that you can use as a structured buffer maps to StructuredBuffer<> / RWStructuredBuffer<> in hlsl.
            Structured,
            // ComputeBuffer that you can use as a constant buffer(uniform buffer).
            Constant,
        }
        public enum Mode
        {
            // Static buffer, only initial upload allowed by the CPU
            Immutable,
            // Dynamic buffer. can be only Write Only by the CPU
            DynamicWriteOnly,
            // Dynamic buffer. can be only Read Only by the CPU
            DynamicReadOnly,
            // Dynamic buffer. allows for read and write by the CPU
            DynamicReadWrite,
        }
        //--------------------------------------------------------------------

        // The debug label for the compute buffer.
        public string Name
        {
            set
            {
                if (m_GraphicStageingBuffer != null)
                    m_GraphicStageingBuffer.Name = "ComputeStageingBuffer" + value;
                if (m_GraphicBuffer != null)
                    m_GraphicBuffer.Name = "ComputeBuffer " + value;
            }
        }
        // Size of one element in the buffer in bytes.
        public readonly uint Stride;
        // Number of elements in the buffer.
        public readonly uint Count;
        public uint SizeInBytes => Stride * Count;

        public ComputeBuffer(int count, int stride) : this(count, stride, Type.Default) { }
        public ComputeBuffer(int count, int stride, Type type) : this(count, stride, Type.Default, Mode.Immutable) { }
        public ComputeBuffer(int count, int stride, Type type, Mode mode)
        {
            if (count == 0)
            {
                Debug.LogException(new Exception("faled to construct ComputeBuffer, Count is out of range expects more then 0 element"));
                return;
            }
            if (!Mathf.IsInRange(stride, 1, 1024))
            {
                Debug.LogException(new Exception("faled to construct ComputeBuffer, Stride is out of range exected 1 to 1024"));
                return;
            }
            Count = (uint)count;
            Stride = (uint)stride;
            m_Mode = mode;
            m_Type = type;
        }
        public void SetData<T>(T data)
        {
            SetData(new T[] {data});
        }
        public void SetData<T>(T[] data)
        {
            //let guard this funcion 

            //... yes :D (it is cheked in consructor)
            if (Count == 0)
            {
                Debug.LogException(new Exception("faled to SetData, Count is out of range expects more then 0 element"));
                return;
            }
            if (!Mathf.IsInRange(Stride, 1, 1024))
            {
                Debug.LogException(new Exception("faled to SetData, Stride is out of range exected 1 to 1024"));
                return;
            }
            //cheak if data is valid
            if (data == null)
            {
                Debug.LogException(new ArgumentNullException(nameof(data)));
                return;
            }
            if (data.Length == 0)
            {
                Debug.LogException(new ArgumentOutOfRangeException(nameof(data)));
                return;
            }
            //cheak if <T> size is same side as stride
            var pSrcStride = Unsafe.SizeOf<T>();
            if (Stride != pSrcStride)
            {
                Debug.LogException(new Exception(string.Format(
                    "faled to SetData, Stride size mismatch expected {0} but got {1}",
                    Stride,
                    pSrcStride
                    )));
                return;
            }
            //are we done with the guard ? nope :D

            //find the format
            PixelFormat pf = PixelFormat.MAX;
            if (ChekaAndGetFormat<T>("SetData", ref pf))
            {
                pixelFormat = pf;
            }
            else
            {
                return;
            }

            //are we done with the guard yet ? a hell no :D

            switch (m_Mode)
            {
                case Mode.DynamicReadOnly:
                    Debug.LogException(new Exception("faled to SetData, Can't upload data to dynamic read only buffer"));
                    return;
                case Mode.Immutable:
                    if (m_MemoryBuffer != null)
                    {
                        Debug.LogException(new Exception("faled to SetData, Can't upload data to Immutable buffer"));
                        return;
                    }
                    break;
            }

            //now we are safe :)

            //aloc only if is null
            m_MemoryBuffer ??= new byte[Count * Stride];
            //copy data to local buffer we are not poking araund with dangling pointers
            unsafe
            {
                //the C# way of doing a memcoppy ?
                var pSrc = Unsafe.AsPointer(ref data[0]);
                var pDest = Unsafe.AsPointer(ref m_MemoryBuffer[0]);
                Buffer.MemoryCopy(pSrc, pDest, SizeInBytes, SizeInBytes);
            }
            if (m_Type == Type.Constant)
                return;//we are done data is inside the buffer on cpu when computer will get dispatched it can be pulled

            if (m_GraphicBuffer == null)
                CreateGPUBuffers();
            else if (m_Mode == Mode.DynamicWriteOnly || m_Mode == Mode.DynamicReadWrite)
            {
                //write to the GPU buffer vita GraphicStageingBuffer
                //this place might explode

                var pBuff = m_GraphicStageingBuffer.Map(GPUResourceMapMode.ReadWrite);
                if (pBuff != nint.Zero)
                {
                    unsafe
                    {
                        //the C# way of doing a memcoppy ?
                        var pSrc = Unsafe.AsPointer(ref m_MemoryBuffer[0]);
                        var pDest = pBuff.ToPointer();
                        Buffer.MemoryCopy(pSrc, pDest, SizeInBytes, SizeInBytes);
                    }
                    m_GraphicStageingBuffer.Unmap();
                    GPUDevice.Instance.MainContext.CopyBuffer(m_GraphicBuffer, m_GraphicStageingBuffer, SizeInBytes);
                }
                else
                {
                    Debug.LogError("can't write to the buffer ? if u see this massage somfing went wrong on the back end ?");
                }
            }
        }

        public T[] GetData<T>()
        {
            //let guard this funcion 
            //it is good because it will be smaller compered to SetData :D

            //find the format
            PixelFormat pf = PixelFormat.MAX;
            if (ChekaAndGetFormat<T>("GetData", ref pf))
            {
                pixelFormat = pf;
            }
            else
            {
                return null;
            }

            if (m_Mode == Mode.DynamicReadOnly || m_Mode == Mode.DynamicReadWrite)
            {
                var readmode = m_Mode == Mode.DynamicReadOnly ? GPUResourceMapMode.Read : GPUResourceMapMode.ReadWrite;

                T[] data = new T[Count];
                GPUDevice.Instance.MainContext.CopyBuffer(m_GraphicStageingBuffer, m_GraphicBuffer, SizeInBytes);
                var pBuff = m_GraphicStageingBuffer.Map(readmode);
                if (pBuff != nint.Zero)
                {
                    unsafe
                    {
                        //the C# way of doing a memcoppy ?
                        var pDest = Unsafe.AsPointer(ref data[0]);
                        var pSrc = pBuff.ToPointer();
                        Buffer.MemoryCopy(pSrc, pDest, SizeInBytes, SizeInBytes);
                    }
                    m_GraphicStageingBuffer.Unmap();
                }
                else
                {
                    Debug.LogError("can't write to the buffer ? if u see this massage somfing went wrong on the back end ?");
                }
                return data;
            }
            return null;
        }


        private void CreateGPUBuffers()
        {
            GPUBufferDescription description = new();
            description.Format = pixelFormat;
            description.Usage = GPUResourceUsage.Default;
            description.InitData = GetDataPointer();
            description.Flags = GPUBufferFlags.ShaderResource;
            description.Stride = Stride;
            description.Size = SizeInBytes;
            switch (m_Type)
            {
                case Type.Default:
                    if (m_Mode != Mode.Immutable)
                        description.Flags |= GPUBufferFlags.UnorderedAccess;
                    break;
                case Type.Structured:
                    if (m_Mode != Mode.Immutable)
                        description.Flags |= GPUBufferFlags.UnorderedAccess | GPUBufferFlags.Structured;
                    break;
            }

            m_GraphicBuffer = new GPUBuffer();
            m_GraphicBuffer.Init(ref description);
            switch (m_Mode)
            {
                case Mode.DynamicReadOnly:
                    {
                        m_GraphicStageingBuffer = new GPUBuffer();
                        var descriptionStageingBuffer = description.ToStagingUpload();
                        m_GraphicStageingBuffer.Init(ref descriptionStageingBuffer);
                    }
                    break;
                case Mode.DynamicWriteOnly:
                    {
                        m_GraphicStageingBuffer = new GPUBuffer();
                        var descriptionStageingBuffer = description.ToStagingReadback();
                        m_GraphicStageingBuffer.Init(ref descriptionStageingBuffer);
                    }
                    break;
                case Mode.DynamicReadWrite:
                    {
                        m_GraphicStageingBuffer = new GPUBuffer();
                        var descriptionStageingBuffer = description.ToStaging();
                        m_GraphicStageingBuffer.Init(ref descriptionStageingBuffer);
                    }
                    break;
            }
        }


        private bool ChekaAndGetFormat<T>(string fun, ref PixelFormat pf)
        {
            if (RWBufferFromats.TryGetValue(typeof(T), out var f))
            {
                pf = f;
                return true;
            }
            else if (typeof(T).IsSubclassOf(typeof(ValueType)))
            {
                if (m_Type == Type.Structured)
                {
                    pf = PixelFormat.Unknown;
                }
                else if (m_Type != Type.Constant)
                {
                    Debug.LogException(new Exception("Wrong buffer type expected Structured but got " + m_Type.ToString()));
                    return false;
                }
            }
            else
            {
                Debug.LogException(new Exception(string.Format(
                @"
                faled to {1}, Unsuported type passed to {1}<T={1}> only blittable ValueType's are suported
                see:
                https://learn.microsoft.com/en-us/dotnet/framework/interop/blittable-and-non-blittable-types ,
                https://learn.microsoft.com/en-us/dotnet/api/system.valuetype?view=net-9.0"
                , fun
                , typeof(T).Name
                )));
                return false;
            }

            if (pixelFormat != PixelFormat.MAX)
            {
                if (pf != pixelFormat)
                {
                    Debug.LogException(new Exception(string.Format(
                        "faled to {2}, PixelFormat was set before with first funcion call to PixelFormat.{0} but now it got the PixelFormat.{1}"
                        , pixelFormat.ToString()
                        , pf.ToString(), fun
                        )));
                    return false;
                }
            }
            return true;
        }

        internal void BindUA(int slot, GPUContext context)
        {
            if (m_GraphicBuffer != null)
            {
                context.BindUA(slot, m_GraphicBuffer.View());
            }
        }
        internal void BindSR(int slot, GPUContext context)
        {
            if (m_GraphicBuffer != null)
            {
                context.BindSR(slot, m_GraphicBuffer.View());
            }
        }
        internal nint GetDataPointer()
        {
            unsafe
            {
                return new nint(Unsafe.AsPointer(ref m_MemoryBuffer[0]));
            }
        }

        internal GPUBuffer GetGPUBuffer()
        {
            return m_GraphicBuffer;
        }

        ~ComputeBuffer()
        {
            m_GraphicStageingBuffer?.ReleaseGPU();
            m_GraphicBuffer?.ReleaseGPU();
        }

        public void Coppy(ComputeBuffer destination) // to do allow for sub data coppy
        {
            if (destination == null)
            {
                //massage here
                return;
            }
            if (m_GraphicBuffer == null)
            {
                //massage here
                return;
            }
            if (destination.SizeInBytes == SizeInBytes)
            {
                var source = m_GraphicBuffer;
                var dest = destination.GetGPUBuffer();
                GPUDevice.Instance.MainContext.CopyBuffer(dest, source, SizeInBytes);
            }
            else
            {
                //massage here
            }
        }
    }
}
