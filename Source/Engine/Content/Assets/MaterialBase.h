// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Graphics/Materials/IMaterial.h"
#include "Engine/Graphics/Materials/MaterialParams.h"

/// <summary>
/// Base class for <see cref="Material"/> and <see cref="MaterialInstance"/>.
/// </summary>
/// <seealso cref="FlaxEngine.BinaryAsset" />
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API MaterialBase : public BinaryAsset, public IMaterial
{
    DECLARE_ASSET_HEADER(MaterialBase);
public:
    /// <summary>
    /// The material parameters collection.
    /// </summary>
    MaterialParams Params;

    /// <summary>
    /// Event called when parameters collections gets modified.
    /// </summary>
    Action ParamsChanged;

    /// <summary>
    /// Returns true if material is a material instance.
    /// </summary>
    virtual bool IsMaterialInstance() const = 0;

public:
    /// <summary>
    /// Gets the material parameters collection.
    /// </summary>
    API_PROPERTY() const Array<MaterialParameter>& GetParameters() const
    {
        return Params;
    }

    /// <summary>
    /// Gets the material info, structure which describes material surface.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const MaterialInfo& Info() const
    {
        return GetInfo();
    }

    /// <summary>
    /// Gets the material parameter.
    /// </summary>
    API_FUNCTION() FORCE_INLINE MaterialParameter* GetParameter(const StringView& name)
    {
        return Params.Get(name);
    }

    /// <summary>
    /// Gets the material parameter value.
    /// </summary>
    /// <returns>The parameter value.</returns>
    API_FUNCTION() Variant GetParameterValue(const StringView& name);

    /// <summary>
    /// Sets the material parameter value (and sets IsOverride to true).
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <param name="value">The value to set.</param>
    /// <param name="warnIfMissing">True to warn if parameter is missing, otherwise will do nothing.</param>
    API_FUNCTION() void SetParameterValue(const StringView& name, const Variant& value, bool warnIfMissing = true);

    /// <summary>
    /// Creates the virtual material instance of this material which allows to override any material parameters.
    /// </summary>
    /// <returns>The created virtual material instance asset.</returns>
    API_FUNCTION() MaterialInstance* CreateVirtualInstance();

public:
    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
#endif
};
