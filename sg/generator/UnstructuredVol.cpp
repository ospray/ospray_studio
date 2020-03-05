// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Generator.h"
// std
#include <random>

namespace ospray {
  namespace sg {

    struct UnstructuredVol : public Generator
    {
      UnstructuredVol()           = default;
      ~UnstructuredVol() override = default;

      void generateData() override;
    };

    OSP_REGISTER_SG_NODE_NAME(UnstructuredVol, generator_unstructured_volume);

    // UnstructuredVol definitions
    // //////////////////////////////////////////////

    void UnstructuredVol::generateData()
    {
      // define hexahedron parameters
      const float hSize = .4f;
      const float hX = -.5f, hY = -.5f, hZ = 0.f;

      // define wedge parameters
      const float wSize = .4f;
      const float wX = .5f, wY = -.5f, wZ = 0.f;

      // define tetrahedron parameters
      const float tSize = .4f;
      const float tX = .5f, tY = .5f, tZ = 0.f;

      // define pyramid parameters
      const float pSize = .4f;
      const float pX = -.5f, pY = .5f, pZ = 0.f;

      // define vertex positions
      std::vector<vec3f> vertices = {
          // hexahedron
          {-hSize + hX, -hSize + hY, hSize + hZ},  // bottom quad
          {hSize + hX, -hSize + hY, hSize + hZ},
          {hSize + hX, -hSize + hY, -hSize + hZ},
          {-hSize + hX, -hSize + hY, -hSize + hZ},
          {-hSize + hX, hSize + hY, hSize + hZ},  // top quad
          {hSize + hX, hSize + hY, hSize + hZ},
          {hSize + hX, hSize + hY, -hSize + hZ},
          {-hSize + hX, hSize + hY, -hSize + hZ},

          // wedge
          {-wSize + wX, -wSize + wY, wSize + wZ},  // botom triangle
          {wSize + wX, -wSize + wY, 0.f + wZ},
          {-wSize + wX, -wSize + wY, -wSize + wZ},
          {-wSize + wX, wSize + wY, wSize + wZ},  // top triangle
          {wSize + wX, wSize + wY, 0.f + wZ},
          {-wSize + wX, wSize + wY, -wSize + wZ},

          // tetrahedron
          {-tSize + tX, -tSize + tY, tSize + tZ},
          {tSize + tX, -tSize + tY, 0.f + tZ},
          {-tSize + tX, -tSize + tY, -tSize + tZ},
          {-tSize + tX, tSize + tY, 0.f + tZ},

          // pyramid
          {-pSize + pX, -pSize + pY, pSize + pZ},
          {pSize + pX, -pSize + pY, pSize + pZ},
          {pSize + pX, -pSize + pY, -pSize + pZ},
          {-pSize + pX, -pSize + pY, -pSize + pZ},
          {pSize + pX, pSize + pY, 0.f + pZ}};

      // define per-vertex values
      std::vector<float> vertexValues = {// hexahedron
                                         0.f,
                                         0.f,
                                         0.f,
                                         0.f,
                                         0.f,
                                         1.f,
                                         1.f,
                                         0.f,

                                         // wedge
                                         0.f,
                                         0.f,
                                         0.f,
                                         1.f,
                                         0.f,
                                         1.f,

                                         // tetrahedron
                                         1.f,
                                         0.f,
                                         1.f,
                                         0.f,

                                         // pyramid
                                         0.f,
                                         1.f,
                                         1.f,
                                         0.f,
                                         0.f};

      // define vertex indices for both shared and separate case
      std::vector<uint32_t> indicesSharedVert = {// hexahedron
                                                 0,
                                                 1,
                                                 2,
                                                 3,
                                                 4,
                                                 5,
                                                 6,
                                                 7,

                                                 // wedge
                                                 1,
                                                 9,
                                                 2,
                                                 5,
                                                 12,
                                                 6,

                                                 // tetrahedron
                                                 5,
                                                 12,
                                                 6,
                                                 17,

                                                 // pyramid
                                                 4,
                                                 5,
                                                 6,
                                                 7,
                                                 17};

      std::vector<uint32_t> &indices = indicesSharedVert;

      // define cell offsets in indices array
      std::vector<uint32_t> cells = {0, 8, 14, 18};

      // define cell types
      std::vector<uint8_t> cellTypes = {
          OSP_HEXAHEDRON, OSP_WEDGE, OSP_TETRAHEDRON, OSP_PYRAMID};

      auto &tf     = createChild("transfer_function", "transfer_function_jet");
      auto &volume = tf.createChild("volume", "volume_unstructured");

      // set data objects for volume object
      volume["vertex.position"] = (cpp::Data)vertices;
      volume["index"]           = (cpp::Data)indices;
      volume["cell.index"]         = (cpp::Data)cells;
      volume["vertex.data"]        = (cpp::Data)vertexValues;
      volume["cell.type"]          = (cpp::Data)cellTypes;

    }
  }  // namespace sg
}  // namespace ospray