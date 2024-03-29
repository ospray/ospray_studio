// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/* ----------------------------------------------------------
** MainWindow class holds functions and member variables for 
** implementing the windowing infrastructure of Studio
** GUI-mode. It uses GLFW to create the WIndow and holds the
** main rendering display loop.
** Internally it also creates Editors based on Imgui but
** doesn't explicitly hold any imgui dependencies.
** ---------------------------------------------------------*/

// glfw
#include <GLFW/glfw3.h>

// std
#include <functional>
#include <iostream>
#include <vector>

#include "widgets/AnimationWidget.h"
#include "widgets/GenerateImGuiWidgets.h"
#include "GUIContext.h"

// on Windows often only GL 1.1 headers are present
// and Mac may be missing the float defines
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif

class MainWindow
{ public:
  MainWindow(const vec2i &windowSize, std::shared_ptr<GUIContext> ctx);

  ~MainWindow();

  std::stringstream windowTitle;

  void updateTitleBar();
  void initGLFW();

  void mainLoop();

  void reshape(bool reshapeGLFW = false);
  void startNewOSPRayFrame();
  void waitOnOSPRayFrame();
  void resetArcball();

  void motion(const vec2f &position);
  void keyboardMotion();
  void mouseButton(const vec2f &position);
  void mouseWheel(const vec2f &scroll);
  void display();
  void centerOnEyePos();
  void buildUI();
  
  void pickCenterOfRotation(float x, float y);

    // GLFW window instance
  GLFWwindow *glfwWindow = nullptr;

  std::shared_ptr<MainMenuBuilder> mainMenuBuilder = nullptr;
  std::shared_ptr<WindowsBuilder> windowsBuilder = nullptr;

  // OpenGL framebuffer texture
  GLuint framebufferTexture = 0;

  // optional registered display callback, called before every display()
  std::function<void(MainWindow *)> displayCallback;

  // optional registered ImGui callback, called during every frame to build UI
  std::function<void()> uiCallback;

  // optional registered key callback, called when keys are pressed
  std::function<void(
      MainWindow *, int key, int scancode, int action, int mods)>
      keyCallback;

  // format used by glTexImage2D, as determined at context creation time
  GLenum gl_rgb_format;
  GLenum gl_rgba_format;

  int fontSize{13}; // pixels
  vec2f contentScale{1.0f};
  vec2i windowSize;
  vec2i fbSize;
  vec2f previousMouse{-1.f};

  // frame
  std::shared_ptr<sg::Frame> frame;
  std::shared_ptr<AnimationWidget> animationWidget{nullptr};
  
  std::shared_ptr<GUIContext> ctx = nullptr;
  std::shared_ptr<ArcballCamera> arcballCamera;
  std::shared_ptr<CameraStack<CameraState>> cameraStack = nullptr;

  CameraStack<CameraState> g_camPath; // interpolated path through cameraStack

  // static member variables
  static int g_camPathPause; // _seconds_ to pause for at end of path
  static int g_rotationConstraint;
  static double CAM_MOVERATE; // TODO: the constant should be scene dependent or
                              // user changeable
  static double g_camMoveX;
  static double g_camMoveY;
  static double g_camMoveZ;
  static double g_camMoveA;
  static double g_camMoveE;
  static double g_camMoveR;

  static bool g_quitNextFrame;
  static bool showUi; // toggles display of ImGui UI, if an ImGui callback is provided
  static bool g_saveNextFrame;

  // FPS measurement of last frame
  float latestFPS{0.f};

  // Option to always show a gamma corrected display to user.  Native sRGB
  // buffer is untouched, linear buffers are displayed as sRGB.
  bool uiDisplays_sRGB{true};

  // Camera motion controls
  float maxMoveSpeed{1.f};
  float fineControl{0.2f};
  float preFPVZoom{0.f};

  // auto rotation speed, 1=0.1% window width mouse movement, 100=10%
  int autorotateSpeed{1};

  void static error_callback(int error, const char *desc);

  void setLockUpDir(const vec3f &lockUpDir);
  void setUpDir(const vec3f &upDir);
  void animationSetShowUI();
  void quitNextFrame();
};
