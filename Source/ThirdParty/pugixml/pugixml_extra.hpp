// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "pugixml.hpp"

namespace pugi
{
    class xml_node_extra : public xml_node
    {
    public:
        xml_node_extra() = default;
        xml_node_extra(xml_node child);

        xml_node_extra child_or_append(const char_t* name_);
        bool set_child_value(const char_t* rhs);
        void append_child_with_value(const char_t* name_, const char_t* rhs);

    };
}
