#ifndef BASE_HPP
#define BASE_HPP

#ifndef STEAM_BEGIN_NAMESPACE
#define STEAM_BEGIN_NAMESPACE                                                                                          \
        namespace steam {                                                                                              \
        inline namespace v1 {
#define STEAM_END_NAMESPACE                                                                                            \
        }                                                                                                              \
        }
#endif

#include <fmt/color.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#endif