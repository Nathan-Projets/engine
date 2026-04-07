#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <simdjson.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../units/model.hpp"
#include "../resource_loader.hpp"
#include "../../helpers/log.hpp"

namespace resources
{
    namespace detail
    {
        inline glm::mat4 ToGlmMatrix(const aiMatrix4x4 &m)
        {
            glm::mat4 r(1.0f);
            r[0][0] = m.a1;
            r[1][0] = m.a2;
            r[2][0] = m.a3;
            r[3][0] = m.a4;
            r[0][1] = m.b1;
            r[1][1] = m.b2;
            r[2][1] = m.b3;
            r[3][1] = m.b4;
            r[0][2] = m.c1;
            r[1][2] = m.c2;
            r[2][2] = m.c3;
            r[3][2] = m.c4;
            r[0][3] = m.d1;
            r[1][3] = m.d2;
            r[2][3] = m.d3;
            r[3][3] = m.d4;
            return r;
        }

        inline glm::vec3 ToGlmVec3(const aiVector3D &v)
        {
            return glm::vec3(v.x, v.y, v.z);
        }

        inline glm::quat ToGlmQuat(const aiQuaternion &q)
        {
            return glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
        }

        inline void AddBoneInfluence(MeshVertex &vertex, uint32_t boneIndex, float weight)
        {
            for (int influenceIndex = 0; influenceIndex < 4; ++influenceIndex)
            {
                if (vertex.boneWeights[influenceIndex] <= 0.0f)
                {
                    vertex.boneIndices[influenceIndex] = boneIndex;
                    vertex.boneWeights[influenceIndex] = weight;
                    return;
                }
            }

            int smallestWeightIndex = 0;
            for (int influenceIndex = 1; influenceIndex < 4; ++influenceIndex)
            {
                if (vertex.boneWeights[influenceIndex] < vertex.boneWeights[smallestWeightIndex])
                {
                    smallestWeightIndex = influenceIndex;
                }
            }

            if (weight > vertex.boneWeights[smallestWeightIndex])
            {
                vertex.boneIndices[smallestWeightIndex] = boneIndex;
                vertex.boneWeights[smallestWeightIndex] = weight;
            }
        }

        inline void NormalizeBoneWeights(MeshVertex &vertex)
        {
            const float totalWeight = vertex.boneWeights.x + vertex.boneWeights.y + vertex.boneWeights.z + vertex.boneWeights.w;
            if (totalWeight > 0.0f)
            {
                vertex.boneWeights /= totalWeight;
            }
        }

        inline uint32_t AppendSkeletonNode(
            const aiNode *node,
            uint32_t parentIndex,
            const glm::mat4 &parentGlobalTransform,
            std::vector<SkeletonNode> &outNodes,
            std::unordered_map<std::string, uint32_t> &outNodeNameToIndex)
        {
            const uint32_t nodeIndex = static_cast<uint32_t>(outNodes.size());
            outNodes.emplace_back();

            SkeletonNode &skeletonNode = outNodes.back();
            skeletonNode.name = node ? node->mName.C_Str() : std::string();
            skeletonNode.parentIndex = parentIndex;

            aiVector3D scaling(1.0f, 1.0f, 1.0f);
            aiVector3D position(0.0f, 0.0f, 0.0f);
            aiQuaternion rotation;
            if (node)
            {
                node->mTransformation.Decompose(scaling, rotation, position);
            }

            skeletonNode.localTranslation = ToGlmVec3(position);
            skeletonNode.localRotation = ToGlmQuat(rotation);
            skeletonNode.localScale = ToGlmVec3(scaling);
            skeletonNode.localBindTransform = node ? ToGlmMatrix(node->mTransformation) : glm::mat4(1.0f);
            skeletonNode.globalBindTransform = parentGlobalTransform * skeletonNode.localBindTransform;

            outNodeNameToIndex[skeletonNode.name] = nodeIndex;

            if (parentIndex != std::numeric_limits<uint32_t>::max())
            {
                outNodes[parentIndex].children.push_back(nodeIndex);
            }

            if (node)
            {
                for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
                {
                    AppendSkeletonNode(node->mChildren[childIndex], nodeIndex, skeletonNode.globalBindTransform, outNodes, outNodeNameToIndex);
                }
            }

            return nodeIndex;
        }

        inline void AppendNodePrimitivesFromSkeleton(
            const aiNode *node,
            const std::unordered_map<std::string, uint32_t> &nodeNameToIndex,
            const glm::mat4 &parentTransform,
            const std::vector<MeshPrimitive> &meshTemplates,
            const std::vector<uint32_t> &meshMaterialIndices,
            const std::vector<bool> &meshUsesSkinning,
            std::vector<MeshPrimitiveInstance> &outInstances)
        {
            if (!node)
                return;

            const auto nodeIt = nodeNameToIndex.find(node->mName.C_Str());
            const uint32_t nodeIndex = nodeIt != nodeNameToIndex.end() ? nodeIt->second : std::numeric_limits<uint32_t>::max();
            const glm::mat4 nodeTransform = parentTransform * ToGlmMatrix(node->mTransformation);

            for (uint32_t ni = 0; ni < node->mNumMeshes; ++ni)
            {
                const uint32_t mi = node->mMeshes[ni];
                if (mi >= meshTemplates.size())
                    continue;

                MeshPrimitiveInstance inst;
                inst.primitiveIndex = mi;
                inst.materialIndex = mi < meshMaterialIndices.size() ? meshMaterialIndices[mi] : 0;
                inst.nodeIndex = nodeIndex;
                inst.name = meshTemplates[mi].name;
                inst.localTransform = nodeTransform;
                inst.usesSkinning = mi < meshUsesSkinning.size() ? meshUsesSkinning[mi] : false;
                if (inst.name.empty())
                    inst.name = node->mName.C_Str();
                outInstances.push_back(std::move(inst));
            }

            for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
            {
                AppendNodePrimitivesFromSkeleton(node->mChildren[childIndex], nodeNameToIndex, nodeTransform, meshTemplates, meshMaterialIndices, meshUsesSkinning, outInstances);
            }
        }

        inline std::vector<AnimationClip> BuildAnimationClips(
            const aiScene *scene,
            const std::unordered_map<std::string, uint32_t> &nodeNameToIndex)
        {
            std::vector<AnimationClip> clips;
            if (!scene || !scene->HasAnimations())
                return clips;

            clips.reserve(scene->mNumAnimations);
            for (uint32_t animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
            {
                const aiAnimation *sourceAnimation = scene->mAnimations[animationIndex];
                if (!sourceAnimation)
                    continue;

                AnimationClip clip;
                clip.name = sourceAnimation->mName.C_Str();
                if (clip.name.empty())
                    clip.name = "Animation_" + std::to_string(animationIndex);
                clip.durationTicks = sourceAnimation->mDuration;
                clip.ticksPerSecond = sourceAnimation->mTicksPerSecond > 0.0 ? sourceAnimation->mTicksPerSecond : 25.0;
                clip.channels.reserve(sourceAnimation->mNumChannels);

                for (uint32_t channelIndex = 0; channelIndex < sourceAnimation->mNumChannels; ++channelIndex)
                {
                    const aiNodeAnim *sourceChannel = sourceAnimation->mChannels[channelIndex];
                    if (!sourceChannel)
                        continue;

                    auto nodeIt = nodeNameToIndex.find(sourceChannel->mNodeName.C_Str());
                    if (nodeIt == nodeNameToIndex.end())
                        continue;

                    AnimationChannel channel;
                    channel.nodeIndex = nodeIt->second;
                    channel.positionKeys.reserve(sourceChannel->mNumPositionKeys);
                    channel.rotationKeys.reserve(sourceChannel->mNumRotationKeys);
                    channel.scaleKeys.reserve(sourceChannel->mNumScalingKeys);

                    for (uint32_t positionIndex = 0; positionIndex < sourceChannel->mNumPositionKeys; ++positionIndex)
                    {
                        const aiVectorKey &key = sourceChannel->mPositionKeys[positionIndex];
                        channel.positionKeys.push_back({key.mTime, ToGlmVec3(key.mValue)});
                    }

                    for (uint32_t rotationIndex = 0; rotationIndex < sourceChannel->mNumRotationKeys; ++rotationIndex)
                    {
                        const aiQuatKey &key = sourceChannel->mRotationKeys[rotationIndex];
                        channel.rotationKeys.push_back({key.mTime, ToGlmQuat(key.mValue)});
                    }

                    for (uint32_t scaleIndex = 0; scaleIndex < sourceChannel->mNumScalingKeys; ++scaleIndex)
                    {
                        const aiVectorKey &key = sourceChannel->mScalingKeys[scaleIndex];
                        channel.scaleKeys.push_back({key.mTime, ToGlmVec3(key.mValue)});
                    }

                    clip.channels.push_back(std::move(channel));
                }

                clips.push_back(std::move(clip));
            }

            return clips;
        }

        struct ResolvedMaterialTexture
        {
            std::string path;
            uint32_t uvIndex = 0;
        };

        inline std::optional<ResolvedMaterialTexture> ResolveMaterialTexturePath(
            const aiMaterial *material,
            aiTextureType textureType,
            const std::filesystem::path &meshDirectory,
            const std::string &meshPath)
        {
            if (!material)
                return std::nullopt;
            uint32_t texCount = material->GetTextureCount(textureType);
            if (texCount == 0)
                return std::nullopt;
            aiString texturePath;
            unsigned int uvIndex = 0;
            if (material->GetTexture(textureType, 0, &texturePath, nullptr, &uvIndex) != aiReturn_SUCCESS)
                return std::nullopt;
            const std::string texStr = texturePath.C_Str();
            if (texStr.empty())
                return std::nullopt;

            ResolvedMaterialTexture result;
            result.uvIndex = uvIndex;
            if (texStr[0] == '*')
            {
                result.path = std::string("embedded://") + meshPath + "#" + texStr.substr(1);
                return result;
            }
            result.path = (meshDirectory / texStr).string();
            return result;
        }

        inline std::optional<ResolvedMaterialTexture> ResolveFirstAvailableTexturePath(
            const aiMaterial *material,
            std::initializer_list<aiTextureType> types,
            const std::filesystem::path &meshDirectory,
            const std::string &meshPath)
        {
            for (aiTextureType t : types)
            {
                auto texCount = material->GetTextureCount(t);
                if (auto r = ResolveMaterialTexturePath(material, t, meshDirectory, meshPath))
                {
                    return r;
                }
            }
            return std::nullopt;
        }

        inline std::optional<uint32_t> ReadOptionalUint32(const simdjson::dom::object &object, std::string_view key)
        {
            simdjson::dom::element element;
            if (object.at_key(key).get(element))
            {
                return std::nullopt;
            }

            uint64_t value = 0;
            if (element.get(value))
            {
                return std::nullopt;
            }

            return static_cast<uint32_t>(value);
        }

        inline std::optional<ResolvedMaterialTexture> ResolveGltfTextureReference(
            const simdjson::dom::array &textures,
            const simdjson::dom::array &images,
            const simdjson::dom::object &textureInfo,
            const std::filesystem::path &meshDirectory)
        {
            const std::optional<uint32_t> textureIndex = ReadOptionalUint32(textureInfo, "index");
            if (!textureIndex.has_value())
            {
                return std::nullopt;
            }

            simdjson::dom::element textureElement;
            if (textures.at(textureIndex.value()).get(textureElement))
            {
                return std::nullopt;
            }

            simdjson::dom::object textureObject;
            if (textureElement.get(textureObject))
            {
                return std::nullopt;
            }

            const std::optional<uint32_t> imageIndex = ReadOptionalUint32(textureObject, "source");
            if (!imageIndex.has_value())
            {
                return std::nullopt;
            }

            simdjson::dom::element imageElement;
            if (images.at(imageIndex.value()).get(imageElement))
            {
                return std::nullopt;
            }

            simdjson::dom::object imageObject;
            if (imageElement.get(imageObject))
            {
                return std::nullopt;
            }

            simdjson::dom::element uriElement;
            if (imageObject.at_key("uri").get(uriElement))
            {
                return std::nullopt;
            }

            std::string_view uri;
            if (uriElement.get(uri))
            {
                return std::nullopt;
            }

            if (uri.empty() || uri.rfind("data:", 0) == 0)
            {
                return std::nullopt;
            }

            ResolvedMaterialTexture resolved;
            resolved.path = (meshDirectory / std::string(uri)).string();
            resolved.uvIndex = ReadOptionalUint32(textureInfo, "texCoord").value_or(0u);
            return resolved;
        }

        inline void ApplyGltfMaterialTextureFallback(const std::string &meshPath, const std::filesystem::path &meshDirectory, Model &model)
        {
            std::ifstream file(meshPath);
            if (!file.is_open())
            {
                return;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            simdjson::dom::parser parser;
            simdjson::dom::element document;
            if (parser.parse(buffer.str()).get(document))
            {
                return;
            }

            simdjson::dom::object root;
            if (document.get(root))
            {
                return;
            }

            simdjson::dom::element texturesElement;
            simdjson::dom::element imagesElement;
            simdjson::dom::element materialsElement;
            if (root.at_key("textures").get(texturesElement) || root.at_key("images").get(imagesElement) || root.at_key("materials").get(materialsElement))
            {
                return;
            }

            simdjson::dom::array textures;
            simdjson::dom::array images;
            simdjson::dom::array materials;
            if (texturesElement.get(textures) || imagesElement.get(images) || materialsElement.get(materials))
            {
                return;
            }

            uint32_t materialIndex = 0;
            for (const simdjson::dom::element materialElement : materials)
            {
                simdjson::dom::object materialObject;
                if (materialElement.get(materialObject))
                {
                    ++materialIndex;
                    continue;
                }

                auto applySlot = [&](MaterialTextureSlot slot, const simdjson::dom::object &textureInfo)
                {
                    if (model.GetImportedMaterialTexturePath(materialIndex, slot).has_value())
                    {
                        return;
                    }

                    const std::optional<ResolvedMaterialTexture> resolved = ResolveGltfTextureReference(textures, images, textureInfo, meshDirectory);
                    if (resolved.has_value())
                    {
                        model.SetImportedMaterialTextureInfo(materialIndex, slot, resolved->path, resolved->uvIndex);
                    }
                };

                simdjson::dom::element pbrElement;
                if (!materialObject.at_key("pbrMetallicRoughness").get(pbrElement))
                {
                    simdjson::dom::object pbrObject;
                    if (!pbrElement.get(pbrObject))
                    {
                        simdjson::dom::element baseColorTextureElement;
                        if (!pbrObject.at_key("baseColorTexture").get(baseColorTextureElement))
                        {
                            simdjson::dom::object baseColorTextureObject;
                            if (!baseColorTextureElement.get(baseColorTextureObject))
                            {
                                applySlot(MaterialTextureSlot::BaseColor, baseColorTextureObject);
                            }
                        }

                        simdjson::dom::element metallicRoughnessTextureElement;
                        if (!pbrObject.at_key("metallicRoughnessTexture").get(metallicRoughnessTextureElement))
                        {
                            simdjson::dom::object metallicRoughnessTextureObject;
                            if (!metallicRoughnessTextureElement.get(metallicRoughnessTextureObject))
                            {
                                applySlot(MaterialTextureSlot::MetallicRoughness, metallicRoughnessTextureObject);
                            }
                        }
                    }
                }

                simdjson::dom::element normalTextureElement;
                if (!materialObject.at_key("normalTexture").get(normalTextureElement))
                {
                    simdjson::dom::object normalTextureObject;
                    if (!normalTextureElement.get(normalTextureObject))
                    {
                        applySlot(MaterialTextureSlot::Normal, normalTextureObject);
                    }
                }

                simdjson::dom::element occlusionTextureElement;
                if (!materialObject.at_key("occlusionTexture").get(occlusionTextureElement))
                {
                    simdjson::dom::object occlusionTextureObject;
                    if (!occlusionTextureElement.get(occlusionTextureObject))
                    {
                        applySlot(MaterialTextureSlot::Occlusion, occlusionTextureObject);
                    }
                }

                simdjson::dom::element emissiveTextureElement;
                if (!materialObject.at_key("emissiveTexture").get(emissiveTextureElement))
                {
                    simdjson::dom::object emissiveTextureObject;
                    if (!emissiveTextureElement.get(emissiveTextureObject))
                    {
                        applySlot(MaterialTextureSlot::Emissive, emissiveTextureObject);
                    }
                }

                ++materialIndex;
            }
        }
    }

    template <>
    inline std::unique_ptr<Model> ResourceLoader<Model>::Load(const std::string &path)
    {
        auto model = std::make_unique<Model>(0, path);
        model->SetState(ResourceState::Loading);

        const std::filesystem::path meshPath(path);
        const std::filesystem::path meshDirectory = meshPath.parent_path();
        const std::string ext = meshPath.extension().string();
        std::string normalizedExt = ext;
        std::transform(normalizedExt.begin(), normalizedExt.end(), normalizedExt.begin(), [](unsigned char character)
                       { return static_cast<char>(std::tolower(character)); });
        const bool needsUvFlip = (normalizedExt == ".glb" || normalizedExt == ".gltf");

        Assimp::Importer importer;
        const aiScene *scene = nullptr;
        {
            LoadDebugStageScope stageScope("Assimp import", 0.2f);
            scene = importer.ReadFile(
                path,
                aiProcess_Triangulate |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_GenNormals |
                    aiProcess_CalcTangentSpace |
                    aiProcess_ImproveCacheLocality |
                    (needsUvFlip ? aiProcess_FlipUVs : 0u) |
                    aiProcess_SortByPType);
        }

        if (!scene || !scene->HasMeshes())
            throw std::runtime_error("Failed to load model at path: " + path);

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshPrimitive> meshTemplates;
        std::vector<uint32_t> meshMaterialIndices;
        std::vector<bool> meshUsesSkinning;
        std::vector<BoneInfo> bones;
        std::unordered_map<std::string, uint32_t> boneNameToIndex;
        meshTemplates.reserve(scene->mNumMeshes);
        meshMaterialIndices.reserve(scene->mNumMeshes);
        meshUsesSkinning.reserve(scene->mNumMeshes);

        {
            LoadDebugStageScope stageScope("Mesh conversion", 0.5f);

            uint32_t vertexBase = 0;
            for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
            {
                const aiMesh *src = scene->mMeshes[meshIndex];
                if (!src)
                    continue;

                const uint32_t primitiveIndexOffset = static_cast<uint32_t>(indices.size());

                for (uint32_t i = 0; i < src->mNumVertices; ++i)
                {
                    MeshVertex v;
                    const aiVector3D &p = src->mVertices[i];
                    v.position = glm::vec3(p.x, p.y, p.z);
                    if (src->HasNormals())
                    {
                        const aiVector3D &n = src->mNormals[i];
                        v.normal = glm::vec3(n.x, n.y, n.z);
                    }
                    if (src->HasTextureCoords(0))
                    {
                        const aiVector3D &uv = src->mTextureCoords[0][i];
                        v.uv0 = glm::vec2(uv.x, uv.y);
                    }
                    if (src->HasTextureCoords(1))
                    {
                        const aiVector3D &uv = src->mTextureCoords[1][i];
                        v.uv1 = glm::vec2(uv.x, uv.y);
                    }
                    if (src->HasTangentsAndBitangents())
                    {
                        const aiVector3D &t = src->mTangents[i];
                        const aiVector3D &bt = src->mBitangents[i];
                        v.tangent = glm::vec3(t.x, t.y, t.z);
                        v.bitangent = glm::vec3(bt.x, bt.y, bt.z);
                    }
                    vertices.push_back(v);
                }

                for (uint32_t f = 0; f < src->mNumFaces; ++f)
                {
                    const aiFace &face = src->mFaces[f];
                    if (face.mNumIndices < 3)
                        continue;
                    for (uint32_t li = 0; li < face.mNumIndices; ++li)
                        indices.push_back(vertexBase + face.mIndices[li]);
                }

                MeshPrimitive prim;
                prim.indexOffset = primitiveIndexOffset;
                prim.indexCount = static_cast<uint32_t>(indices.size()) - primitiveIndexOffset;
                prim.name = src->mName.C_Str();
                meshTemplates.push_back(std::move(prim));
                meshMaterialIndices.push_back(src->mMaterialIndex);
                meshUsesSkinning.push_back(src->HasBones());

                if (src->HasBones())
                {
                    for (uint32_t boneIndex = 0; boneIndex < src->mNumBones; ++boneIndex)
                    {
                        const aiBone *sourceBone = src->mBones[boneIndex];
                        if (!sourceBone)
                            continue;

                        const std::string boneName = sourceBone->mName.C_Str();
                        uint32_t resolvedBoneIndex = 0;
                        auto boneIt = boneNameToIndex.find(boneName);
                        if (boneIt == boneNameToIndex.end())
                        {
                            resolvedBoneIndex = static_cast<uint32_t>(bones.size());
                            boneNameToIndex.emplace(boneName, resolvedBoneIndex);

                            BoneInfo boneInfo;
                            boneInfo.name = boneName;
                            boneInfo.inverseBindMatrix = detail::ToGlmMatrix(sourceBone->mOffsetMatrix);
                            bones.push_back(std::move(boneInfo));
                        }
                        else
                        {
                            resolvedBoneIndex = boneIt->second;
                        }

                        for (uint32_t weightIndex = 0; weightIndex < sourceBone->mNumWeights; ++weightIndex)
                        {
                            const aiVertexWeight &weight = sourceBone->mWeights[weightIndex];
                            const uint32_t vertexIndex = vertexBase + weight.mVertexId;
                            if (vertexIndex >= vertices.size())
                                continue;

                            detail::AddBoneInfluence(vertices[vertexIndex], resolvedBoneIndex, weight.mWeight);
                        }
                    }
                }

                vertexBase += src->mNumVertices;
            }

            for (MeshVertex &vertex : vertices)
            {
                detail::NormalizeBoneWeights(vertex);
            }
        }

        if (vertices.empty() || indices.empty())
            throw std::runtime_error("Model file contains no renderable geometry: " + path);

        {
            LoadDebugStageScope stageScope("Skeleton build", 0.7f);

            std::vector<SkeletonNode> skeletonNodes;
            std::unordered_map<std::string, uint32_t> nodeNameToIndex;
            uint32_t rootNodeIndex = 0;
            if (scene->mRootNode)
            {
                rootNodeIndex = detail::AppendSkeletonNode(scene->mRootNode,
                                                           std::numeric_limits<uint32_t>::max(),
                                                           glm::mat4(1.0f),
                                                           skeletonNodes,
                                                           nodeNameToIndex);
            }

            for (BoneInfo &bone : bones)
            {
                auto nodeIt = nodeNameToIndex.find(bone.name);
                if (nodeIt != nodeNameToIndex.end())
                {
                    bone.nodeIndex = nodeIt->second;
                }
            }

            std::vector<MeshPrimitiveInstance> primitiveInstances;
            if (scene->mRootNode)
                detail::AppendNodePrimitivesFromSkeleton(scene->mRootNode,
                                                         nodeNameToIndex,
                                                         glm::mat4(1.0f),
                                                         meshTemplates,
                                                         meshMaterialIndices,
                                                         meshUsesSkinning,
                                                         primitiveInstances);

            {
                LoadDebugStageScope animationStage("Animation build", 0.82f);
                model->SetData(std::move(vertices), std::move(indices),
                               std::move(meshTemplates), std::move(primitiveInstances));
                model->SetSkeletonData(std::move(skeletonNodes),
                                       std::move(bones),
                                       detail::BuildAnimationClips(scene, nodeNameToIndex),
                                       rootNodeIndex);
            }
        }

        if (scene->HasMaterials())
        {
            LoadDebugStageScope stageScope("Material extraction", 0.94f);
            for (uint32_t mi = 0; mi < scene->mNumMaterials; ++mi)
            {
                const aiMaterial *src = scene->mMaterials[mi];
                if (!src)
                    continue;

                ImportedMaterialProperties importedProperties;
                aiColor4D baseColor(0.8f, 0.8f, 0.8f, 1.0f);
                if (AI_SUCCESS == aiGetMaterialColor(src, AI_MATKEY_BASE_COLOR, &baseColor) ||
                    AI_SUCCESS == aiGetMaterialColor(src, AI_MATKEY_COLOR_DIFFUSE, &baseColor))
                {
                    importedProperties.baseColorFactor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
                }

                aiColor4D emissive(0.0f, 0.0f, 0.0f, 1.0f);
                if (AI_SUCCESS == aiGetMaterialColor(src, AI_MATKEY_COLOR_EMISSIVE, &emissive))
                {
                    importedProperties.emissiveFactor = glm::vec3(emissive.r, emissive.g, emissive.b);
                }

                float scalar = 0.0f;
                if (AI_SUCCESS == aiGetMaterialFloat(src, AI_MATKEY_METALLIC_FACTOR, &scalar))
                {
                    importedProperties.metallicFactor = scalar;
                }
                if (AI_SUCCESS == aiGetMaterialFloat(src, AI_MATKEY_ROUGHNESS_FACTOR, &scalar))
                {
                    importedProperties.roughnessFactor = scalar;
                }
                model->SetImportedMaterialProperties(mi, importedProperties);

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE},
                                                                      meshDirectory, path))
                {
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::BaseColor, t->path, t->uvIndex);
                }

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA, aiTextureType_HEIGHT},
                                                                      meshDirectory, path))
                {
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Normal, t->path, t->uvIndex);
                }

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_METALNESS,
                                                                       aiTextureType_SPECULAR},
                                                                      meshDirectory, path))
                {
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::MetallicRoughness, t->path, t->uvIndex);
                }

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP,
                                                                       aiTextureType_AMBIENT},
                                                                      meshDirectory, path))
                {
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Occlusion, t->path, t->uvIndex);
                }

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR},
                                                                      meshDirectory, path))
                {
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Emissive, t->path, t->uvIndex);
                }

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_DISPLACEMENT, aiTextureType_HEIGHT},
                                                                      meshDirectory, path))
                {
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Displacement, t->path, t->uvIndex);
                }
            }
        }

        if (normalizedExt == ".gltf")
        {
            detail::ApplyGltfMaterialTextureFallback(path, meshDirectory, *model);
        }

        return model;
    }
}
