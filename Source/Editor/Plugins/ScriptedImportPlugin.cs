using FlaxEditor;
using FlaxEditor.Content;

namespace FlaxEditor.Plugins
{
    /// <summary>
    /// A plugin that allows you to script how assets are imported.
    /// </summary>
    public class ScriptedImportPlugin : EditorPlugin
    {
        /// <inheritdoc />
        public override void Initialize()
        {
            base.Initialize();
            Editor.ContentImporting.OnAssetImport += OnAssetImport;
            Editor.ContentImporting.OnAssetImportComplete += OnAssetImportComplete;
        }

        /// <inheritdoc />
        public override void Deinitialize()
        {
            Editor.ContentImporting.OnAssetImportComplete -= OnAssetImportComplete;
            Editor.ContentImporting.OnAssetImport -= OnAssetImport;
            base.Deinitialize();
        }

        /// <summary>
        /// Called when the importing of any asset is started, before the user is shown any dialogs.
        /// </summary>
        /// <param name="importContext">The current state of the import context as it is modified.</param>
        public virtual void OnAssetImport(FlaxEditor.Modules.ContentImportingModule.ImportModificationsInfo importContext)
        {}

        /// <summary>
        /// Called when the importing of any asset is completed. This includes when you handle OnAssetImport and want to do something later.
        /// </summary>
        /// <param name="importedFile">The entry for the file that was imported.</param>
        public virtual void OnAssetImportComplete(IFileEntryAction importedFile)
        {}
    }
}
