#include "obj_loader.hpp"

#define REGISTER_VERTEX(indices, vertices, registered_vertices, vertex, positions, normals, texcoords) \
    {                                                                                                  \
        if (registered_vertices.find(vertex) == registered_vertices.end())                             \
        {                                                                                              \
            vertices.push_back({                                                                       \
                .position = positions[vertex.v],                                                       \
                .normal = vertex.vn > -1 ? normals[vertex.vn] : glm::vec3(0.0f),                       \
                .tcoords = vertex.vt > -1 ? texcoords[vertex.vt] : glm::vec2(0.0f),                    \
            });                                                                                        \
            registered_vertices[vertex] = vertices.size() - 1;                                         \
            indices.push_back(vertices.size() - 1);                                                    \
        }                                                                                              \
        else                                                                                           \
        {                                                                                              \
            indices.push_back(registered_vertices[vertex]);                                            \
        }                                                                                              \
    }

std::optional<Mesh> OBJLoader::Load(const std::string &path)
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        ERROR("File not read successfully, param: " << path);
        return {};
    }

    std::string directory = path.substr(0, path.find_last_of('/'));

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::unordered_map<Triplet, int, TripletHash> registered_vertices = {};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    INFO("Starting OBJ loading file at: " << path);
    int line = 1;
    // TODO: copy first the whole file and then parse it using pointer instead of stream.get() each time
    char c = stream.get();
    while (!stream.fail())
    {
        // comment
        if (c == '#')
        {
            // WARNING("Ignoring comment at line: " << line);
            skipLine(stream);
        }
        // object name
        else if (c == 'o')
        {
            ignoreSpaces(stream);
            c = stream.peek();
            if (c == '\n')
            {
                ERROR("Needs a group name for keyword " << BOLD("o") << " at line: " << line);
                return {};
            }

            DEBUG("Group name: " << BOLD(readLine(stream)) << " at line: " << line);
        }
        else if (c == 'v')
        {
            c = stream.get();
            // texture vertices
            if (c == 't')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec2> vt = readVec2(stream);
                if (vt)
                {
                    texcoords.push_back(*vt);
                    DEBUG("Texture vertex " << BOLD((*vt).x << ", " << (*vt).y) << " at line: " << line);
                }
                else
                {
                    ERROR("Parsing geometric vertex failed at line: " << line);
                }
            }
            // vertex normals
            else if (c == 'n')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> vn = readVec3(stream);
                if (vn)
                {
                    normals.push_back(*vn);
                    DEBUG("Normal vertex " << BOLD((*vn).x << ", " << (*vn).y << ", " << (*vn).z) << " at line: " << line);
                }
                else
                {
                    ERROR("Parsing geometric vertex failed at line: " << line);
                }
            }
            // parameter space vertices
            else if (c == 'p')
            {
            }
            // geometric vertices
            else if (c == ' ')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> v = readVec3(stream);
                if (v)
                {
                    positions.push_back(*v);
                    DEBUG("Geometric vertex " << BOLD((*v).x << ", " << (*v).y << ", " << (*v).z) << " at line: " << line);
                }
                else
                {
                    ERROR("Parsing geometric vertex failed at line: " << line);
                }
            }
            else
            {
                ERROR("Unknown keyword at line: " << line);
            }
            skipLine(stream);
        }
        // face
        else if (c == 'f')
        {
            std::vector<Triplet> triplets;
            while (!stream.fail() && stream.peek() != '\n')
            {
                ignoreSpaces(stream);
                std::optional<Triplet> triplet = readTriplet(stream);
                if (triplet)
                {
                    triplets.push_back(*triplet);
                }
                else
                {
                    break;
                }
            }
            skipLine(stream);

            if (triplets.size() < 3)
            {
                ERROR("Face needs at least three triplets at line: " << line);
                return {};
            }

            // triangulate face
            for (size_t i = 1; i < triplets.size() - 1; ++i)
            {
                REGISTER_VERTEX(indices, vertices, registered_vertices, triplets[0], positions, normals, texcoords);
                REGISTER_VERTEX(indices, vertices, registered_vertices, triplets[i], positions, normals, texcoords);
                REGISTER_VERTEX(indices, vertices, registered_vertices, triplets[i + 1], positions, normals, texcoords);
            }
        }
        // smoothing group
        else if (c == 's')
        {
            ignoreSpaces(stream);
            DEBUG("Smoothing group: " << BOLD(readLine(stream)) << " at line: " << line);
            skipLine(stream);
        }
        else if (c == 'u')
        {
            stream.unget();
            if (readWord(stream) == "usemtl")
            {
                ignoreSpaces(stream);
                DEBUG("Material name: " << BOLD(readLine(stream)) << " at line: " << line);
            }
            skipLine(stream);
        }
        else if (c == 'm')
        {
            stream.unget();
            if (readWord(stream) == "mtllib")
            {
                ignoreSpaces(stream);
                DEBUG("Material library: " << BOLD(readLine(stream)) << " at line: " << line);
            }
            skipLine(stream);
        }
        else if (c == '\n')
        {
            line++;
        }
        else
        {
            // ignore line
            // WARNING("Character not handled right now: " << c);
            skipLine(stream);
        }

        c = stream.get();
    }

    INFO("Finished loading mesh...");

    return Mesh(vertices, indices, {});
}

std::optional<OBJLoader::Triplet> OBJLoader::readTriplet(std::ifstream &stream)
{
    OBJLoader::Triplet result;

    std::optional<int> number = readInteger(stream, false);
    if (number)
    {
        // v : 0-indexed
        result.v = (*number) - 1;
    }
    else
    {
        ERROR("Missing geometric vertex in face");
        return {};
    }

    char c = stream.peek();
    if (c == ' ' || c == '\n' || c == EOF)
    {
        return result;
    }

    if (c != '/')
    {
        ERROR("Invalid triplet format");
        return {};
    }
    stream.get();

    number = readInteger(stream, false);
    if (number)
    {
        // vt : 0-indexed
        result.vt = (*number) - 1;
    }

    if (stream.peek() != '/')
    {
        ERROR("Invalid triplet format");
        return {};
    }
    stream.get();

    number = readInteger(stream, false);
    if (number)
    {
        // vn : 0-indexed
        result.vn = (*number) - 1;
    }

    return result;
}

std::optional<glm::vec2> OBJLoader::readVec2(std::ifstream &stream)
{
    glm::vec2 result;

    std::optional<float> number = readNumber(stream);
    if (number)
    {
        result.x = *number;
    }
    else
    {
        ERROR("Expects x component");
        return {};
    }
    if (stream.peek() != ' ')
    {
        ERROR("Expects y component");
        return {};
    }
    ignoreSpaces(stream);
    number = readNumber(stream);
    if (number)
    {
        result.y = *number;
    }
    else
    {
        ERROR("Expects y component");
        return {};
    }

    return result;
}

std::optional<glm::vec3> OBJLoader::readVec3(std::ifstream &stream)
{
    glm::vec3 result;

    std::optional<float> number = readNumber(stream);
    if (number)
    {
        result.x = *number;
    }
    else
    {
        ERROR("Expects x component");
        return {};
    }

    if (stream.peek() != ' ')
    {
        ERROR("Expects y component");
        return {};
    }
    ignoreSpaces(stream);
    number = readNumber(stream);
    if (number)
    {
        result.y = *number;
    }
    else
    {
        ERROR("Expects y component");
        return {};
    }

    if (stream.peek() != ' ')
    {
        ERROR("Expects z component");
        return {};
    }
    ignoreSpaces(stream);
    number = readNumber(stream);
    if (number)
    {
        result.z = *number;
    }
    else
    {
        ERROR("Expects z component");
        return {};
    }

    return result;
}

std::optional<float> OBJLoader::readNumber(std::ifstream &stream)
{
    std::string data;
    char c = stream.get();
    bool hasDecimal = false;

    if (c == '-' || c == '+')
    {
        data += c;
        c = stream.get();
    }

    while (!stream.fail())
    {
        if (c >= '0' && c <= '9')
        {
            data += c;
        }
        else if (c == '.' || c == ',')
        {
            if (!hasDecimal)
            {
                data += c;
                hasDecimal = true;
            }
            else
            {
                ERROR("Invalid number in stream");
                return {};
            }
        }
        else
        {
            stream.unget();
            break;
        }
        c = stream.get();
    }

    if (data.empty())
    {
        ERROR("No number found");
        return {};
    }
    return std::stof(data);
}

std::optional<int> OBJLoader::readInteger(std::ifstream &stream, bool strict)
{
    int value = 0;

    bool hasDigits = false;
    bool isNegative = false;

    int c = stream.peek();
    if (c == '-' || c == '+')
    {
        isNegative = (c == '-');
        stream.get(); // consume sign
    }

    while (!stream.fail())
    {
        c = stream.peek();
        if (c >= '0' && c <= '9')
        {
            hasDigits = true;
            value = value * 10 + (c - '0');
            stream.get(); // consume digit
        }
        else
        {
            break;
        }
    }
    if (!hasDigits)
    {
        if (strict)
        {
            ERROR("Invalid integer in stream");
        }
        return std::nullopt;
    }

    return isNegative ? -value : value;
}

std::string OBJLoader::readLine(std::ifstream &stream)
{
    std::string data;
    char c = stream.get();
    while (!stream.fail() && c != '\n')
    {
        data += c;
        c = stream.get();
    }
    if (!stream.fail())
    {
        stream.unget(); // put the eol into buffer
    }
    return data;
}

std::string OBJLoader::readWord(std::ifstream &stream)
{
    std::string name;
    char c = stream.get();
    while (!stream.fail() && !std::isspace(c))
    {
        name += c;
        c = stream.get();
    }
    if (!stream.fail())
    {
        stream.unget(); // put the space token into buffer
    }
    return name;
}

void OBJLoader::ignoreSpaces(std::ifstream &stream)
{
    unsigned char c = stream.get();
    while (!stream.fail() && c == ' ')
    {
        c = stream.get();
    }
    if (!stream.fail())
    {
        stream.unget(); // add back the non space character to buffer
    }
}

void OBJLoader::skipLine(std::ifstream &stream)
{
    char c = stream.get();
    while (!stream.fail() && c != '\n')
    {
        c = stream.get();
    }
    if (!stream.fail())
    {
        stream.unget();
    }
}