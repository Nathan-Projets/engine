#pragma once

#include <memory>

#include "loader.hpp"
#include "iloader.hpp"

#define DEFINE_LOADER(CLASS, EXT)                             \
    inline constexpr char CLASS##_##EXT##_EXTENSION[] = #EXT; \
    class CLASS : public AutoRegisterLoader<CLASS, CLASS##_##EXT##_EXTENSION>

template <typename T, const char *EXTENSION>
class AutoRegisterLoader : public ILoader
{
protected:
    struct Registrar
    {
        Registrar()
        {
            Loader::RegisterLoader(EXTENSION, []()
                                   { return std::make_unique<T>(); });
        }
    };
    inline static Registrar registrar{};
};