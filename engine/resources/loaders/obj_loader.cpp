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

std::vector<Mesh> OBJLoader::Load(const std::string &path)
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
    std::unordered_map<std::string, Material> registered_materials = {};

    std::vector<Mesh> meshes;

    INFO("Starting OBJ loading file at: " << path);
    int line = 1;
    // TODO: optimize by copying first the whole file and then parse it using pointer instead of stream.get() each time
    char c = stream.get();
    while (!stream.fail())
    {
        // comment
        if (c == '#')
        {
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
            meshes.push_back({});
            Mesh &active = meshes.back();
            active.name = readLine(stream);
        }
        // vertices
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
                }
                else
                {
                    ERROR("Parsing geometric vertex failed at line: " << line);
                }
            }
            // normals vertices
            else if (c == 'n')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> vn = readVec3(stream);
                if (vn)
                {
                    normals.push_back(*vn);
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
            // in case no object name defined, we could stop there
            if (meshes.empty())
            {
                meshes.push_back({});
            }
            Mesh &active = meshes.back();

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
                REGISTER_VERTEX(active.indices, active.vertices, registered_vertices, triplets[0], positions, normals, texcoords);
                REGISTER_VERTEX(active.indices, active.vertices, registered_vertices, triplets[i], positions, normals, texcoords);
                REGISTER_VERTEX(active.indices, active.vertices, registered_vertices, triplets[i + 1], positions, normals, texcoords);
            }
        }
        // smoothing group
        else if (c == 's')
        {
            ignoreSpaces(stream);
            // DEBUG("Smoothing group: " << BOLD(readLine(stream)) << " at line: " << line);
            skipLine(stream);
        }
        else if (c == 'u')
        {
            stream.unget();
            if (readWord(stream) == "usemtl")
            {
                ignoreSpaces(stream);
                std::string name = readLine(stream);
                auto it = registered_materials.find(name);
                if (it != registered_materials.end())
                {
                    if (meshes.empty())
                    {
                        meshes.push_back({});
                    }
                    Mesh &active = meshes.back();
                    active.AddMaterial(it->second);
                }
            }
            skipLine(stream);
        }
        else if (c == 'm')
        {
            stream.unget();
            if (readWord(stream) == "mtllib")
            {
                ignoreSpaces(stream);

                std::filesystem::path path = directory + "/" + readLine(stream);
                if (!std::filesystem::exists(path))
                {
                    ERROR("Material file doesn't exist: " << path.string());
                    continue;
                }

                DEBUG("Material library: " << BOLD(path.string()) << " at line: " << line);
                for (Material &material : LoadMaterial(path.string()))
                {
                    if (registered_materials.find(material.name) == registered_materials.end())
                    {
                        registered_materials[material.name] = material;
                    }
                    else
                    {
                        DEBUG("Material already registered, name: " << material.name);
                    }
                }
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
            skipLine(stream);
        }

        c = stream.get();
    }

    INFO("Finished loading mesh...");

    for (Mesh &mesh : meshes)
    {
        mesh.SetupMesh();
    }

    return meshes;
}

std::vector<Material> OBJLoader::LoadMaterial(const std::string &path)
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        ERROR("File not read successfully, param: " << path);
        return {};
    }

    std::string directory = path.substr(0, path.find_last_of('/'));

    std::vector<Material> materials;

    int line = 1;
    char c = stream.get();
    while (!stream.fail())
    {
        if (c == 'K')
        {
            if (materials.empty())
            {
                ERROR("Keyword newmtl mandatory");
                return {};
            }
            c = stream.get();
            // ambient color
            if (c == 'a')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> color = readVec3(stream);
                if (color)
                {
                    Material &active = materials.back();
                    active.ambient_color = *color;
                }
                else
                {
                    return {};
                }
            }
            // diffuse color
            else if (c == 'd')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> color = readVec3(stream);
                if (color)
                {
                    Material &active = materials.back();
                    active.diffuse_color = *color;
                }
                else
                {
                    return {};
                }
            }
            // emissive color
            else if (c == 'e')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> color = readVec3(stream);
                if (color)
                {
                    Material &active = materials.back();
                    active.emissive_color = *color;
                }
                else
                {
                    return {};
                }
            }
            // specular color
            else if (c == 's')
            {
                ignoreSpaces(stream);
                std::optional<glm::vec3> color = readVec3(stream);
                if (color)
                {
                    Material &active = materials.back();
                    active.specular_color = *color;
                }
                else
                {
                    return {};
                }
            }
            else
            {
                ERROR("Unknown keyword " << BOLD("K" + c) << " at line: " << line);
                return {};
            }
            skipLine(stream);
        }
        else if (c == 'n')
        {
            stream.unget();
            if (readWord(stream) == "newmtl")
            {
                ignoreSpaces(stream);
                Material material = {.name = readLine(stream)};
                materials.push_back(material);
            }
            skipLine(stream);
        }
        else if (c == 'N')
        {
            if (materials.empty())
            {
                ERROR("Keyword newmtl mandatory");
                return {};
            }
            c = stream.get();
            // specular exponent
            if (c == 's')
            {
                ignoreSpaces(stream);
                std::optional<float> number = readNumber(stream);
                if (number)
                {
                    Material &active = materials.back();
                    active.specular_exponent = *number;
                }
                else
                {
                    return {};
                }
            }
            // optic density
            else if (c == 'i')
            {
                ignoreSpaces(stream);
                std::optional<float> number = readNumber(stream);
                if (number)
                {
                    Material &active = materials.back();
                    active.optic_density = *number;
                }
                else
                {
                    return {};
                }
            }
            else
            {
                ERROR("Unknown keyword " << BOLD("N" + c) << " at line: " << line);
            }
            skipLine(stream);
        }
        else if (c == 'd')
        {
            if (materials.empty())
            {
                ERROR("Keyword newmtl mandatory");
                return {};
            }
            ignoreSpaces(stream);
            std::optional<float> number = readNumber(stream);
            if (number)
            {
                Material &active = materials.back();
                active.alpha = *number;
            }
            else
            {
                return {};
            }
            skipLine(stream);
        }
        else if (c == 'm')
        {
            if (materials.empty())
            {
                ERROR("Keyword newmtl mandatory");
                return {};
            }
            stream.unget();
            std::string token = readWord(stream);
            ignoreSpaces(stream);
            Material &active = materials.back();
            std::string texturePath;
            if (token == "map_Kd" || token == "map_Ks" || token == "map_Ka" || token == "map_Bump")
            {
                std::filesystem::path texturePath = directory + "/" + readLine(stream);
                if (!std::filesystem::exists(texturePath))
                {
                    ERROR("Texture file doesn't exist: " << texturePath.string());
                    skipLine(stream);
                    continue;
                }
                std::optional<Texture> texture = LoadTexture(texturePath.string());
                if (texture)
                {
                    std::string type = token.substr(4);
                    if (type == "Kd")
                    {
                        (*texture).type = TextureType::DIFFUSE;
                        active.texture_diffuse = *texture;
                    }
                    else if (type == "Ks")
                    {
                        (*texture).type = TextureType::SPECULAR;
                        active.texture_specular = *texture;
                    }
                    else if (type == "Ka")
                    {
                        (*texture).type = TextureType::AMBIENT;
                        active.texture_ambiant = *texture;
                    }
                    else if (type == "Bump")
                    {
                        (*texture).type = TextureType::NORMAL;
                        active.texture_normal = *texture;
                    }
                }
            }
            else
            {
                ERROR("Unknown keyword " << BOLD(token) << " at line: " << line);
            }
            skipLine(stream);
        }
        else if (c == '\n')
        {
            line++;
        }
        else
        {
            skipLine(stream);
        }
        c = stream.get();
    }

    INFO("Loaded " << materials.size() << " materials");

    return materials;
}

std::optional<Texture> OBJLoader::LoadTexture(const std::string &path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // stbi_set_flip_vertically_on_load(true); // not sure why I need to flip and sometimes no, we'll look into it later on

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        Texture texture;
        texture.id = textureID;
        texture.path = path;
        return texture;
    }
    else
    {
        ERROR("Failed to load texture, param: " << path);
        stbi_image_free(data);
        return {};
    }
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