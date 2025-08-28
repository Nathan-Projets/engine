#include "fileloader.hpp"

#include "render/mesh.hpp"

#define REGISTER_VERTEX(temp_indices, temp_vertices, registered_vertices, vert, positions, normals, texcoords) \
    {                                                                                                          \
        if ((registered_vertices).find((vert).token) == (registered_vertices).end())                           \
        {                                                                                                      \
            (temp_vertices).push_back({                                                                        \
                .position = (positions)[(vert).v],                                                             \
                .normal = (normals)[(vert).vn],                                                                \
                .tcoords = (texcoords)[(vert).vt],                                                             \
            });                                                                                                \
            (registered_vertices)[(vert).token] = (temp_vertices).size() - 1;                                  \
            (temp_indices).push_back((temp_vertices).size() - 1);                                              \
        }                                                                                                      \
        else                                                                                                   \
        {                                                                                                      \
            (temp_indices).push_back((registered_vertices)[(vert).token]);                                     \
        }                                                                                                      \
    }

struct FaceIndex
{
    int v = -1;
    int vt = 0;
    int vn = 0;
    std::string token = "";
};

struct Object
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

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

std::vector<Mesh> tools::LoadFileOBJ(const std::string &iFilepath)
{
    std::ifstream aFrom(iFilepath);
    if (!aFrom.is_open())
    {
        ERROR("File not read successfully, param: " << iFilepath);
        return {};
    }

    std::string directory = iFilepath.substr(0, iFilepath.find_last_of('/'));

    std::vector<Object> objects;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::unordered_map<std::string, int> registered_vertices = {};

    // DEBUG("Trying to load data from: " << iFilepath);
    std::string line;
    while (std::getline(aFrom, line))
    {
        if (line.empty())
        {
            continue;
        }
        std::stringstream streamLine(line);
        std::string prefix;
        streamLine >> prefix;

        if (prefix == "o")
        {
            objects.push_back({});
            Object &active = objects.back();
            streamLine >> active.name;
        }
        else if (prefix == "v")
        {
            glm::vec3 pos;
            streamLine >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (prefix == "vt")
        {
            glm::vec2 uv;
            streamLine >> uv.x >> uv.y;
            texcoords.push_back(uv);
        }
        else if (prefix == "vn")
        {
            glm::vec3 n;
            streamLine >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (prefix == "f")
        {
            std::vector<FaceIndex> faceVertices;
            std::string token;
            while (streamLine >> token)
            {
                faceVertices.push_back(parseFaceVertex(token));
            }

            if (faceVertices.size() < 3)
            {
                ERROR("OBJ parsing failed, reason: Face with less than 3 vertices");
                return {};
            }

            // create a new object if not already existing (should probably throw not sure about the spec on this)
            if (objects.size() == 0)
            {
                objects.push_back({});
            }
            Object &active = objects.back();

            // triangulate (fan method)
            for (size_t i = 1; i < faceVertices.size() - 1; ++i)
            {
                REGISTER_VERTEX(active.indices, active.vertices, registered_vertices, faceVertices[0], positions, normals, texcoords);
                REGISTER_VERTEX(active.indices, active.vertices, registered_vertices, faceVertices[i], positions, normals, texcoords);
                REGISTER_VERTEX(active.indices, active.vertices, registered_vertices, faceVertices[i + 1], positions, normals, texcoords);
            }
        }

        //     else if (line.starts_with("mtllib"))
        //     {
        //         // material library
        //         std::filesystem::path path = directory + "/" + trim(line.substr(std::string("mtllib").size()));

        //         if (!std::filesystem::exists(path))
        //         {
        //             ERROR("Material file doesn't exist: {}", path.string());
        //             continue;
        //         }
        //     }
    }

    std::vector<Mesh> meshes;
    for (const Object &object : objects)
    {
        if (object.vertices.size() == 0 || object.indices.size() == 0)
        {
            ERROR("vertices/indices are empty in object: " << object.name);
            return {};
        }
        meshes.push_back(Mesh(object.vertices, object.indices, {}));
    }
    // DEBUG(meshes.size() << " meshes loaded");
    return meshes;
}

FaceIndex tools::parseFaceVertex(const std::string &token)
{
    FaceIndex fi;
    std::stringstream ss(token);
    std::string index;

    // Vertex position
    if (std::getline(ss, index, '/'))
        fi.v = std::stoi(index) - 1;

    // Texture coordinate
    if (std::getline(ss, index, '/') && !index.empty())
    {
        fi.vt = std::stoi(index) - 1;
    }

    // Normal
    if (std::getline(ss, index, '/') && !index.empty())
    {
        fi.vn = std::stoi(index) - 1;
    }

    fi.token = token;

    return fi;
}

glm::vec2 tools::ParseVec2(const std::string &line, size_t startPos)
{
    glm::vec2 result(0.0f);
    const char *ptr = line.c_str() + startPos;
    char *endPtr = nullptr;

    result.x = std::strtof(ptr, &endPtr);
    if (ptr == endPtr)
    {
        throw std::runtime_error("Failed to parse X component from line: " + line);
    }
    ptr = endPtr;

    result.y = std::strtof(ptr, &endPtr);
    if (ptr == endPtr)
    {
        throw std::runtime_error("Failed to parse Y component from line: " + line);
    }
    ptr = endPtr;

    return result;
}

glm::vec3 tools::ParseVec3(const std::string &line, size_t startPos)
{
    glm::vec3 result(0.0f);
    const char *ptr = line.c_str() + startPos;
    char *endPtr = nullptr;

    result.x = std::strtof(ptr, &endPtr);
    if (ptr == endPtr)
    {
        throw std::runtime_error("Failed to parse X component from line: " + line);
    }
    ptr = endPtr;

    result.y = std::strtof(ptr, &endPtr);
    if (ptr == endPtr)
    {
        throw std::runtime_error("Failed to parse Y component from line: " + line);
    }
    ptr = endPtr;

    result.z = std::strtof(ptr, &endPtr);
    if (ptr == endPtr)
    {
        throw std::runtime_error("Failed to parse Z component from line: " + line);
    }

    return result;
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
