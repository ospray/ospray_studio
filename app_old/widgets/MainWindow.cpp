// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2016-2018 Intel Corporation                                    //
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

#include "MainWindow.h"
// ospcommon
#include "ospcommon/utility/SaveImage.h"
#include "ospcommon/utility/getEnvVar.h"
// ospray_sg
#include "sg/common/FrameBuffer.h"
#include "sg/visitor/GatherNodesByPosition.h"
#include "sg/visitor/MarkAllAsModified.h"
// imgui
#include "imgui.h"
#include "imguifilesystem/imguifilesystem.h"
// ospray_sg ui
#include "sg_ui/ospray_sg_ui.h"
// panels
#include "GenericPanel.h"
#include "panels/About.h"
#include "panels/Lights.h"
#include "panels/NodeFinder.h"
#include "panels/RenderingSettings.h"
#include "panels/SGAdvanced.h"
#include "panels/SGTreeView.h"
#include "panels/TransferFunctionEditor.h"

#include "../sg_utility/utility.h"

#include "../jobs/JobScheduler.h"

#include <GLFW/glfw3.h>

using namespace ospcommon;

static ImGuiFs::Dialog openFileDialog;

namespace ospray {

  MainWindow *MainWindow::g_instance = nullptr;

  MainWindow::MainWindow(const std::shared_ptr<sg::Frame> &scenegraph,
                         const PluginManager &pluginManager,
                         const std::vector<std::string> &tfnsToLoad)
      : ImGui3DWidget(ImGui3DWidget::RESIZE_KEEPFOVY),
        scenegraph(scenegraph),
        renderer(scenegraph->child("renderer").nodeAs<sg::Renderer>()),
        master_tfn(sg::createNode("master_tf", "TransferFunction")
                       ->nodeAs<sg::TransferFunction>()),
        renderEngine(scenegraph)
  {
    if (g_instance != nullptr)
      throw std::runtime_error("FATAL: can only instantiate MainWindow once!");

    g_instance = this;

    AsyncRenderEngine::g_instance = &renderEngine;
    setDefaultViewToCamera();
    setWorldBounds(renderer->child("bounds").valueAs<box3f>());

    auto &navFB = scenegraph->createChild("navFrameBuffer", "FrameBuffer");
    navFB["useAccumBuffer"]    = false;
    navFB["useVarianceBuffer"] = false;

    auto OSPRAY_DYNAMIC_LOADBALANCER =
        utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

    useDynamicLoadBalancer = OSPRAY_DYNAMIC_LOADBALANCER.value_or(false);

    if (useDynamicLoadBalancer)
      numPreAllocatedTiles = OSPRAY_DYNAMIC_LOADBALANCER.value();

    originalView = viewPort;

    scenegraph->child("frameAccumulationLimit") = accumulationLimit;

    ospSetProgressFunc(&ospray::MainWindow::progressCallbackWrapper, this);

    // create panels //

    aboutPanel = make_unique<PanelAbout>();

    defaultPanels.emplace_back(
        new GenericPanel("Renderer Stats", [&]() { this->guiRenderStats(); }));

    defaultPanels.emplace_back(new GenericPanel(
        "Job Scheduler", [&]() { this->guiJobStatusControlPanel(); }));

    defaultPanels.emplace_back(new PanelRenderingSettings(scenegraph));
    defaultPanels.emplace_back(new PanelTFEditor(master_tfn, tfnsToLoad));
    defaultPanels.emplace_back(new PanelLights(scenegraph));
    defaultPanels.emplace_back(new PanelNodeFinder(scenegraph));

    auto newPluginPanels = pluginManager.getAllPanelsFromPlugins(scenegraph);
    std::move(newPluginPanels.begin(),
              newPluginPanels.end(),
              std::back_inserter(pluginPanels));

    advancedPanels.emplace_back(new PanelSGTreeView(scenegraph));
    advancedPanels.emplace_back(new PanelSGAdvanced(scenegraph));
  }

  MainWindow::~MainWindow()
  {
    renderEngine.stop();
  }

  void MainWindow::startAsyncRendering()
  {
    renderEngine.start();
  }

  void MainWindow::mouseButton(int button, int action, int mods)
  {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
        ((mods & GLFW_MOD_SHIFT) | (mods & GLFW_MOD_CONTROL))) {
      const vec2f pos(currMousePos.x / static_cast<float>(windowSize.x),
                      1.f - currMousePos.y / static_cast<float>(windowSize.y));
      renderEngine.pick(pos);
      lastPickQueryType = (mods & GLFW_MOD_SHIFT) ? PICK_CAMERA : PICK_NODE;
    }
  }

  void MainWindow::reshape(const vec2i &newSize)
  {
    ImGui3DWidget::reshape(newSize);
    scenegraph->child("frameBuffer")["size"]    = renderSize;
    scenegraph->child("navFrameBuffer")["size"] = navRenderSize;
  }

  void MainWindow::keypress(char key)
  {
    switch (key) {
    case ' ': {
      if (renderer && renderer->hasChild("animationcontroller")) {
        bool animating =
            renderer->child("animationcontroller")["enabled"].valueAs<bool>();
        renderer->child("animationcontroller")["enabled"] = !animating;
      }
      break;
    }
    case 'R':
      toggleRenderingPaused();
      break;
    case '!':
      saveScreenshot = true;
      break;
    case 'X':
      viewPort.setViewUpX();
      break;
    case 'Y':
      viewPort.setViewUpY();
      break;
    case 'Z':
      viewPort.setViewUpZ();
      break;
    case 'c':
      viewPort.modified = true;  // Reset accumulation
      break;
    case 'o':
      showWindowImportData = true;
      break;
    case 'g':
      showWindowGenerateData = true;
      break;
    case 'r':
      resetView();
      break;
    case 'd':
      resetDefaultView();
      break;
    case 'L':
      clearScene();
      break;
    case 'p':
      printViewport();
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      int whichPanel = key - '0' - 1;
      auto numDefaultPanels = defaultPanels.size();
      if (whichPanel < numDefaultPanels)
        defaultPanels[whichPanel]->toggleShown();
      else if (whichPanel < numDefaultPanels + pluginPanels.size())
        pluginPanels[whichPanel - numDefaultPanels]->toggleShown();
      break;
    }
    default:
      ImGui3DWidget::keypress(key);
    }
  }

  void MainWindow::resetView()
  {
    auto oldAspect    = viewPort.aspect;
    viewPort          = originalView;
    viewPort.aspect   = oldAspect;
    viewPort.modified = true;
  }

  void MainWindow::setDefaultViewToCamera()
  {
    auto [pos, gaze, up] = getViewFromCamera(*scenegraph);

    setViewPort(pos, gaze, up);
    originalView = viewPort;
  }

  void MainWindow::resetDefaultView()
  {
    createDefaultView(*scenegraph);
    setDefaultViewToCamera();
  }

  void MainWindow::printViewport()
  {
    printf("-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
           viewPort.from.x,
           viewPort.from.y,
           viewPort.from.z,
           viewPort.up.x,
           viewPort.up.y,
           viewPort.up.z,
           viewPort.at.x,
           viewPort.at.y,
           viewPort.at.z);
    fflush(stdout);
  }

  void MainWindow::toggleRenderingPaused()
  {
    renderingPaused = !renderingPaused;
    renderingPaused ? renderEngine.stop() : renderEngine.start();
  }

  void MainWindow::display()
  {
    if (renderEngine.hasNewPickResult()) {
      auto picked = renderEngine.getPickResult();
      if (picked.hit) {
        if (lastPickQueryType == PICK_NODE) {
#if 0  // TODO: this list needs to be forwarded to a UI widget (or pick pos?)
          sg::GatherNodesByPosition visitor((vec3f&)picked.position);
          scenegraph->traverse(visitor);
          collectedNodesFromSearch = visitor.results();
#endif
        } else {
          // No conversion operator or ctor??
          viewPort.at.x     = picked.position.x;
          viewPort.at.y     = picked.position.y;
          viewPort.at.z     = picked.position.z;
          viewPort.modified = true;
        }
      }
    }

    if (viewPort.modified) {
      auto &camera = scenegraph->child("camera");
      auto dir     = viewPort.at - viewPort.from;
      if (camera.hasChild("focusDistance"))
        camera["focusDistance"] = length(dir);
      dir           = normalize(dir);
      camera["dir"] = dir;
      camera["pos"] = viewPort.from;
      camera["up"]  = viewPort.up;
      camera.markAsModified();

      // don't cancel the first frame, otherwise it is hard to navigate
      if (scenegraph->frameId() > 0 && cancelFrameOnInteraction) {
        cancelRendering = true;
        renderEngine.setFrameCancelled();
      }

      viewPort.modified = false;
    }

    renderFPS         = renderEngine.lastFrameFps();
    renderFPSsmoothed = renderEngine.lastFrameFpsSmoothed();

    if (renderEngine.hasNewFrame()) {
      auto &mappedFB = renderEngine.mapFramebuffer();
      auto fbSize    = mappedFB.size();
      auto fbData    = mappedFB.data();
      GLenum texelType;
      std::string filename("ospexampleviewer");
      switch (mappedFB.format()) {
      default: /* fallthrough */
      case OSP_FB_NONE:
        fbData = nullptr;
        break;
      case OSP_FB_RGBA8: /* fallthrough */
      case OSP_FB_SRGBA:
        texelType = GL_UNSIGNED_BYTE;
        if (saveScreenshot) {
          filename += ".ppm";
          utility::writePPM(filename, fbSize.x, fbSize.y, (uint32_t *)fbData);
        }
        break;
      case OSP_FB_RGBA32F:
        texelType = GL_FLOAT;
        if (saveScreenshot) {
          filename += ".pfm";
          utility::writePFM(filename, fbSize.x, fbSize.y, (vec4f *)fbData);
        }
        break;
      }

      // update/upload fbTexture
      if (fbData) {
        fbAspect = fbSize.x / float(fbSize.y);
        glBindTexture(GL_TEXTURE_2D, fbTexture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     fbSize.x,
                     fbSize.y,
                     0,
                     GL_RGBA,
                     texelType,
                     fbData);
      } else
        fbAspect = 1.f;

      if (saveScreenshot) {
        std::cout << "saved current frame to '" << filename << "'" << std::endl;
        saveScreenshot = false;
      }

      renderEngine.unmapFramebuffer();
    }

    // set border color TODO maybe move to application
    vec4f texBorderCol(0.f);  // default black
    // TODO be more sophisticated (depending on renderer type, fb mode (sRGB))
    if (renderer->child("useBackplate").valueAs<bool>()) {
      auto col      = renderer->child("bgColor").valueAs<vec3f>();
      const float g = 1.f / 2.2f;
      texBorderCol = vec4f(powf(col.x, g), powf(col.y, g), powf(col.z, g), 0.f);
    }
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &texBorderCol[0]);

    ImGui3DWidget::display();

    lastTotalTime   = ImGui3DWidget::totalTime;
    lastGUITime     = ImGui3DWidget::guiTime;
    lastDisplayTime = ImGui3DWidget::displayTime;
  }

  void MainWindow::processFinishedJobs()
  {
    for (auto it = jobsInProgress.begin(); it != jobsInProgress.end(); ++it) {
      auto &job = *it;
      if (job.get() && job->isFinished()) {
        auto nodes = job->get();
        std::copy(nodes.begin(), nodes.end(), std::back_inserter(loadedNodes));
        jobsInProgress.erase(it++);
      }
    }

    if (autoImportNodesFromFinishedJobs && !loadedNodes.empty())
      importFinishedNodes();
  }

  void MainWindow::importFinishedNodes()
  {
    bool wasRunning = renderEngine.runningState() == ExecState::RUNNING;
    renderEngine.stop();

    for (auto &node : loadedNodes)
      renderer->child("world").add(node);

    replaceAllTFsWithMasterTF(*scenegraph);

    loadedNodes.clear();

    renderer->computeBounds();

    scenegraph->verify();

    resetDefaultView();
    resetView();

    setMotionSpeed(-1.f);
    setWorldBounds(renderer->child("bounds").valueAs<box3f>());

    if (wasRunning)
      renderEngine.start();
  }

  void MainWindow::clearScene()
  {
    bool wasRunning = renderEngine.runningState() == ExecState::RUNNING;
    renderEngine.stop();

    auto newWorld = sg::createNode("world", "Model");
    renderer->add(newWorld);

    scenegraph->traverse(sg::MarkAllAsModified{});

    if (wasRunning)
      renderEngine.start();
  }

  std::shared_ptr<sg::Node> MainWindow::getMasterTransferFunctioNode()
  {
    return master_tfn;
  }

  void MainWindow::buildGui()
  {
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    processFinishedJobs();

    guiMainMenu();

    if (showWindowImportData)
      guiImportData();
    if (showWindowGenerateData)
      guiGenerateData();

    if (aboutPanel->isShown())
      aboutPanel->buildUI();

    for (auto &p : defaultPanels)
      if (p->isShown())
        p->buildUI();

    for (auto &p : pluginPanels)
      if (p->isShown())
        p->buildUI();

    for (auto &p : advancedPanels)
      if (p->isShown())
        p->buildUI();

    if (showWindowImGuiDemo)
      ImGui::ShowTestWindow();
  }

  void MainWindow::guiMainMenu()
  {
    if (ImGui::BeginMainMenuBar()) {
      guiMainMenuFile();
      guiMainMenuPanels();
      guiMainMenuCamera();
      guiMainMenuHelp();

      ImGui::EndMainMenuBar();
    }
  }

  void MainWindow::guiMainMenuFile()
  {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("(o) Import Data..."))
        showWindowImportData = true;

      if (ImGui::MenuItem("(g) Generate Data..."))
        showWindowGenerateData = true;

      ImGui::Separator();
      ImGui::Separator();

      if (ImGui::MenuItem("(L) Clear Scene"))
        clearScene();

      ImGui::Separator();
      ImGui::Separator();

      bool paused = renderingPaused;
      if (ImGui::Checkbox("(R) Pause Rendering", &paused))
        toggleRenderingPaused();

      auto setFrameAccumulation = [&]() {
        accumulationLimit = accumulationLimit < 0 ? 0 : accumulationLimit;
        scenegraph->child("frameAccumulationLimit") = accumulationLimit;
      };

      if (ImGui::Checkbox("Limit Accumulation", &limitAccumulation)) {
        if (limitAccumulation)
          setFrameAccumulation();
      }

      if (limitAccumulation) {
        if (ImGui::InputInt("Frame Limit", &accumulationLimit))
          setFrameAccumulation();
      } else {
        scenegraph->child("frameAccumulationLimit") = -1;
      }

      ImGui::Separator();
      ImGui::Separator();

      if (ImGui::BeginMenu("Scale Resolution")) {
        float scale = renderResolutionScale;
        if (ImGui::MenuItem("0.25x"))
          renderResolutionScale = 0.25f;
        if (ImGui::MenuItem("0.50x"))
          renderResolutionScale = 0.5f;
        if (ImGui::MenuItem("0.75x"))
          renderResolutionScale = 0.75f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.00x"))
          renderResolutionScale = 1.f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.25x"))
          renderResolutionScale = 1.25f;
        if (ImGui::MenuItem("2.00x"))
          renderResolutionScale = 2.0f;
        if (ImGui::MenuItem("4.00x"))
          renderResolutionScale = 4.0f;

        ImGui::Separator();

        if (ImGui::BeginMenu("custom")) {
          ImGui::InputFloat("x##fb_scaling", &renderResolutionScale);
          ImGui::EndMenu();
        }

        if (scale != renderResolutionScale)
          reshape(windowSize);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scale Resolution while Navigating")) {
        float scale = navRenderResolutionScale;
        if (ImGui::MenuItem("0.25x"))
          navRenderResolutionScale = 0.25f;
        if (ImGui::MenuItem("0.50x"))
          navRenderResolutionScale = 0.5f;
        if (ImGui::MenuItem("0.75x"))
          navRenderResolutionScale = 0.75f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.00x"))
          navRenderResolutionScale = 1.f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.25x"))
          navRenderResolutionScale = 1.25f;
        if (ImGui::MenuItem("2.00x"))
          navRenderResolutionScale = 2.0f;
        if (ImGui::MenuItem("4.00x"))
          navRenderResolutionScale = 4.0f;

        ImGui::Separator();

        if (ImGui::BeginMenu("custom")) {
          ImGui::InputFloat("x##fb_scaling", &navRenderResolutionScale);
          ImGui::EndMenu();
        }

        if (scale != navRenderResolutionScale)
          reshape(windowSize);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Aspect Control")) {
        const float aspect = fixedRenderAspect;
        if (ImGui::MenuItem("Lock"))
          fixedRenderAspect = (float)windowSize.x / windowSize.y;
        if (ImGui::MenuItem("Unlock"))
          fixedRenderAspect = 0.f;
        ImGui::InputFloat("Set", &fixedRenderAspect);
        fixedRenderAspect = std::max(fixedRenderAspect, 0.f);

        if (aspect != fixedRenderAspect) {
          if (fixedRenderAspect > 0.f)
            resizeMode = RESIZE_LETTERBOX;
          else
            resizeMode = RESIZE_KEEPFOVY;
          reshape(windowSize);
        }

        ImGui::EndMenu();
      }

      ImGui::Separator();
      ImGui::Separator();

      if (ImGui::MenuItem("(q) Quit"))
        exitRequestedByUser = true;

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiMainMenuPanels()
  {
    if (ImGui::BeginMenu("Panels")) {
      int panelIndex = 1;
      for (auto &p : defaultPanels) {
        std::stringstream ss;

        if (panelIndex <= 9)
          ss << '(' << std::to_string(panelIndex++) << ") " << p->name();
        else
          ss << p->name();

        bool show = p->isShown();
        if (ImGui::Checkbox(ss.str().c_str(), &show))
          p->toggleShown();
      }

      if (!pluginPanels.empty()) {
        ImGui::Separator();
        ImGui::Text("Plugins");
      }

      for (auto &p : pluginPanels) {
        std::stringstream ss;

        if (panelIndex <= 9)
          ss << '(' << std::to_string(panelIndex++) << ") " << p->name();
        else
          ss << p->name();

        bool show = p->isShown();
        if (ImGui::Checkbox(ss.str().c_str(), &show))
          p->toggleShown();
      }

      ImGui::Separator();

      if (ImGui::BeginMenu("Advanced")) {
        for (auto &p : advancedPanels) {
          bool show = p->isShown();
          if (ImGui::Checkbox(p->name().c_str(), &show))
            p->toggleShown();
        }

        ImGui::EndMenu();
      }

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiMainMenuCamera()
  {
    if (ImGui::BeginMenu("Camera")) {
      ImGui::Checkbox("(A) Auto-Rotate", &animating);

      ImGui::Separator();

      bool orbitMode = (manipulator == inspectCenterManipulator.get());
      bool flyMode   = (manipulator == moveModeManipulator.get());

      if (ImGui::Checkbox("(I) Orbit Camera Mode", &orbitMode))
        manipulator = inspectCenterManipulator.get();

      if (orbitMode)
        ImGui::Checkbox("Anchor 'Up' Direction", &upAnchored);

      if (ImGui::Checkbox("(F) Fly Camera Mode", &flyMode))
        manipulator = moveModeManipulator.get();

      if (ImGui::MenuItem("(r) Reset View"))
        resetView();
      if (ImGui::MenuItem("Create Default View"))
        resetDefaultView();

      ImGui::Separator();

      if (ImGui::BeginMenu("Set Viewport 'Up'...")) {
        if (ImGui::MenuItem("(X) +/- X"))
          viewPort.setViewUpX();
        if (ImGui::MenuItem("(Y) +/- Y"))
          viewPort.setViewUpY();
        if (ImGui::MenuItem("(Z) +/- Z"))
          viewPort.setViewUpZ();
        ImGui::EndMenu();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("(c) Reset Accumulation"))
        viewPort.modified = true;
      if (ImGui::MenuItem("(p) Print View"))
        printViewport();

      ImGui::InputFloat("Motion Speed", &motionSpeed);

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiMainMenuHelp()
  {
    if (ImGui::BeginMenu("Help")) {
      ImGui::Checkbox("Show ImGui Demo", &showWindowImGuiDemo);

      ImGui::Separator();

      if (ImGui::MenuItem("About OSPRay Studio"))
        aboutPanel->toggleShown();

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiRenderStats()
  {
    const float DISTANCE = 10.0f;
    static int corner    = 2;  // <-- encoding comes from imgui_demo.cpp
    ImVec2 window_pos    = ImVec2(
        (corner & 1) ? ImGui::GetIO().DisplaySize.x - DISTANCE : DISTANCE,
        (corner & 2) ? ImGui::GetIO().DisplaySize.y - DISTANCE : DISTANCE);
    ImVec2 window_pos_pivot =
        ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    if (corner != -1)
      ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.3f);  // Transparent background
    if (ImGui::Begin(
            "Rendering Statistics",
            nullptr,
            (corner != -1 ? ImGuiWindowFlags_NoMove : 0) |
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
      ImGui::Text("OSPRay render rate: %.1f fps", renderFPS);
      ImGui::Text("  Total GUI frame rate: %.1f fps", ImGui::GetIO().Framerate);
      ImGui::Text("  Total 3dwidget time: %.1f ms", lastTotalTime * 1000.f);
      ImGui::Text("  GUI time: %.1f ms", lastGUITime * 1000.f);
      ImGui::Text("  display pixel time: %.1f ms", lastDisplayTime * 1000.f);
      auto variance = renderer->getLastVariance();
      ImGui::Text("Variance: %.3f", variance);

      auto eta = scenegraph->estimatedSeconds();

      const bool showProgressBar        = (renderFPS <= 1.f);
      const bool showFrameCompletionEst = (std::isfinite(eta) && eta > 5.f);

      ImGui::Separator();

      if (showProgressBar) {
        ImGui::Text("Frame progress: ");
        ImGui::SameLine();
        ImGui::ProgressBar(frameProgress);
      } else {
        ImGui::NewLine();
      }

      if (showFrameCompletionEst) {
        auto sec = scenegraph->elapsedSeconds();

        ImGui::Text("Total progress: ");

        char str[100];
        if (sec < eta)
          snprintf(str, sizeof(str), "%.1f s / %.1f s", sec, eta);
        else
          snprintf(str, sizeof(str), "%.1f s", sec);

        ImGui::SameLine();
        ImGui::ProgressBar(sec / eta, ImVec2(-1, 0), str);
      } else {
        ImGui::NewLine();
      }
    }

    ImGui::End();
  }

  void MainWindow::guiJobStatusControlPanel()
  {
    auto flags = g_defaultWindowFlags | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize;

    bool &autoImport = autoImportNodesFromFinishedJobs;

    if (ImGui::Begin("Job Scheduler Panel", nullptr, flags)) {
      ImGui::Text("%lu jobs running", jobsInProgress.size());
      ImGui::Text("%lu nodes ready", loadedNodes.size());
      ImGui::NewLine();

      ImGui::Checkbox("auto add to scene", &autoImport);

      if (!autoImport) {
        if (ImGui::Button("Add Loaded Nodes to SceneGraph"))
          importFinishedNodes();
        ImGui::Separator();
        ImGui::Text("Loaded Nodes:");
        ImGui::NewLine();
        for (auto &n : loadedNodes)
          ImGui::Text("%s", n->name().c_str());
      }
    }

    ImGui::End();
  }

  void MainWindow::guiGenerateData()
  {
    ImGui::OpenPopup("Generate Data");
    if (ImGui::BeginPopupModal("Generate Data",
                               &showWindowGenerateData,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Generator Type:   ");
      ImGui::SameLine();

      static int which = 0;
      ImGui::Combo(
          "##1", &which, "spheres\0cube\0cylinders\0volume\0vtkWavelet\0\0", 5);

      static std::string parameters;
      ImGui::Text("Generator Params: ");
      ImGui::SameLine();
      static std::array<char, 512> buf;
      strcpy(buf.data(), parameters.c_str());
      buf[parameters.size()] = '\0';
      if (ImGui::InputText("##2", buf.data(), 511)) {
        parameters = std::string(buf.data());
      }

      ImGui::Separator();

      if (ImGui::Button("OK", ImVec2(120, 0))) {
        // TODO: move this inline-lambda to a named functor instead
        job_scheduler::scheduleJob([=]() {
          job_scheduler::Nodes retval;

          std::string type;

          switch (which) {
          case 0:
            type = "spheres";
            break;
          case 1:
            type = "cube";
            break;
          case 2:
            type = "cylinders";
            break;
          case 3:
            type = "volume";
            break;
          case 4:
            type = "vtkWavelet";
            break;
          default:
            std::cerr << "WAAAAT" << std::endl;
          }

          try {
            auto node = sg::createGeneratorNode(type, parameters);
            retval.push_back(node);
          } catch (...) {
            std::cerr << "Failed to generate data with '" << type << "'!\n";
          }

          return retval;
        });

        showWindowGenerateData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        showWindowGenerateData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  void MainWindow::guiImportData()
  {
    ImGui::OpenPopup("Import Data");
    if (ImGui::BeginPopupModal("Import Data",
                               &showWindowImportData,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      const bool pressed   = ImGui::Button("Choose File...");
      const char *fileName = openFileDialog.chooseFileDialog(pressed);

      static std::string fileToOpen;

      if (strlen(fileName) > 0)
        fileToOpen = fileName;

      ImGui::Text("File:");
      ImGui::SameLine();

      std::array<char, 512> buf;
      strcpy(buf.data(), fileToOpen.c_str());

      ImGui::InputText(
          "", buf.data(), buf.size(), ImGuiInputTextFlags_EnterReturnsTrue);

      fileToOpen = buf.data();

      ImGui::Separator();

      if (ImGui::Button("OK", ImVec2(120, 0))) {
        job_scheduler::scheduleJob([=]() {
          job_scheduler::Nodes retval;

          try {
            auto node = sg::createImporterNode(fileToOpen);
            retval.push_back(node);
          } catch (...) {
            std::cerr << "Failed to open file '" << fileToOpen << "'!\n";
          }

          return retval;
        });

        showWindowImportData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        showWindowImportData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  void MainWindow::setCurrentDeviceParameter(const std::string &param,
                                             int value)
  {
    renderEngine.stop();

    auto device = ospGetCurrentDevice();
    if (device == nullptr)
      throw std::runtime_error("FATAL: could not get current OSPDevice!");

    ospDeviceSet1i(device, param.c_str(), value);
    ospDeviceCommit(device);

    if (!renderingPaused)
      renderEngine.start();
  }

  int MainWindow::progressCallback(const float progress)
  {
    frameProgress = progress;

    // one-shot cancel
    return !cancelRendering.exchange(false);
  }

  int MainWindow::progressCallbackWrapper(void *ptr, const float progress)
  {
    return ((MainWindow *)ptr)->progressCallback(progress);
  }

}  // namespace ospray