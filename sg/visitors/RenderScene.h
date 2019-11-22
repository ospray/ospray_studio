// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#pragma once

#include "../Node.h"
// std
#include <stack>

namespace ospray {
  namespace sg {

    struct RenderScene : public Visitor
    {
      RenderScene();

      bool operator()(Node &node, TraversalContext &ctx) override;
      void postChildren(Node &node, TraversalContext &) override;

     private:
      // Helper Functions //

      void createGeometry(Node &node);
      void addGeometriesToGroup();
      void createInstanceFromGroup();
      void placeInstancesInWorld();

      // Data //

      struct
      {
        std::vector<cpp::GeometricModel> geometries;
        cpp::Group group;
        // Apperance information:
        //     - Material
        //     - TransferFunction
        //     - ...others?
      } current;

      cpp::World world;
      std::vector<cpp::Instance> instances;
      std::stack<affine3f> xfms;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline RenderScene::RenderScene()
    {
      xfms.emplace(math::one);
    }

    inline bool RenderScene::operator()(Node &node, TraversalContext &)
    {
      bool traverseChildren = true;

      switch (node.type()) {
      case NodeType::WORLD:
        world = node.valueAs<cpp::World>();
        break;
      case NodeType::GEOMETRY:
        createGeometry(node);
        traverseChildren = false;
        break;
      case NodeType::TRANSFORM:
        xfms.push(xfms.top() * node.valueAs<affine3f>());
        break;
      default:
        break;
      }

      return traverseChildren;
    }

    inline void RenderScene::postChildren(Node &node, TraversalContext &)
    {
      switch (node.type()) {
      case NodeType::WORLD:
        createInstanceFromGroup();
        placeInstancesInWorld();
        world.commit();
        break;
      case NodeType::TRANSFORM:
        createInstanceFromGroup();
        xfms.pop();
        break;
      default:
        // Nothing
        break;
      }
    }

    inline void RenderScene::createGeometry(Node &node)
    {
      auto geom = node.valueAs<cpp::Geometry>();
      cpp::GeometricModel model(geom);
      // TODO: add material/color information
      model.commit();
      current.geometries.push_back(model);
    }

    inline void RenderScene::createInstanceFromGroup()
    {
      if (current.geometries.empty())
        return;

      if (!current.geometries.empty())
        current.group.setParam("geometry", cpp::Data(current.geometries));

      // TODO: volumes

      current.group.commit();

      cpp::Instance inst(current.group);
      inst.setParam("xfm", xfms.top());
      inst.commit();
      instances.push_back(inst);
    }

    inline void RenderScene::placeInstancesInWorld()
    {
      if (!instances.empty())
        world.setParam("instance", cpp::Data(instances));
    }

  }  // namespace sg
}  // namespace ospray