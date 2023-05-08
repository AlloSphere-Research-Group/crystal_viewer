#include <iostream>
#include <memory>
#include <vector>

#include "al/app/al_DistributedApp.hpp"
#include "al/ui/al_ParameterGUI.hpp"

#include "CrystalViewer.hpp"

using namespace al;

struct State {
  Pose pose;
};

struct CrystalApp : DistributedAppWithState<State> {
  CrystalViewer viewer;

  void onCreate() override {
    lens().near(0.1).far(100).fovy(45);
    nav().pos(0, 0, 4);

    if (!viewer.init(parameterServer())) {
      std::cerr << "Crystal viewer failed to initialize" << std::endl;
      quit();
    }

    if (hasCapability(Capability::CAP_2DGUI)) {
      imguiInit();
    }
  }

  void onAnimate(double dt) override {
    if (hasCapability(Capability::CAP_2DGUI)) {
      imguiBeginFrame();

      viewer.setGUIFrame(navControl());

      imguiEndFrame();
    }
    if (isPrimary()) {
      state().pose = nav();
    } else {
      nav().set(state().pose);
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0);

    viewer.draw(g, nav());

    if (hasCapability(Capability::CAP_2DGUI)) {
      imguiDraw();
    }
  }

  bool onMouseMove(const Mouse &m) override {
    viewer.getSlice()->pickableManager.onMouseMove(graphics(), m, width(),
                                                   height());
    return true;
  }

  bool onMouseDown(const Mouse &m) override {
    viewer.getSlice()->pickableManager.onMouseDown(graphics(), m, width(),
                                                   height());
    viewer.getSlice()->updatePickables();
    return true;
  }

  /*bool onMouseDrag(const Mouse &m) override {
    viewer.getSlice()->pickableManager.onMouseDrag(graphics(), m, width(),
                                                   height());
    return true;
  }*/

  bool onMouseUp(const Mouse &m) override {
    viewer.getSlice()->pickableManager.onMouseUp(graphics(), m, width(),
                                                 height());
    return true;
  }

  void onExit() {
    if (hasCapability(Capability::CAP_2DGUI)) {
      imguiShutdown();
    }
  }
};

int main() {
  CrystalApp app;
  app.dimensions(1200, 800);
  // app.dimensions(1920, 1080);
  app.start();
}
