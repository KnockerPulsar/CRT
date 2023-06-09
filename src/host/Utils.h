#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "common/material_id.h"
#include "host/CLMath.h"

#define STR_EQ(str1, str2) (std::strcmp((str1), (str2)) == 0)

// https://stackoverflow.com/a/26221725
template<typename ... Args>
std::string fmt( const std::string& format, Args ... args )
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s ); std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

// https://stackoverflow.com/a/10031155
auto replaceAll(const std::string& source, std::string toBeReplaced, std::string replacement) -> std::string;

auto float3Equals(float3 a, float3 b) -> bool;

auto materialIdEquals(MaterialId a, MaterialId b) -> bool;
