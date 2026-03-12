#include "fileloader.hpp"

#include "render/mesh.hpp"

std::string tools::LoadFile(const std::string &iFilepath)
{
    std::ifstream aFrom(iFilepath);
    if (!aFrom.is_open())
    {
        ERROR("File not read successfully, param: " << iFilepath);
        return "";
    }

    std::stringstream oBuffer;
    oBuffer << aFrom.rdbuf();
    aFrom.close();

    return oBuffer.str();
}

std::string tools::ltrim(const std::string &s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    {
        ++start;
    }
    return s.substr(start);
}

std::string tools::rtrim(const std::string &s)
{
    if (s.empty())
        return s;
    size_t end = s.size() - 1;
    while (end != std::string::npos && std::isspace(static_cast<unsigned char>(s[end])))
    {
        if (end == 0)
            return ""; // whole string is spaces
        --end;
    }
    return s.substr(0, end + 1);
}

std::string tools::trim(const std::string &s)
{
    return rtrim(ltrim(s));
}
