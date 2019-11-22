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

#include "Camera.h"

namespace ospray {
  namespace sg {

    Camera::Camera(std::string type)
    {
      auto handle = ospNewCamera(type.c_str());
      setHandle(handle);

      createChild("position", "vec3f", "Camera position", vec3f(0.f));
      createChild("direction", "vec3f", "Camera 'look' direction", vec3f(1.f));
      createChild("up", "vec3f", "Camera 'up' direction", vec3f(0.f, 1.f, 0.f));

      createChild("nearClip", "float", "Near clip distance", 0.f);

      createChild("imageStart", "vec2f", "Start of image region", vec2f(0.f));
      createChild("imageEnd", "vec2f", "End of image region", vec2f(1.f));
    }

    NodeType Camera::type() const
    {
      return NodeType::CAMERA;
    }

  }  // namespace sg
}  // namespace ospray