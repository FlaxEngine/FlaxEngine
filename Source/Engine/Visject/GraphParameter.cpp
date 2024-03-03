// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GraphParameter.h"
#include "Engine/Core/Types/DataContainer.h"

BytesContainer GraphParameter::GetMetaData(int32 typeID) const
{
    BytesContainer result;
    for (const auto& e : Meta.Entries)
    {
        if (e.TypeID == typeID)
        {
            result.Link(e.Data);
            break;
        }
    }
    return result;
}
