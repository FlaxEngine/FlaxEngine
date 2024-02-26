// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Threading;
using System.Threading.Tasks;

namespace FlaxEngine.Utilities
{
    /// <summary>
    /// Helper utility for caching model meshes data.
    /// </summary>
    [HideInEditor]
    public class MeshDataCache
    {
        /// <summary>
        /// The single mesh data container.
        /// </summary>
        public struct MeshData
        {
            /// <summary>
            /// The index buffer.
            /// </summary>
            public uint[] IndexBuffer;

            /// <summary>
            /// The vertex buffer.
            /// </summary>
            public Mesh.Vertex[] VertexBuffer;
        }

        private Model _model;
        private MeshData[][] _meshDatas;
        private bool _meshDatasInProgress;
        private bool _meshDatasCancel;

        /// <summary>
        /// Gets the mesh datas (null if during downloading).
        /// </summary>
        public MeshData[][] MeshDatas => _meshDatasInProgress ? null : _meshDatas;

        /// <summary>
        /// Occurs when mesh data gets downloaded (called on async thread).
        /// </summary>
        public event Action Finished;

        /// <summary>
        /// Requests the mesh data.
        /// </summary>
        /// <param name="model">The model to get it's data.</param>
        /// <returns>True if has valid data to access, otherwise false if it's during downloading.</returns>
        public bool RequestMeshData(Model model)
        {
            if (model == null)
                throw new ArgumentNullException();
            if (_model != model)
            {
                // Mode changes so release previous cache
                Dispose();
            }
            if (_meshDatasInProgress)
            {
                // Still downloading
                return false;
            }
            if (_meshDatas != null)
            {
                // Has valid data
                return true;
            }

            // Start downloading
            _model = model;
            _meshDatasInProgress = true;
            _meshDatasCancel = false;
            Task.Run(new Action(DownloadMeshData));
            return false;
        }

        /// <summary>
        /// Releases cache.
        /// </summary>
        public void Dispose()
        {
            WaitForMeshDataRequestEnd();
            _meshDatas = null;
            _meshDatasInProgress = false;
        }

        /// <summary>
        /// Waits for mesh data request to end. Does nothing if already has valid data or no valid request pending.
        /// </summary>
        public void WaitForMeshDataRequestEnd()
        {
            if (_meshDatasInProgress)
            {
                _meshDatasCancel = true;
                for (int i = 0; i < 500 && _meshDatasInProgress; i++)
                    Thread.Sleep(10);
            }
        }

        private void DownloadMeshData()
        {
            var success = false;

            try
            {
                if (!_model)
                    return;
                if (_model.WaitForLoaded())
                    throw new Exception("WaitForLoaded failed");

                var lods = _model.LODs;
                _meshDatas = new MeshData[lods.Length][];

                for (int lodIndex = 0; lodIndex < lods.Length && !_meshDatasCancel; lodIndex++)
                {
                    var lod = lods[lodIndex];
                    var meshes = lod.Meshes;
                    _meshDatas[lodIndex] = new MeshData[meshes.Length];

                    for (int meshIndex = 0; meshIndex < meshes.Length && !_meshDatasCancel; meshIndex++)
                    {
                        var mesh = meshes[meshIndex];
                        _meshDatas[lodIndex][meshIndex] = new MeshData
                        {
                            IndexBuffer = mesh.DownloadIndexBuffer(),
                            VertexBuffer = mesh.DownloadVertexBuffer()
                        };
                    }
                }
                success = true;
            }
            catch (Exception ex)
            {
                Debug.Logger.LogHandler.LogWrite(LogType.Warning, "Failed to get mesh data.");
                Debug.Logger.LogHandler.LogWrite(LogType.Warning, _model?.ToString() ?? string.Empty);
                Debug.Logger.LogHandler.LogException(ex, null);
            }
            finally
            {
                _meshDatasInProgress = false;

                if (success)
                {
                    Finished?.Invoke();
                }
            }
        }
    }
}
