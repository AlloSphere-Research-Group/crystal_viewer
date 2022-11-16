#include <iostream>
#include <memory>
#include <vector>

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_ParameterGUI.hpp"

#include "Crystal.hpp"

using namespace al;

struct MyApp : App {
  CrystalViewer viewer;

  ParameterInt dimension{"dimension", "", 3, 3, 5};

  Parameter l1{"l1", "", 0, -5, 5};
  Parameter l2{"l2", "", 0, -5, 5};
  Parameter l3{"l3", "", 0, -5, 5};

  Parameter m1{"m1", "", 1, -2, 2};
  Parameter m2{"m2", "", 0, -2, 2};
  Parameter m3{"m3", "", 0, -2, 2};

  Parameter windowSize{"windowSize", "", 0, 0, 10.f};

  bool showLattice{true};

  void onCreate() override {
    lens().near(0.1).far(25).fovy(45);
    nav().pos(0, 0, 4);

    viewer.init();
    viewer.generate();

    viewer.slice->update(m1.get(), m2.get(), m3.get(), windowSize.get());

    /*auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
    gui = &guiDomain->newGUI();
    *gui << m1 << m2 << m3 << windowSize;*/

    m1.registerChangeCallback([&](float value) {
      viewer.slice->update(value, m2.get(), m3.get(), windowSize.get());
    });

    m2.registerChangeCallback([&](float value) {
      viewer.slice->update(m1.get(), value, m3.get(), windowSize.get());
    });

    m3.registerChangeCallback([&](float value) {
      viewer.slice->update(m1.get(), m2.get(), value, windowSize.get());
    });

    windowSize.registerChangeCallback([&](float value) {
      viewer.slice->update(m1.get(), m2.get(), m3.get(), value);
    });

    // hasCapability(Capability::CAP_2DGUI)
    imguiInit();
  }

  void onAnimate(double dt) override {
    // hasCapability(Capability::CAP_2DGUI)
    imguiBeginFrame();
    ImGui::Begin("Crystal");
    ParameterGUI::draw(&dimension);

    ParameterGUI::draw(&l1);
    ParameterGUI::draw(&l2);
    ParameterGUI::draw(&l3);

    ParameterGUI::draw(&m1);
    ParameterGUI::draw(&m2);
    ParameterGUI::draw(&m3);
    ParameterGUI::draw(&windowSize);
    ImGui::End();
    imguiEndFrame();
  }

  void onDraw(Graphics &g) override {
    g.clear(0);

    g.depthTesting(false);
    g.blending(true);
    g.blendAdd();

    if (showLattice) {
      viewer.drawLattice(g);
    }

    viewer.drawSlice(g);

    // hasCapability(Capability::CAP_2DGUI)
    imguiDraw();
  }

  bool onKeyDown(const Keyboard &k) override {
    switch (k.key()) {
    case ' ':
      showLattice = !showLattice;
      break;
    case '3':
      viewer.generate(3);
      viewer.slice->update(m1.get(), m2.get(), m3.get(), windowSize.get());
      break;
    case '4':
      viewer.generate(4);
      viewer.slice->update(m1.get(), m2.get(), m3.get(), windowSize.get());
      break;
    case '5':
      viewer.generate(5);
      viewer.slice->update(m1.get(), m2.get(), m3.get(), windowSize.get());
      break;
    default:
      return false;
    }
  }

  void onExit() {
    // hasCapability(Capability::CAP_2DGUI)
    imguiShutdown();
  }
};

int main() {
  MyApp app;
  // app.dimensions(600, 400);
  app.dimensions(1920, 1080);
  app.start();
}
