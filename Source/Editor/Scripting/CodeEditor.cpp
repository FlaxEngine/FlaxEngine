// Copyright (c) Wojciech Figat. All rights reserved.

#include "CodeEditor.h"
#include "CodeEditors/SystemDefaultCodeEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "ScriptsBuilder.h"
#include "CodeEditors/VisualStudioCodeEditor.h"
#include "CodeEditors/RiderCodeEditor.h"
#if USE_VISUAL_STUDIO_DTE
#include "CodeEditors/VisualStudio/VisualStudioEditor.h"
#endif
#include "Engine/Core/Log.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Threading/IRunnable.h"

void OnAsyncBegin(Thread* thread);
void OnAsyncEnd();

class AsyncOpenTask : public IRunnable
{
private:

    bool _isSolutionOpenTask;
    String _path;
    int32 _line;
    CodeEditor* _editor;

public:

    AsyncOpenTask(const String& path, int32 line, CodeEditor* editor)
        : _isSolutionOpenTask(false)
        , _path(path)
        , _line(line)
        , _editor(editor)
    {
    }

    AsyncOpenTask(CodeEditor* editor)
        : _isSolutionOpenTask(true)
        , _line(0)
        , _editor(editor)
    {
    }

    static void OpenSolution(CodeEditor* editor)
    {
        auto task = New<AsyncOpenTask>(editor);
        auto thread = Thread::Create(task, task->ToString(), ThreadPriority::BelowNormal);
        if (thread == nullptr)
        {
            Delete(task);
            LOG(Error, "Failed to start a thread to open solution");
            return;
        }
        OnAsyncBegin(thread);
    }

    static void OpenFile(CodeEditor* editor, const String& filePath, int32 line)
    {
        auto task = New<AsyncOpenTask>(filePath, line, editor);
        auto thread = Thread::Create(task, task->ToString(), ThreadPriority::BelowNormal);
        if (thread == nullptr)
        {
            Delete(task);
            LOG(Error, "Failed to start a thread to open file");
            return;
        }
        OnAsyncBegin(thread);
    }

public:

    // [IRunnable]
    String ToString() const override
    {
        return TEXT("Code Editor open");
    }

    int32 Run() override
    {
        if (_isSolutionOpenTask)
        {
            _editor->OpenSolution();
        }
        else
        {
            _editor->OpenFile(_path, _line);
        }
        return 0;
    }

    void AfterWork(bool wasKilled) override
    {
        OnAsyncEnd();
        Delete(this);
    }
};

class CodeEditingManagerService : public EngineService
{
public:

    CodeEditingManagerService()
        : EngineService(TEXT("Code Editing Manager"))
    {
    }

    bool Init() override;
    void Dispose() override;
};

CodeEditingManagerService CodeEditingManagerServiceInstance;

Array<CodeEditor*> CodeEditors;
Thread* AsyncOpenThread = nullptr;

Action CodeEditingManager::AsyncOpenBegin;
Action CodeEditingManager::AsyncOpenEnd;

const Array<CodeEditor*>& CodeEditingManager::GetEditors()
{
    return CodeEditors;
}

bool CodeEditingManager::IsAsyncOpenRunning()
{
    return AsyncOpenThread != nullptr;
}

CodeEditor* CodeEditingManager::GetCodeEditor(CodeEditorTypes editorType)
{
    for (int32 i = 0; i < CodeEditors.Count(); i++)
    {
        if (CodeEditors[i]->GetType() == editorType)
            return CodeEditors[i];
    }
    return nullptr;
}

void CodeEditingManager::OpenFile(CodeEditorTypes editorType, const String& path, int32 line)
{
    const auto editor = GetCodeEditor(editorType);
    if (editor)
    {
        OpenFile(editor, path, line);
    }
    else
    {
        LOG(Warning, "Missing code editor type {0}", (int32)editorType);
    }
}

void CodeEditingManager::OpenFile(CodeEditor* editor, const String& path, int32 line)
{
    // Ensure that file exists
    if (!FileSystem::FileExists(path))
        return;

    // Ensure that no async task is running
    if (IsAsyncOpenRunning())
    {
        // TODO: enqueue action and handle many actions in the queue
        LOG(Warning, "Cannot use code editor during async open action.");
        return;
    }

    if (editor->UseAsyncForOpen())
    {
        AsyncOpenTask::OpenFile(editor, path, line);
    }
    else
    {
        editor->OpenFile(path, line);
    }
}

void CodeEditingManager::OpenSolution(CodeEditorTypes editorType)
{
    const auto editor = GetCodeEditor(editorType);
    if (editor)
    {
        // Ensure that no async task is running
        if (IsAsyncOpenRunning())
        {
            // TODO: enqueue action and handle many actions in the queue
            LOG(Warning, "Cannot use code editor during async open action.");
            return;
        }

        if (editor->UseAsyncForOpen())
        {
            AsyncOpenTask::OpenSolution(editor);
        }
        else
        {
            editor->OpenSolution();
        }
    }
    else
    {
        LOG(Warning, "Missing code editor type {0}", (int32)editorType);
    }
}

void CodeEditingManager::OnFileAdded(CodeEditorTypes editorType, const String& path)
{
    const auto editor = GetCodeEditor(editorType);
    if (editor)
    {
        editor->OnFileAdded(path);
    }
    else
    {
        LOG(Warning, "Missing code editor type {0}", (int32)editorType);
    }
}

void OnAsyncBegin(Thread* thread)
{
    ASSERT(AsyncOpenThread == nullptr);
    AsyncOpenThread = thread;
    CodeEditingManager::AsyncOpenBegin();
}

void OnAsyncEnd()
{
    ASSERT(AsyncOpenThread != nullptr);
    AsyncOpenThread = nullptr;
    CodeEditingManager::AsyncOpenEnd();
}

bool CodeEditingManagerService::Init()
{
    // Try get editors
#if USE_VISUAL_STUDIO_DTE
    VisualStudioEditor::FindEditors(&CodeEditors);
#endif
    VisualStudioCodeEditor::FindEditors(&CodeEditors);
    RiderCodeEditor::FindEditors(&CodeEditors);
    CodeEditors.Add(New<SystemDefaultCodeEditor>());

    return false;
}

void CodeEditingManagerService::Dispose()
{
    // Stop async task
    if (CodeEditingManager::IsAsyncOpenRunning())
    {
        AsyncOpenThread->Kill(true);
    }

    // Cleanup
    CodeEditors.ClearDelete();
}
