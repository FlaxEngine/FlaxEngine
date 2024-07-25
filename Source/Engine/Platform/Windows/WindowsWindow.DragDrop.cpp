// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "Engine/Platform/Windows/WindowsWindow.h"

#if USE_EDITOR
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Platform/Base/DragDropHelper.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#endif
#include "../Win32/IncludeWindowsHeaders.h"
#include <propidl.h>
#if USE_EDITOR
#include <oleidl.h>
#include <shellapi.h>
#endif

#if USE_EDITOR

Windows::HRESULT Window::QueryInterface(const Windows::IID& id, void** ppvObject)
{
    // Check to see what interface has been requested
    if ((const IID&)id == IID_IUnknown || (const IID&)id == IID_IDropTarget)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    // No interface
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

Windows::ULONG Window::AddRef()
{
    _InterlockedIncrement(&_refCount);
    return _refCount;
}

Windows::ULONG Window::Release()
{
    return _InterlockedDecrement(&_refCount);
}

HGLOBAL duplicateGlobalMem(HGLOBAL hMem)
{
    auto len = GlobalSize(hMem);
    auto source = GlobalLock(hMem);
    auto dest = GlobalAlloc(GMEM_FIXED, len);
    Platform::MemoryCopy(dest, source, len);
    GlobalUnlock(hMem);
    return dest;
}

DWORD dropEffect2OleEnum(DragDropEffect effect)
{
    DWORD result;
    switch (effect)
    {
    case DragDropEffect::None:
        result = DROPEFFECT_NONE;
        break;
    case DragDropEffect::Copy:
        result = DROPEFFECT_COPY;
        break;
    case DragDropEffect::Move:
        result = DROPEFFECT_MOVE;
        break;
    case DragDropEffect::Link:
        result = DROPEFFECT_LINK;
        break;
    default:
        result = DROPEFFECT_NONE;
        break;
    }
    return result;
}

DragDropEffect dropEffectFromOleEnum(DWORD effect)
{
    DragDropEffect result;
    switch (effect)
    {
    case DROPEFFECT_NONE:
        result = DragDropEffect::None;
        break;
    case DROPEFFECT_COPY:
        result = DragDropEffect::Copy;
        break;
    case DROPEFFECT_MOVE:
        result = DragDropEffect::Move;
        break;
    case DROPEFFECT_LINK:
        result = DragDropEffect::Link;
        break;
    default:
        result = DragDropEffect::None;
        break;
    }
    return result;
}

HANDLE StringToHandle(const StringView& str)
{
    // Allocate and lock a global memory buffer.
    // Make it fixed data so we don't have to use GlobalLock
    const int32 length = str.Length();
    char* ptr = static_cast<char*>(GlobalAlloc(GMEM_FIXED, length + 1));

    // Copy the string into the buffer as ANSI text
    StringUtils::ConvertUTF162ANSI(str.Get(), ptr, length);
    ptr[length] = '\0';

    return ptr;
}

void DeepCopyFormatEtc(FORMATETC* dest, FORMATETC* source)
{
    // Copy the source FORMATETC into dest
    *dest = *source;

    if (source->ptd)
    {
        // Allocate memory for the DVTARGETDEVICE if necessary
        dest->ptd = static_cast<DVTARGETDEVICE*>(CoTaskMemAlloc(sizeof(DVTARGETDEVICE)));

        // Copy the contents of the source DVTARGETDEVICE into dest->ptd
        *(dest->ptd) = *(source->ptd);
    }
}

HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC* pFormatEtc, IEnumFORMATETC** ppEnumFormatEtc);

/// <summary>
/// GUI data for Windows platform
/// </summary>
class WindowsGuiData : public IGuiData
{
private:
    Type _type;
    Array<String> _data;

public:
    /// <summary>
    /// Init
    /// </summary>
    WindowsGuiData()
        : _type(Type::Unknown)
        , _data(1)
    {
    }

public:
    /// <summary>
    /// Init from Ole IDataObject
    /// </summary>
    /// <param name="pDataObj">Object</param>
    void Init(IDataObject* pDataObj)
    {
        // Temporary data
        FORMATETC fmtetc;
        STGMEDIUM stgmed;

        // Clear
        _type = Type::Unknown;
        _data.Clear();

        // Check type
        fmtetc = { CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK)
        {
            // Text
            _type = Type::Text;

            // Get data
            char* text = static_cast<char*>(GlobalLock(stgmed.hGlobal));
            _data.Add(String(text));
            GlobalUnlock(stgmed.hGlobal);
            ReleaseStgMedium(&stgmed);
        }
        else
        {
            fmtetc = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK)
            {
                // Unicode Text
                _type = Type::Text;

                // Get data
                Char* text = static_cast<Char*>(GlobalLock(stgmed.hGlobal));
                _data.Add(String(text));
                GlobalUnlock(stgmed.hGlobal);
                ReleaseStgMedium(&stgmed);
            }
            else
            {
                fmtetc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                if (pDataObj->GetData(&fmtetc, &stgmed) == S_OK)
                {
                    // Files
                    _type = Type::Files;

                    // Get data
                    Char item[MAX_PATH];
                    HDROP hdrop = static_cast<HDROP>(GlobalLock(stgmed.hGlobal));
                    UINT filesCount = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
                    for (UINT i = 0; i < filesCount; i++)
                    {
                        if (DragQueryFileW(hdrop, i, item, MAX_PATH) != 0)
                        {
                            _data.Add(String(item));
                        }
                    }
                    GlobalUnlock(stgmed.hGlobal);
                    ReleaseStgMedium(&stgmed);
                }
            }
        }
    }

public:
    // [IGuiData]
    Type GetType() const override
    {
        return _type;
    }
    String GetAsText() const override
    {
        String result;
        if (_type == Type::Text)
        {
            result = _data[0];
        }
        return result;
    }
    void GetAsFiles(Array<String>* files) const override
    {
        if (_type == Type::Files)
        {
            files->Add(_data);
        }
    }
};

/// <summary>
/// Tool class for Windows Ole support
/// </summary>
class WindowsEnumFormatEtc : public IEnumFORMATETC
{
private:
    ULONG _refCount;
    ULONG _index;
    ULONG _formatsCount;
    FORMATETC* _formatEtc;

public:
    WindowsEnumFormatEtc(FORMATETC* pFormatEtc, int32 nNumFormats)
        : _refCount(1)
        , _index(0)
        , _formatsCount(nNumFormats)
        , _formatEtc(nullptr)
    {
        // Allocate memory
        _formatEtc = new FORMATETC[nNumFormats];

        // Copy the FORMATETC structures
        for (int32 i = 0; i < nNumFormats; i++)
        {
            DeepCopyFormatEtc(&_formatEtc[i], &pFormatEtc[i]);
        }
    }

    ~WindowsEnumFormatEtc()
    {
        if (_formatEtc)
        {
            for (uint32 i = 0; i < _formatsCount; i++)
            {
                if (_formatEtc[i].ptd)
                {
                    CoTaskMemFree(_formatEtc[i].ptd);
                }
            }

            delete[] _formatEtc;
        }
    }

public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
    {
        // Check to see what interface has been requested
        if (riid == IID_IEnumFORMATETC || riid == IID_IUnknown)
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }

        // No interface
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        _InterlockedIncrement(&_refCount);
        return _refCount;
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRefCount = _InterlockedDecrement(&_refCount);
        if (_refCount == 0)
        {
            delete this;
        }
        return ulRefCount;
    }

    // [IEnumFormatEtc]
    HRESULT STDMETHODCALLTYPE Next(ULONG celt, FORMATETC* pFormatEtc, ULONG* pceltFetched) override
    {
        ULONG copied = 0;

        // validate arguments
        if (celt == 0 || pFormatEtc == nullptr)
            return E_INVALIDARG;

        // copy FORMATETC structures into caller's buffer
        while (_index < _formatsCount && copied < celt)
        {
            DeepCopyFormatEtc(&pFormatEtc[copied], &_formatEtc[_index]);
            copied++;
            _index++;
        }

        // store result
        if (pceltFetched != nullptr)
            *pceltFetched = copied;

        // did we copy all that was requested?
        return (copied == celt) ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override
    {
        _index += celt;
        return (_index <= _formatsCount) ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Reset() override
    {
        _index = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC** ppEnumFormatEtc) override
    {
        HRESULT result;

        // Make a duplicate enumerator
        result = CreateEnumFormatEtc(_formatsCount, _formatEtc, ppEnumFormatEtc);

        if (result == S_OK)
        {
            // Manually set the index state
            static_cast<WindowsEnumFormatEtc*>(*ppEnumFormatEtc)->_index = _index;
        }

        return result;
    }
};

HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC* pFormatEtc, IEnumFORMATETC** ppEnumFormatEtc)
{
    if (nNumFormats == 0 || pFormatEtc == nullptr || ppEnumFormatEtc == nullptr)
        return E_INVALIDARG;
    *ppEnumFormatEtc = new WindowsEnumFormatEtc(pFormatEtc, nNumFormats);
    return *ppEnumFormatEtc ? S_OK : E_OUTOFMEMORY;
}

/// <summary>
/// Drag drop source and data container for Ole
/// </summary>
class WindowsDragSource : public IDataObject, public IDropSource
{
private:
    ULONG _refCount;
    int32 _formatsCount;
    FORMATETC* _formatEtc;
    STGMEDIUM* _stgMedium;

public:
    WindowsDragSource(FORMATETC* fmtetc, STGMEDIUM* stgmed, int32 count)
        : _refCount(1)
        , _formatsCount(count)
        , _formatEtc(nullptr)
        , _stgMedium(nullptr)
    {
        // Allocate memory
        _formatEtc = new FORMATETC[count];
        _stgMedium = new STGMEDIUM[count];

        // Copy descriptors
        for (int32 i = 0; i < count; i++)
        {
            _formatEtc[i] = fmtetc[i];
            _stgMedium[i] = stgmed[i];
        }
    }

    virtual ~WindowsDragSource()
    {
        delete[] _formatEtc;
        delete[] _stgMedium;
    }

public:
    // [IUnknown]
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
    {
        // Check to see what interface has been requested
        if (riid == IID_IDataObject || riid == IID_IUnknown || riid == IID_IDropSource)
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }

        // No interface
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        _InterlockedIncrement(&_refCount);
        return _refCount;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRefCount = _InterlockedDecrement(&_refCount);
        if (_refCount == 0)
        {
            delete this;
        }
        return ulRefCount;
    }

    // [IDropSource]
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(_In_ BOOL fEscapePressed, _In_ DWORD grfKeyState) override
    {
        // If the Escape key has been pressed since the last call, cancel the drop
        if (fEscapePressed == TRUE || grfKeyState & MK_RBUTTON)
            return DRAGDROP_S_CANCEL;

        // If the LeftMouse button has been released, then do the drop!
        if ((grfKeyState & MK_LBUTTON) == 0)
            return DRAGDROP_S_DROP;

        // Continue with the drag-drop
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GiveFeedback(_In_ DWORD dwEffect) override
    {
        // TODO: allow to use custom mouse cursor during drop and drag operation
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

    // [IDataObject]
    HRESULT STDMETHODCALLTYPE GetData(_In_ FORMATETC* pformatetcIn, _Out_ STGMEDIUM* pmedium) override
    {
        if (pformatetcIn == nullptr || pmedium == nullptr)
            return E_INVALIDARG;

        // Try to match the specified FORMATETC with one of our supported formats
        int32 index = lookupFormatEtc(pformatetcIn);
        if (index == INVALID_INDEX)
            return DV_E_FORMATETC;

        // Found a match - transfer data into supplied storage medium
        pmedium->tymed = _formatEtc[index].tymed;
        pmedium->pUnkForRelease = nullptr;

        // Copy the data into the caller's storage medium
        switch (_formatEtc[index].tymed)
        {
        case TYMED_HGLOBAL:
            pmedium->hGlobal = duplicateGlobalMem(_stgMedium[index].hGlobal);
            break;

        default:
            return DV_E_FORMATETC;
        }

        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDataHere(_In_ FORMATETC* pformatetc, _Inout_ STGMEDIUM* pmedium) override
    {
        return DATA_E_FORMATETC;
    }
    HRESULT STDMETHODCALLTYPE QueryGetData(__RPC__in_opt FORMATETC* pformatetc) override
    {
        return lookupFormatEtc(pformatetc) == INVALID_INDEX ? DV_E_FORMATETC : S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(__RPC__in_opt FORMATETC* pformatectIn, __RPC__out FORMATETC* pformatetcOut) override
    {
        // Apparently we have to set this field to NULL even though we don't do anything else
        pformatetcOut->ptd = nullptr;
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetData(_In_ FORMATETC* pformatetc, _In_ STGMEDIUM* pmedium, BOOL fRelease) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, __RPC__deref_out_opt IEnumFORMATETC** ppenumFormatEtc) override
    {
        // Only the get direction is supported for OLE
        if (dwDirection == DATADIR_GET)
        {
            // TODO: use SHCreateStdEnumFmtEtc API call
            return CreateEnumFormatEtc(_formatsCount, _formatEtc, ppenumFormatEtc);
        }

        // The direction specified is not supported for drag+drop
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE DAdvise(__RPC__in FORMATETC* pformatetc, DWORD advf, __RPC__in_opt IAdviseSink* pAdvSink, __RPC__out DWORD* pdwConnection) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumDAdvise(__RPC__deref_out_opt IEnumSTATDATA** ppenumAdvise) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

private:
    int32 lookupFormatEtc(FORMATETC* pFormatEtc) const
    {
        // Check each of our formats in turn to see if one matches
        for (int32 i = 0; i < _formatsCount; i++)
        {
            if ((_formatEtc[i].tymed & pFormatEtc->tymed) &&
                _formatEtc[i].cfFormat == pFormatEtc->cfFormat &&
                _formatEtc[i].dwAspect == pFormatEtc->dwAspect)
            {
                // Return index of stored format
                return i;
            }
        }

        // Format not found
        return INVALID_INDEX;
    }
};

WindowsGuiData GuiDragDropData;

DragDropEffect Window::DoDragDrop(const StringView& data)
{
    // Create background worker that will keep updating GUI (perform rendering)
    const auto task = New<DoDragDropJob>();
    Task::StartNew(task);
    while (task->GetState() == TaskState::Queued)
        Platform::Sleep(1);

    // Create descriptors
    FORMATETC fmtetc = { CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgmed = { TYMED_HGLOBAL, { nullptr }, nullptr };

    // Create a HGLOBAL inside the storage medium
    stgmed.hGlobal = StringToHandle(data);

    // Create drop source
    auto dropSource = new WindowsDragSource(&fmtetc, &stgmed, 1);

    // Do the drag drop operation
    DWORD dwEffect;
    HRESULT result = ::DoDragDrop(dropSource, dropSource, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK | DROPEFFECT_SCROLL, &dwEffect);

    // Wait for job end
    Platform::AtomicStore(&task->ExitFlag, 1);
    task->Wait();

    // Release allocated data
    dropSource->Release();
    ReleaseStgMedium(&stgmed);

    // Fix hanging mouse state (Windows doesn't send WM_LBUTTONUP when we end the drag and drop)
    if (Input::GetMouseButton(MouseButton::Left))
    {
        ::POINT point;
        ::GetCursorPos(&point);
        Input::Mouse->OnMouseUp(Float2((float)point.x, (float)point.y), MouseButton::Left, this);
    }

    return SUCCEEDED(result) ? dropEffectFromOleEnum(dwEffect) : DragDropEffect::None;
}

HRESULT Window::DragEnter(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect)
{
    POINT p = { pt.x, pt.y };
    ::ScreenToClient((HWND)_handle, &p);
    GuiDragDropData.Init((IDataObject*)pDataObj);
    DragDropEffect effect = DragDropEffect::None;
    OnDragEnter(&GuiDragDropData, Float2(static_cast<float>(p.x), static_cast<float>(p.y)), effect);
    Focus();
    *pdwEffect = dropEffect2OleEnum(effect);
    return S_OK;
}

HRESULT Window::DragOver(Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect)
{
    POINT p = { pt.x, pt.y };
    ::ScreenToClient((HWND)_handle, &p);
    DragDropEffect effect = DragDropEffect::None;
    OnDragOver(&GuiDragDropData, Float2(static_cast<float>(p.x), static_cast<float>(p.y)), effect);
    *pdwEffect = dropEffect2OleEnum(effect);
    return S_OK;
}

HRESULT Window::DragLeave()
{
    OnDragLeave();
    return S_OK;
}

HRESULT Window::Drop(Windows::IDataObject* pDataObj, Windows::DWORD grfKeyState, Windows::POINTL pt, Windows::DWORD* pdwEffect)
{
    POINT p = { pt.x, pt.y };
    ::ScreenToClient((HWND)_handle, &p);
    GuiDragDropData.Init((IDataObject*)pDataObj);
    DragDropEffect effect = DragDropEffect::None;
    OnDragDrop(&GuiDragDropData, Float2(static_cast<float>(p.x), static_cast<float>(p.y)), effect);
    *pdwEffect = dropEffect2OleEnum(effect);
    return S_OK;
}

#else

DragDropEffect Window::DoDragDrop(const StringView& data)
{
    return DragDropEffect::None;
}

#endif

#endif
