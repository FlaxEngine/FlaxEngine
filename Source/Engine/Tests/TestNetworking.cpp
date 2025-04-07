// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Formatting.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Networking/NetworkStream.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Networking")
{
    SECTION("Test Network Stream")
    {
        auto writeStream = New<NetworkStream>();
        auto readStream = New<NetworkStream>();

        // Quaternions
        {
#define TEST_RAW 0
            writeStream->Initialize();
            constexpr int32 QuatRes = 64;
            constexpr float ResToDeg = 360.0f / (float)QuatRes;
            for (int32 x = 0; x <= QuatRes; x++)
            {
                const float qx = (float)x * ResToDeg;
                for (int32 y = 0; y <= QuatRes; y++)
                {
                    const float qy = (float)y * ResToDeg;
                    for (int32 z = 0; z <= QuatRes; z++)
                    {
                        const float qz = (float)z * ResToDeg;
                        auto quat = Quaternion::Euler(qx, qy, qz);
                        quat.Normalize();
#if TEST_RAW
                        writeStream->WriteBytes(&quat, sizeof(quat));
#else
                        writeStream->Write(quat);
#endif
                    }
                }
            }
            readStream->Initialize(writeStream->GetBuffer(), writeStream->GetPosition());
            for (int32 x = 0; x <= QuatRes; x++)
            {
                const float qx = (float)x * ResToDeg;
                for (int32 y = 0; y <= QuatRes; y++)
                {
                    const float qy = (float)y * ResToDeg;
                    for (int32 z = 0; z <= QuatRes; z++)
                    {
                        const float qz = (float)z * ResToDeg;
                        auto expected = Quaternion::Euler(qx, qy, qz);
                        expected.Normalize();
                        Quaternion quat;
#if TEST_RAW
                        readStream->ReadBytes(&quat, sizeof(quat));
#else
                        readStream->Read(quat);
#endif
                        const bool equal = Quaternion::Dot(expected, quat) > 0.9999f;
                        CHECK(equal);
                    }
                }
            }
#undef TEST_RAW
        }

        Delete(readStream);
        Delete(writeStream);
    }
}
