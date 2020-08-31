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

// sg
#include "../Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Texture : public OSPNode<cpp::Texture, NodeType::TEXTURE>
  {
    Texture(const std::string &type);
    virtual ~Texture() override = default;

    NodeType type() const override;
  };

  }  // namespace sg
} // namespace ospray
