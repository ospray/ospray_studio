// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.h"

namespace ospray {
  namespace sg {

  Material::Material(std::string t) : matType(t)
  {
    createChild("handles");
    child("handles").setSGNoUI();
  }

  NodeType Material::type() const
  {
    return NodeType::MATERIAL;
  }

  std::string Material::osprayMaterialType() const
  {
    return matType;
  }

  void Material::preCommit()
  {
    const auto &c       = children();
    const auto &handles = child("handles").children();

    if (c.empty() || handles.empty())
      return;
    for (auto &c_child : c)
      if (c_child.second->type() == NodeType::PARAMETER)
        if (!c_child.second->sgOnly())
          for (auto &h : handles) {
            c_child.second->setOSPRayParam(
                c_child.first, h.second->valueAs<cpp::Material>().handle());
          }
  }

  void Material::postCommit()
  {
    const auto &handles = child("handles").children();
    for (auto &h : handles)
      h.second->valueAs<cpp::Material>().commit();
  }

  }  // namespace sg
} // namespace ospray
