using FlaxEditor;
using FlaxEditor.Content;
using static FlaxEditor.Modules.ContentImportingModule;

namespace FlaxEngine.Editor.Plugins
{
    public class ScriptedImportPlugin : EditorPlugin
    {
        public override void Initialize()
        {
            base.Initialize();
            Editor.ContentImporting.OnAssetImport += OnAssetImport;
            Editor.ContentImporting.OnAssetImportComplete += OnAssetImportComplete;
        }

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
        public virtual void OnAssetImport(ImportModificationsInfo importContext)
        {}

        /// <summary>
        /// Called when the importing of any asset is completed. This includes when you handle OnAssetImport and want to do something later.
        /// </summary>
        /// <param name="importedFile">The entry for the file that was imported.</param>
        public virtual void OnAssetImportComplete(IFileEntryAction importedFile)
        {}
    }
}
