// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
  namespace sg {

  Geometry::Geometry(const std::string &osp_type)
  {
    setValue(cpp::Geometry(osp_type));
    createChild("isClipping", "bool", false);
    createChild("visible", "bool", true);

    child("isClipping").setSGOnly();
    child("visible").setSGOnly();
  }

  }  // namespace sg
} // namespace ospray
