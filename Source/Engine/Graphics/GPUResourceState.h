// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Tracking of per-resource or per-subresource state for GPU resources that require to issue resource access barriers during rendering.
/// </summary>
template<typename StateType, StateType InvalidState>
class GPUResourceState
{
private:
    /// <summary>
    /// The whole resource state (used only if _allSubresourcesSame is 1).
    /// </summary>
    StateType _resourceState : 31;

    /// <summary>
    /// Set to 1 if _resourceState is valid. In this case, all subresources have the same state.
    /// Set to 0 if _subresourceState is valid. In this case, each subresources may have a different state (or may be unknown).
    /// </summary>
    uint32 _allSubresourcesSame : 1;

    /// <summary>
    /// The per subresource state (used only if _allSubresourcesSame is 0).
    /// </summary>
    Array<StateType> _subresourceState;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceState"/> class.
    /// </summary>
    GPUResourceState()
        : _resourceState(InvalidState)
        , _allSubresourcesSame(true)
    {
    }

public:
    void Initialize(uint32 subresourceCount, StateType initialState, bool usePerSubresourceTracking)
    {
        ASSERT(_subresourceState.IsEmpty() && subresourceCount > 0);
        _allSubresourcesSame = true;
        _resourceState = initialState;
        if (usePerSubresourceTracking && subresourceCount > 1)
            _subresourceState.Resize(subresourceCount, false);
#if BUILD_DEBUG
        _subresourceState.SetAll(InvalidState);
#endif
    }

    bool IsInitializated() const
    {
        return _resourceState != InvalidState || _subresourceState.HasItems();
    }

    void Release()
    {
        _resourceState = InvalidState;
        _subresourceState.Resize(0);
    }

    bool AreAllSubresourcesSame() const
    {
        return _allSubresourcesSame;
    }

    int32 GetSubresourcesCount() const
    {
        return _subresourceState.Count();
    }

    bool CheckResourceState(StateType state) const
    {
        if (_allSubresourcesSame)
            return state == _resourceState;
        for (int32 i = 0; i < _subresourceState.Count(); i++)
        {
            if (_subresourceState[i] != state)
                return false;
        }
        return true;
    }

    StateType GetSubresourceState(uint32 subresourceIndex) const
    {
        if (_allSubresourcesSame)
            return _resourceState;
        ASSERT(subresourceIndex >= 0 && subresourceIndex < static_cast<uint32>(_subresourceState.Count()));
        return _subresourceState[subresourceIndex];
    }

    void SetResourceState(StateType state)
    {
        _allSubresourcesSame = 1;
        _resourceState = state;
#if BUILD_DEBUG
        for (int32 i = 0; i < _subresourceState.Count(); i++)
            _subresourceState[i] = InvalidState;
#endif
    }

    void SetSubresourceState(int32 subresourceIndex, StateType state)
    {
        // Check if use single state for the whole resource
        if (subresourceIndex == -1 || _subresourceState.Count() <= 1)
        {
            SetResourceState(state);
        }
        else
        {
            ASSERT(subresourceIndex < static_cast<int32>(_subresourceState.Count()));

            // Transition for all sub-resources
            if (_allSubresourcesSame)
            {
                for (int32 i = 0; i < _subresourceState.Count(); i++)
                    _subresourceState[i] = _resourceState;
                _allSubresourcesSame = 0;
#if BUILD_DEBUG
                _resourceState = InvalidState;
#endif
            }

            _subresourceState[subresourceIndex] = state;
        }
    }
};
