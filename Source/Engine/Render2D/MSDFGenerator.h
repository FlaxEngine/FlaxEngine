// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include <ThirdParty/freetype/ftoutln.h>
#include <ThirdParty/msdfgen/msdfgen.h>
#include <ThirdParty/msdfgen/msdfgen/core/ShapeDistanceFinder.h>

class MSDFGenerator
{
    using Point2 = msdfgen::Point2;
    using Shape = msdfgen::Shape;
    using Contour = msdfgen::Contour;
    using EdgeHolder = msdfgen::EdgeHolder;

    static Point2 ftPoint2(const FT_Vector& vector, double scale)
    {
        return Point2(scale * vector.x, scale * vector.y);
    }

    struct FtContext
    {
        double scale;
        Point2 position;
        Shape* shape;
        Contour* contour;
    };

    static int ftMoveTo(const FT_Vector* to, void* user)
    {
        FtContext* context = reinterpret_cast<FtContext*>(user);
        if (!(context->contour && context->contour->edges.empty()))
            context->contour = &context->shape->addContour();
        context->position = ftPoint2(*to, context->scale);
        return 0;
    }

    static int ftLineTo(const FT_Vector* to, void* user)
    {
        FtContext* context = reinterpret_cast<FtContext*>(user);
        Point2 endpoint = ftPoint2(*to, context->scale);
        if (endpoint != context->position)
        {
            context->contour->addEdge(EdgeHolder(context->position, endpoint));
            context->position = endpoint;
        }
        return 0;
    }

    static int ftConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
    {
        FtContext* context = reinterpret_cast<FtContext*>(user);
        Point2 endpoint = ftPoint2(*to, context->scale);
        if (endpoint != context->position)
        {
            context->contour->addEdge(EdgeHolder(context->position, ftPoint2(*control, context->scale), endpoint));
            context->position = endpoint;
        }
        return 0;
    }

    static int ftCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
    {
        FtContext* context = reinterpret_cast<FtContext*>(user);
        Point2 endpoint = ftPoint2(*to, context->scale);
        if (endpoint != context->position || msdfgen::crossProduct(ftPoint2(*control1, context->scale) - endpoint, ftPoint2(*control2, context->scale) - endpoint))
        {
            context->contour->addEdge(EdgeHolder(context->position, ftPoint2(*control1, context->scale), ftPoint2(*control2, context->scale), endpoint));
            context->position = endpoint;
        }
        return 0;
    }

    static void correctWinding(Shape& shape, Shape::Bounds& bounds)
    {
        Point2 p(bounds.l - (bounds.r - bounds.l) - 1, bounds.b - (bounds.t - bounds.b) - 1);
        double distance = msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(shape, p);
        if (distance > 0)
        {
            for (auto& contour : shape.contours)
                contour.reverse();
        }
    }

public:
    static void GenerateMSDF(FT_GlyphSlot glyph, Array<byte>& output, int32& outputWidth, int32& outputHeight, int16& top, int16& left)
    {
        Shape shape;
        shape.contours.clear();
        shape.inverseYAxis = true;

        FtContext context = { };
        context.scale = 1.0 / 64.0;
        context.shape = &shape;
        FT_Outline_Funcs ftFunctions;
        ftFunctions.move_to = &ftMoveTo;
        ftFunctions.line_to = &ftLineTo;
        ftFunctions.conic_to = &ftConicTo;
        ftFunctions.cubic_to = &ftCubicTo;
        ftFunctions.shift = 0;
        ftFunctions.delta = 0;
        FT_Outline_Decompose(&glyph->outline, &ftFunctions, &context);

        shape.normalize();
        edgeColoringSimple(shape, 3.0);

        // Todo: make this configurable
        // Also hard-coded in material: MSDFFontMaterial
        const double pxRange = 4.0;

        Shape::Bounds bounds = shape.getBounds();
        int32 width = static_cast<int32>(Math::CeilToInt(bounds.r - bounds.l + pxRange));
        int32 height = static_cast<int32>(Math::CeilToInt(bounds.t - bounds.b + pxRange));
        msdfgen::Bitmap<float, 4> msdf(width, height);

        auto transform = msdfgen::Vector2(Math::Ceil(-bounds.l + pxRange / 2.0), Math::Ceil(-bounds.b + pxRange / 2.0));
        
        msdfgen::SDFTransformation t(msdfgen::Projection(1.0, transform), msdfgen::Range(pxRange));
        correctWinding(shape, bounds);
        generateMTSDF(msdf, shape, t);

        output.Resize(width * height * 4);

        const msdfgen::BitmapConstRef<float, 4>& bitmap = msdf;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                output[(y * width + x) * 4] = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
                output[(y * width + x) * 4 + 1] = msdfgen::pixelFloatToByte(bitmap(x, y)[1]);
                output[(y * width + x) * 4 + 2] = msdfgen::pixelFloatToByte(bitmap(x, y)[2]);
                output[(y * width + x) * 4 + 3] = msdfgen::pixelFloatToByte(bitmap(x, y)[3]);
            }
        }
        outputWidth = width;
        outputHeight = height;
        top = height - static_cast<int16>(transform.y);
        left = -static_cast<int16>(transform.x);
    }
};
