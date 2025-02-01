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
        //Dictionary of types
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

        /// <summary>
        /// Buffer type
        /// </summary>
        public enum Type
        {
            /// <summary>
            /// Default ComputeBuffer type maps to Buffer&lt;T&gt; / RWBuffer&lt;T&gt; in hlsl.
            /// <br> note: </br>
            /// <br> Buffer and RWBuffer support only 32-bit formats: float, int and uint.</br>
            /// </summary>
            Default,
            /// <summary>
            /// ComputeBuffer that you can use as a structured buffer maps to StructuredBuffer&lt;T&gt; / RWStructuredBuffer&lt;T&gt; in hlsl.
            /// </summary>
            Structured,
            /// <summary>
            /// ComputeBuffer that you can use as a constant buffer(uniform buffer).
            /// </summary>
            Constant,
        }
        /// <summary>
        /// Buffer mode
        /// </summary>
        public enum Mode
        {
            /// <summary>
            /// Static buffer, only initial upload allowed by the CPU
            /// </summary>
            Immutable,
            /// <summary>
            /// Dynamic buffer. can be only Write Only by the CPU
            /// </summary>
            DynamicWriteOnly,
            /// <summary>
            /// Dynamic buffer. can be only Read Only by the CPU
            /// </summary>
            DynamicReadOnly,
            /// <summary>
            /// Dynamic buffer. allows for read and write by the CPU
            /// </summary>
            DynamicReadWrite,
        }
        //--------------------------------------------------------------------

        /// <summary>
        /// The debug label for the compute buffer.
        /// </summary>
        /// <value>
        /// The name.
        /// </value>
        public string Name
        {
            set
            {
#if !BUILD_RELEASE
                if (m_GraphicStageingBuffer != null)
                    m_GraphicStageingBuffer.Name = "ComputeStageingBuffer" + value;
                if (m_GraphicBuffer != null)
                    m_GraphicBuffer.Name = "ComputeBuffer " + value;
#endif
            }
        }
        /// <summary>
        /// Size of one element in the buffer in bytes.
        /// </summary>
        public readonly uint Stride;
        /// <summary>
        /// Number of elements in the buffer.
        /// </summary>
        public readonly uint Count;

        /// <summary>
        /// Gets the size in bytes.
        /// </summary>
        public uint SizeInBytes => Stride * Count;

        /// <summary>
        /// Initializes a new instance of the <see cref="ComputeBuffer"/> class.
        /// </summary>
        /// <param name="count">The count.</param>
        /// <param name="stride">The stride.</param>
        /// <param name="type">The type.</param>
        /// <param name="mode">The mode.</param>
        public ComputeBuffer(int count, int stride, Type type = Type.Default, Mode mode = Mode.Immutable)
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
        /// <summary>
        /// Sets the data.<br></br>
        /// Note:<br></br>
        /// <see langword="sizeof"/>(<typeparamref name="T"/>) must be the same as <see cref="Stride"/><br></br>
        /// and <see cref="Count"/> must be 1
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="data">The data.</param>
        public void SetData<T>(T data)
        {
            if (Count != 1)
            {
                Debug.LogException(new Exception("SetData<T> can't set data buffer was created with more then 1 element or count is 0"));
                return;
            }
            SetData(new T[] {data});
        }
        /// <summary>
        /// Sets the data.<br></br>
        /// Note:<br></br>
        /// <see langword="sizeof"/>(<typeparamref name="T"/>) must be the same as <see cref="Stride"/>
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="data">The data.</param>
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
                return;//we are done data is inside the buffer on cpu, when compute will get dispatched data will be pulled

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

        /// <summary>
        /// Gets the data.
        /// works only on buffers with mode set to <see cref="Mode.DynamicReadOnly"/>,<see cref="Mode.DynamicReadWrite"/>.<br></br>
        /// Note:<br></br>
        /// <see langword="sizeof"/>(<typeparamref name="T"/>) must be the same as <see cref="Stride"/>
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns><see langword="null"/> or <typeparamref name="T"/>[] with size of <see cref="Count"/> </returns>
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
        /// <summary>
        /// Finalizes an instance of the <see cref="ComputeBuffer"/> class.
        /// </summary>
        ~ComputeBuffer()
        {
            m_GraphicStageingBuffer?.ReleaseGPU();
            m_GraphicBuffer?.ReleaseGPU();
        }
        /// <summary>
        /// Coppies <see cref="ComputeBuffer"/> the specified destination.
        /// </summary>
        /// <param name="destination">The destination.</param>
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
