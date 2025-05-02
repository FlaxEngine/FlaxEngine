// Copyright (c) Wojciech Figat. All rights reserved.

#include "pugixml.hpp"
#include "pugixml_extra.hpp"

namespace pugi
{
    // Compare two strings
    // This comes from pugi::impl::strequal
    bool strequal(const char_t* src, const char_t* dst)
    {
#ifdef PUGIXML_WCHAR_MODE
        return wcscmp(src, dst) == 0;
#else
        return strcmp(src, dst) == 0;
#endif
    }

    // This comes from pugi::impl::is_text_node
    inline bool is_text_node(const xml_node& node)
    {
        xml_node_type type = node.type();

        return type == node_pcdata || type == node_cdata;
    }

    xml_node_extra::xml_node_extra(xml_node child) : xml_node(child)
    {
    }

    xml_node_extra xml_node_extra::child_or_append(const char_t* name_)
    {
        if (!name_ || !_root) return xml_node_extra();

        for (xml_node& child : *this)
        {
            const auto *name = child.name();
            if (name && strequal(name_, name)) return xml_node_extra(child);
        }

        return xml_node_extra(append_child(name_));
    }

    bool xml_node_extra::set_child_value(const char_t* rhs)
    {
        if (!_root) return xml_node();

        for (xml_node& child : *this)
        {
            if (child.value() && is_text_node(child))
            {
                return child.set_value(rhs);
            }
        }

        return append_child(node_pcdata).set_value(rhs);
    }

    void xml_node_extra::append_child_with_value(const char_t* name_, const char_t* rhs)
    {
        xml_node_extra child = xml_node_extra(append_child(name_));
        child.append_child(node_pcdata).set_value(rhs);
    }
}
