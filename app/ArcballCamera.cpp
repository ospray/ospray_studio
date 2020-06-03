// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
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

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const box3f &worldBounds, const vec2i &windowSize)
    : zoomSpeed(1),
      invWindowSize(vec2f(1.0) / vec2f(windowSize)),
      centerTranslation(one),
      translation(one),
      rotation(one)
{
  vec3f diag = worldBounds.size();
  zoomSpeed  = max(length(diag) / 150.0, 0.001);
  diag       = max(diag, vec3f(0.3f * length(diag)));

  centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  translation       = AffineSpace3f::translate(vec3f(0, 0, length(diag)));
  updateCamera();
}

void ArcballCamera::rotate(const vec2f &from, const vec2f &to)
{
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}

void ArcballCamera::zoom(float amount)
{
  amount *= zoomSpeed;
  translation = AffineSpace3f::translate(vec3f(0, 0, amount)) * translation;
  updateCamera();
}

void ArcballCamera::pan(const vec2f &delta)
{
  const vec3f t =
      vec3f(-delta.x * invWindowSize.x, delta.y * invWindowSize.y, 0);
  const vec3f worldt = translation.p.z * xfmVector(invCamera, t);
  centerTranslation  = AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

vec3f ArcballCamera::eyePos() const
{
  return xfmPoint(invCamera, vec3f(0, 0, 1));
}

vec3f ArcballCamera::center() const
{
  return -centerTranslation.p;
}

vec3f ArcballCamera::lookDir() const
{
  return xfmVector(invCamera, vec3f(0, 0, 1));
}

vec3f ArcballCamera::upDir() const
{
  return xfmVector(invCamera, vec3f(0, 1, 0));
}

void ArcballCamera::updateCamera()
{
  const AffineSpace3f rot    = LinearSpace3f(rotation);
  const AffineSpace3f camera = translation * rot * centerTranslation;
  invCamera                  = rcp(camera);
}

void ArcballCamera::setRotation(quaternionf q)
{
  rotation = q;
  updateCamera();
}

void ArcballCamera::setState(const CameraState &state)
{
  centerTranslation = state.centerTranslation;
  translation = state.translation;
  rotation = state.rotation;
  updateCamera();
}

CameraState ArcballCamera::getState() const
{
  return CameraState(centerTranslation, translation, rotation);
}

void ArcballCamera::updateWindowSize(const vec2i &windowSize)
{
  invWindowSize = vec2f(1) / vec2f(windowSize);
}

quaternionf ArcballCamera::screenToArcball(const vec2f &p)
{
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f) {
    return quaternionf(0, p.x, p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const vec2f unitDir = normalize(p);
    return quaternionf(0, unitDir.x, unitDir.y, 0);
  }
}

// Catmull-Rom interpolation for rotation quaternions
// linear interpolation for translation matrices
// adapted from Graphics Gems 2
CameraState catmullRom(const CameraState &prefix,
    const CameraState &from,
    const CameraState &to,
    const CameraState &suffix,
    float frac)
{
  if (frac == 0) {
    return from;
  } else if (frac == 1) {
    return to;
  }

  // essentially this interpolation creates a "pyramid"
  // interpolate 4 points to 3
  CameraState c10 = prefix.slerp(from, frac + 1);
  CameraState c11 = from.slerp(to, frac);
  CameraState c12 = to.slerp(suffix, frac - 1);

  // 3 points to 2
  CameraState c20 = c10.slerp(c11, (frac + 1) / 2.f);
  CameraState c21 = c11.slerp(c12, frac / 2.f);

  // and 2 to 1
  CameraState cf = c20.slerp(c21, frac);

  return cf;
}

std::vector<CameraState> buildPath(const std::vector<CameraState> &anchors,
    const float stepSize)
{
  if (anchors.size() < 2) {
    throw std::runtime_error("Must have at least 2 anchors to create path!");
  }

  // in order to touch all provided anchor, we need to extrapolate a new anchor
  // on both ends for Catmull-Rom's prefix/suffix
  size_t last = anchors.size() - 1;
  CameraState prefix = anchors[0].slerp(anchors[1], -0.1f);
  CameraState suffix = anchors[last - 1].slerp(anchors[last], 1.1f);

  std::vector<CameraState> path;
  for (size_t i = 0; i < last; i++) {
    CameraState c0 = (i == 0) ? prefix : anchors[i - 1];
    CameraState c1 = anchors[i];
    CameraState c2 = anchors[i + 1];
    CameraState c3 = (i == (last - 1)) ? suffix : anchors[i + 2];

    for (float frac = 0.f; frac < 1.f; frac += stepSize) {
      path.push_back(catmullRom(c0, c1, c2, c3, frac));
    }
  }

  return path;
}
