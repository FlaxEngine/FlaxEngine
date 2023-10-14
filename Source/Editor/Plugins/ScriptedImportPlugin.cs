using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using System.Diagnostics;

namespace FlaxEditor
{
    /// <summary>
    /// A plugin that allows you to script how assets are imported.
    /// </summary>
    public class ScriptedImportPlugin : EditorPlugin
    {
        /// <inheritdoc />
        public override void InitializeEditor()
        {
            base.InitializeEditor();
            Editor.ContentImporting.OnAssetImport += OnAssetImport;
            Editor.ContentImporting.OnAssetImportComplete += OnAssetImportComplete;
        }

        /// <inheritdoc />
        public override void DeinitializeEditor()
        {
            Editor.ContentImporting.OnAssetImportComplete -= OnAssetImportComplete;
            Editor.ContentImporting.OnAssetImport -= OnAssetImport;
            base.DeinitializeEditor();
        }
        
        private bool EnsureImportSettings<T>(Modules.ContentImportingModule.ImportSettingsContext importContext) where T : new()
        {
            if (importContext.Settings == null)
            {
                importContext.Settings = new T();
            }

            if (importContext.Settings is not T)
            {
                Editor.LogWarning($"OnAssetImport detected mismatch between " +
                        $"{typeof(T).Name} and {importContext.Settings.GetType()} while importing.");
                return false;
            }

            return true;
        }

        /// <summary>
        /// Called when the importing of any asset is started, before the user is shown any dialogs.
        /// Make sure to call the base method when overriding this.
        /// </summary>
        /// <param name="importContext">The current state of the import context as it is modified.</param>
        public virtual void OnAssetImport(Modules.ContentImportingModule.ImportSettingsContext importContext)
        {
            if (importContext.FileEntry is ModelImportEntry)
            {
                if (EnsureImportSettings<ModelImportSettings>(importContext))
                {
                    OnModelImport(importContext);
                }
            }
            else if (importContext.FileEntry is TextureImportEntry)
            {
                if (EnsureImportSettings<TextureImportSettings>(importContext))
                {
                    OnTextureImport(importContext);
                }
            }
        }

        /// <summary>
        /// Called when the importing of any asset is completed. This includes when you handle OnAssetImport and want to do something later.
        /// Make sure to call the base method when overriding this.
        /// </summary>
        /// <param name="importedFile">The entry for the file that was imported.</param>
        public virtual void OnAssetImportComplete(IFileEntryAction importedFile)
        {}

        /// <summary>
        /// Called when the importing of any model asset begins.
        /// </summary>
        /// <param name="importContext">The current import context. The Settings field can be assumed to be non-null and an instance of ModelImportSettings.</param>
        public virtual void OnModelImport(Modules.ContentImportingModule.ImportSettingsContext importContext)
        {}

        /// <summary>
        /// Called when the importing of any texture asset begins.
        /// </summary>
        /// <param name="importContext">The current import context. The Settings field can be assumed to be non-null and an instance of TextureImportSettings.</param>
        public virtual void OnTextureImport(Modules.ContentImportingModule.ImportSettingsContext importContext)
        {}
    }
}
