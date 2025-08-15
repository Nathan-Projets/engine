#include "fileloader.hpp"

std::string tools::LoadFile(std::string iFilepath)
{
    std::ifstream aFrom(iFilepath);
    if (!aFrom.is_open())
    {
        std::println("Error: File not read successfully, param: {}", iFilepath);
        return "";
    }

    std::stringstream oBuffer;
    oBuffer << aFrom.rdbuf();
    aFrom.close();

    return oBuffer.str();
}