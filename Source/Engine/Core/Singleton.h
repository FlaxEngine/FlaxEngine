// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

template<class T>
class Singleton
{
private:

    Singleton(Singleton const&)
    {
    }

    Singleton& operator=(Singleton const&)
    {
        return *this;
    }

protected:

    Singleton()
    {
    }

    ~Singleton()
    {
    }

public:

    // Gets instance of the singleton type object
    static T* Instance()
    {
        static T _instance;
        return &_instance;
    }
};
