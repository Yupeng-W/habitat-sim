// Copyright (c) Meta Platforms, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "CompositorState.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/MeshTools/Concatenate.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MaterialData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

namespace esp {

namespace Cr = Corrade;
namespace Mn = Magnum;
using namespace Cr::Containers::Literals;
using namespace Mn::Math::Literals;

CompositorState::CompositorState(const Corrade::Containers::StringView output) {
  converterManager.registerExternalManager(imageConverterManager);

  /* Reasonable config defaults */
  if(Cr::PluginManager::PluginMetadata* m = importerManager.metadata("GltfImporter")) {
    /* Don't need any of this */
    m->configuration().setValue("phongMaterialFallback", false);
    m->configuration().setValue("compatibilitySkinningAttributes", false);
  }
  if(Cr::PluginManager::PluginMetadata* m = imageConverterManager.metadata("BasisImageConverter")) {
    // TODO ... actually, do we want it at all? it's crap slow
  }
  if(Cr::PluginManager::PluginMetadata* m = converterManager.metadata("GltfSceneConverter")) {
    m->configuration().setValue("experimentalKhrTextureKtx", true);
    // TODO BasisKtxImageConverter but again, do we want basis at all?? it's SLOW
    m->configuration().setValue("imageConverter", "KtxImageConverter");
  }

  /* Magnum's OBJ importer is ... well, not great. It'll get replaced
     eventually. Assimp is not great either, tho, UFBX would be much nicer. */
  if(importerManager.loadState("ObjImporter") != Cr::PluginManager::LoadState::NotFound)
    importerManager.setPreferredPlugins("ObjImporter", {"AssimpImporter"});

  /* Use StbImageImporter because for it we can override channel count */
  // TODO channel count option on (S)PngImporter itself, some have just 1 channel (ffs)
  // TODO what about transparent things?
  {
  #if 1
    Cr::PluginManager::PluginMetadata* m = importerManager.metadata("StbImageImporter");
    CORRADE_INTERNAL_ASSERT(m);
    m->configuration().setValue("forceChannelCount", 3);
    importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    importerManager.setPreferredPlugins("JpegImporter", {"StbImageImporter"});
#else
    importerManager.setPreferredPlugins("PngImporter", {"SpngImporter"});
#endif

  }

  // TODO configurable?
  converter = converterManager.loadAndInstantiate("GltfSceneConverter");

  /* To prevent the file from being opened by unsuspecting libraries */
  converter->configuration().addValue("extensionUsed", "MAGNUMX_mesh_views");
  converter->configuration().addValue("extensionRequired", "MAGNUMX_mesh_views");

  /* Create the output directory if it doesn't exist yet */
  CORRADE_INTERNAL_ASSERT(Cr::Utility::Path::make(Cr::Utility::Path::split(output).first()));

  /* Begin file conversion */
  converter->beginFile(output);
  converter->setSceneFieldName(SceneFieldMeshViewIndexOffset, "meshViewIndexOffset");
  converter->setSceneFieldName(SceneFieldMeshViewIndexCount, "meshViewIndexCount");
  converter->setSceneFieldName(SceneFieldMeshViewMaterial, "meshViewMaterial");
}

Mn::Trade::SceneData CompositorSceneState::finalizeScene() const {
  /* Combine the SceneData. In case of glTF the SceneData could be just a view
     on the whole memory, with no combining, but this future-proofs it for
     dumping into a binary representation */
  // TODO use SceneTools::combine() instead once it's public
  Cr::Containers::StridedArrayView1D<Parent> outputParents;
  Cr::Containers::StridedArrayView1D<Transformation> outputTransformations;
  Cr::Containers::StridedArrayView1D<Mesh> outputMeshes;
  Cr::Containers::ArrayTuple data{
    {Cr::NoInit, parents.size(), outputParents},
    {Cr::NoInit, transformations.size(), outputTransformations},
    {Cr::NoInit, meshes.size(), outputMeshes},
  };
  Cr::Utility::copy(parents, outputParents);
  Cr::Utility::copy(transformations, outputTransformations);
  Cr::Utility::copy(meshes, outputMeshes);
  Mn::Trade::SceneData scene{Mn::Trade::SceneMappingType::UnsignedInt, parents.size(), std::move(data), {
    Mn::Trade::SceneFieldData{Mn::Trade::SceneField::Parent,
      outputParents.slice(&Parent::mapping),
      outputParents.slice(&Parent::parent)},
    Mn::Trade::SceneFieldData{Mn::Trade::SceneField::Transformation,
      outputTransformations.slice(&Transformation::mapping),
      outputTransformations.slice(&Transformation::transformation)},
    Mn::Trade::SceneFieldData{Mn::Trade::SceneField::Mesh,
      outputMeshes.slice(&Mesh::mapping),
      outputMeshes.slice(&Mesh::mesh)},
    Mn::Trade::SceneFieldData{SceneFieldMeshViewIndexOffset,
      outputMeshes.slice(&Mesh::mapping),
      outputMeshes.slice(&Mesh::meshIndexOffset)},
    Mn::Trade::SceneFieldData{SceneFieldMeshViewIndexCount,
      outputMeshes.slice(&Mesh::mapping),
      outputMeshes.slice(&Mesh::meshIndexCount)},
    Mn::Trade::SceneFieldData{SceneFieldMeshViewMaterial,
      outputMeshes.slice(&Mesh::mapping),
      outputMeshes.slice(&Mesh::meshMaterial)}
  }};
  return scene;
}

CompositorDataState::CompositorDataState(const Mn::Vector2i& textureAtlasSize): textureAtlasSize{textureAtlasSize} {
  constexpr Mn::Color3ub WhitePixel[]{0xffffff_rgb};
#if 0
  arrayAppend(inputImages, Mn::Trade::ImageData2D{
    Mn::PixelStorage{}.setAlignment(1),
    Mn::PixelFormat::RGB8Unorm, {1, 1},
    Mn::Trade::DataFlags{}, WhitePixel});

#else

  // TODO figure out why the above doesn't work with zero scale for repeated textures
  // TODO abiity to switch between one and the other
  Cr::Containers::Array<char> data{Cr::NoInit, std::size_t(textureAtlasSize.product())*sizeof(Mn::Color3ub)};
  Cr::Utility::copy(
    // TODO conversion of Vector2i to Size/Stride
    Cr::Containers::StridedArrayView2D<const Mn::Color3ub>{WhitePixel,
      {std::size_t(textureAtlasSize.y()), std::size_t(textureAtlasSize.x())},
      {0, 0}},
    Cr::Containers::StridedArrayView2D<Mn::Color3ub>{Cr::Containers::arrayCast<Mn::Color3ub>(data), {std::size_t(textureAtlasSize.y()), std::size_t(textureAtlasSize.x())}});
  arrayAppend(inputImages, Mn::Trade::ImageData2D{
    Mn::PixelStorage{}.setAlignment(1),
    Mn::PixelFormat::RGB8Unorm, textureAtlasSize,
    std::move(data)});
#endif

  arrayAppend(inputMaterials, Mn::Trade::MaterialData{Mn::Trade::MaterialType::PbrMetallicRoughness, {
    {Mn::Trade::MaterialAttribute::BaseColorTexture, 0u},
    /* The layer ID and matrix translation get updated based on where the 1x1
       image ends up being in the atlas */
    {Mn::Trade::MaterialAttribute::BaseColorTextureLayer, 0u},
    // {Mn::Trade::MaterialAttribute::BaseColorTextureMatrix,
    //   Mn::Matrix3::scaling(Mn::Vector2{1.0f}/Mn::Vector2(textureAtlasSize))}
  }});
}

Mn::Trade::MeshData CompositorDataState::finalizeMesh() const {
  /* Target layout for the mesh. So far just normals, no tangents for normal
     mapping. */
  // TODO pack normals to 16bit and texcoords to half-floats(gltf extension?)
  Mn::Trade::MeshData mesh{Mn::MeshPrimitive::Triangles, nullptr, {
    Mn::Trade::MeshAttributeData{Mn::Trade::MeshAttribute::Position, Mn::VertexFormat::Vector3, nullptr},
    Mn::Trade::MeshAttributeData{Mn::Trade::MeshAttribute::Normal, Mn::VertexFormat::Vector3, nullptr},
    Mn::Trade::MeshAttributeData{Mn::Trade::MeshAttribute::TextureCoordinates, Mn::VertexFormat::Vector2, nullptr},
  }};
  // TODO generate normals for meshes that don't have them if there are any
  // TODO this should have been gradual to avoid too high peak mem usage
  Mn::MeshTools::concatenateInto(mesh, inputMeshes);

  return mesh;
}

Mn::Trade::ImageData3D CompositorDataState::finalizeImage(const Cr::Containers::ArrayView<Mn::Trade::MaterialData> inputMaterials) const {
  /* Just set the limit to the total image count -- that'll make all reference
     a single texture */
  return finalizeImage(inputMaterials, inputImages.size());
}

Mn::Trade::ImageData3D CompositorDataState::finalizeImage(const Cr::Containers::ArrayView<Mn::Trade::MaterialData> inputMaterials, Mn::Int layerCountLimit) const {
  /* Pack input images into an atlas */
  Cr::Containers::Pair<Mn::Int, Cr::Containers::Array<Mn::Vector3i>> layerCountOffsets =
    Mn::TextureTools::atlasArrayPowerOfTwo(textureAtlasSize, stridedArrayView(inputImages).slice(&Mn::Trade::ImageData2D::size));

  /* A combined 2D array image */
  // TODO document why the alignemnt
  Mn::Trade::ImageData3D image{Mn::PixelStorage{}.setAlignment(1), Mn::PixelFormat::RGB8Unorm,
    {textureAtlasSize, layerCountOffsets.first()},
    Cr::Containers::Array<char>{Cr::NoInit, std::size_t(textureAtlasSize.product()*layerCountOffsets.first()*3)}, Mn::ImageFlag3D::Array};
  /* Copy the images to their respective locations, calculate waste ratio
     during the process */
  std::size_t inputImageArea = 0;
  for(std::size_t i = 0; i != inputImages.size(); ++i) {
    inputImageArea += inputImages[i].size().product();
    /* This should have been ensured at the import time already, RGBA is for
       Basis (sigh) */
    CORRADE_ASSERT(
      inputImages[i].format() == Mn::PixelFormat::RGB8Unorm ||
      inputImages[i].format() == Mn::PixelFormat::RGB8Srgb ||
      inputImages[i].format() == Mn::PixelFormat::RGBA8Unorm ||
      inputImages[i].format() == Mn::PixelFormat::RGBA8Srgb,
      "Unexpected" << inputImages[i].format() << "in image" << i, (Mn::Trade::ImageData3D{Mn::PixelFormat::R8I, {}, nullptr}));
    Cr::Utility::copy(inputImages[i].pixels().prefix({
      std::size_t(inputImages[i].size().y()),
      std::size_t(inputImages[i].size().x()),
      3 /* to strip off the alpha channel if present */
    }),
      // TODO have implicit conversion of Vector to StridedDimensions, FINALLY
      image.mutablePixels()[layerCountOffsets.second()[i].z()].sliceSize({
        std::size_t(layerCountOffsets.second()[i].y()),
        std::size_t(layerCountOffsets.second()[i].x()),
        0
      }, {
        std::size_t(inputImages[i].size().y()),
        std::size_t(inputImages[i].size().x()),
        std::size_t(3)
      }));

    /* Free the input image right after the copy to reduce peak memory use */
    // inputImages[i] = Mn::Trade::ImageData3D{Mn::PixelFormat::RGB8Unorm, {}, nullptr};
  }

  Mn::Debug{} << inputImages.size() << "images packed to" << layerCountOffsets.first() << "layers," << Cr::Utility::format("{:.2f}", 100.0f - 100.0f*inputImageArea/(textureAtlasSize.product()*layerCountOffsets.first())) << Mn::Debug::nospace << "% area wasted";

  /* Update layer and offset info in the materials */
  for(Mn::Trade::MaterialData& inputMaterial: inputMaterials) {
    Mn::UnsignedInt& texture = inputMaterial.mutableAttribute<Mn::UnsignedInt>(Mn::Trade::MaterialAttribute::BaseColorTexture);
    Mn::UnsignedInt& layer = inputMaterial.mutableAttribute<Mn::UnsignedInt>(Mn::Trade::MaterialAttribute::BaseColorTextureLayer);
    const Mn::UnsignedInt imageId = layer;

    // TODO the separation to textures would probably make more sense done
    //  spatially, i.e. meshes rendered together being in the same layer .. but
    //  who cares for now
    texture = layerCountOffsets.second()[imageId].z()/layerCountLimit;
    layer = layerCountOffsets.second()[imageId].z()%layerCountLimit;

    /* If the material has a texture matrix (textures that are same as atlas
       layer size don't have it), update the offset there */
    if(const Cr::Containers::Optional<Mn::UnsignedInt> baseColorTextureMatrixAttributeId = inputMaterial.findAttributeId(Mn::Trade::MaterialAttribute::BaseColorTextureMatrix)) {
      Mn::Matrix3& matrix = inputMaterial.mutableAttribute<Mn::Matrix3>(*baseColorTextureMatrixAttributeId);
      matrix = Mn::Matrix3::translation(Mn::Vector2{layerCountOffsets.second()[imageId].xy()}/Mn::Vector2{textureAtlasSize})*matrix;
    }
  }

  return image;
}

Mn::Trade::TextureData CompositorDataState::finalizeTexture() const {
  return Mn::Trade::TextureData{Mn::Trade::TextureType::Texture2DArray,
    Mn::SamplerFilter::Linear, Mn::SamplerFilter::Linear, Mn::SamplerMipmap::Linear,
    Mn::SamplerWrapping::Repeat, 0};
}

}
