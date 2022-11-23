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

  ParameterBool showLattice{"showLattice", "", 1};
  ParameterInt latticeDim{"latticeDim", "", 3, 3, 5};
  ParameterInt latticeSize{"latticeSize", "", 3, 1, 5};

  ParameterInt basisNum{"basisNum", "", 0, 0, 2};
  Parameter basis1{"basis1", "", 1, -5, 5};
  Parameter basis2{"basis2", "", 0, -5, 5};
  Parameter basis3{"basis3", "", 0, -5, 5};
  Parameter basis4{"basis4", "", 0, -5, 5};
  Parameter basis5{"basis5", "", 0, -5, 5};

  ParameterBool showSlice{"showSlice", "", 1};
  ParameterInt sliceDim{"sliceDim", "", 2, 2, 2};

  ParameterInt millerNum{"millerNum", "", 0, 0, 0};
  ParameterBool intMiller{"intMiller", ""};
  Parameter miller1{"miller1", "", 1, -4, 4};
  Parameter miller2{"miller2", "", 0, -4, 4};
  Parameter miller3{"miller3", "", 0, -4, 4};
  Parameter miller4{"miller4", "", 0, -4, 4};
  Parameter miller5{"miller5", "", 0, -4, 4};

  ParameterBool showBox{"showBox", "", 0};
  Parameter windowDepth{"windowDepth", "", 0, 0, 10.f};

  void onCreate() override {
    lens().near(0.1).far(100).fovy(45);
    nav().pos(0, 0, 4);

    viewer.init();
    viewer.generate(latticeDim.get());

    readBasis(basisNum.get());

    viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(),
                       miller3.get(), miller4.get(), miller5.get(),
                       windowDepth.get());

    latticeDim.registerChangeCallback([&](int value) {
      basisNum.max(value - 1);
      if (basisNum.get() > value - 1) {
        basisNum.set(value - 1);
      }
      sliceDim.max(value - 1);
      if (sliceDim.get() > value - 1) {
        sliceDim.set(value - 1);
      }
      millerNum.max(value - sliceDim.get() - 1);
      if (millerNum.get() > value - sliceDim.get() - 1) {
        millerNum.set(value - sliceDim.get() - 1);
      }

      viewer.generate(value);
      viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(),
                         miller3.get(), miller4.get(), miller5.get(),
                         windowDepth.get());
    });

    latticeSize.registerChangeCallback([&](int value) {
      viewer.createLattice(value);
      viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(),
                         miller3.get(), miller4.get(), miller5.get(),
                         windowDepth.get());
    });

    basisNum.registerChangeCallback([&](int value) { readBasis(value); });

    basis1.registerChangeCallback(
        [&](float value) { viewer.setBasis(value, basisNum.get(), 0); });

    basis2.registerChangeCallback(
        [&](float value) { viewer.setBasis(value, basisNum.get(), 1); });

    basis3.registerChangeCallback(
        [&](float value) { viewer.setBasis(value, basisNum.get(), 2); });

    basis4.registerChangeCallback(
        [&](float value) { viewer.setBasis(value, basisNum.get(), 3); });

    basis5.registerChangeCallback(
        [&](float value) { viewer.setBasis(value, basisNum.get(), 4); });

    sliceDim.registerChangeCallback([&](int value) {
      millerNum.max(latticeDim.get() - value - 1);
      if (millerNum.get() > latticeDim.get() - value - 1) {
        millerNum.set(latticeDim.get() - value - 1);
      }
    });

    intMiller.registerChangeCallback([&](float value) {
      if (value) {
        miller1.setHint("input", -1);
        miller2.setHint("input", -1);
        miller3.setHint("input", -1);
        miller4.setHint("input", -1);
        miller5.setHint("input", -1);
      } else {
        miller1.removeHint("input");
        miller2.removeHint("input");
        miller3.removeHint("input");
        miller4.removeHint("input");
        miller5.removeHint("input");
      }
    });

    millerNum.registerChangeCallback([&](int value) {
      viewer.updateSlice(value, miller1.get(), miller2.get(), miller3.get(),
                         miller4.get(), miller5.get(), windowDepth.get());
    });

    miller1.registerChangeCallback([&](float value) {
      viewer.updateSlice(millerNum.get(), value, miller2.get(), miller3.get(),
                         miller4.get(), miller5.get(), windowDepth.get());
    });

    miller2.registerChangeCallback([&](float value) {
      viewer.updateSlice(millerNum.get(), miller1.get(), value, miller3.get(),
                         miller4.get(), miller5.get(), windowDepth.get());
    });

    miller3.registerChangeCallback([&](float value) {
      viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(), value,
                         miller4.get(), miller5.get(), windowDepth.get());
    });

    miller4.registerChangeCallback([&](float value) {
      viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(),
                         miller3.get(), value, miller5.get(),
                         windowDepth.get());
    });

    miller5.registerChangeCallback([&](float value) {
      viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(),
                         miller3.get(), miller4.get(), value,
                         windowDepth.get());
    });

    windowDepth.registerChangeCallback([&](float value) {
      viewer.updateSlice(millerNum.get(), miller1.get(), miller2.get(),
                         miller3.get(), miller4.get(), miller5.get(), value);
    });

    // hasCapability(Capability::CAP_2DGUI)
    imguiInit();
  }

  void readBasis(int basisNum) {
    basis1.setNoCalls(viewer.getBasis(basisNum, 0));
    basis2.setNoCalls(viewer.getBasis(basisNum, 1));
    basis3.setNoCalls(viewer.getBasis(basisNum, 2));
    if (viewer.dim > 3) {
      basis4.setNoCalls(viewer.getBasis(basisNum, 3));
      if (viewer.dim > 4) {
        basis5.setNoCalls(viewer.getBasis(basisNum, 4));
      }
    }
  }

  void onAnimate(double dt) override {
    // hasCapability(Capability::CAP_2DGUI)
    imguiBeginFrame();
    ImGui::Begin("Crystal");

    ParameterGUI::draw(&showLattice);
    ParameterGUI::draw(&latticeDim);
    ParameterGUI::draw(&latticeSize);

    ImGui::NewLine();

    if (ImGui::CollapsingHeader("Edit Basis Vector",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ParameterGUI::draw(&basisNum);
      ImGui::Indent();
      ParameterGUI::draw(&basis1);
      ParameterGUI::draw(&basis2);
      ParameterGUI::draw(&basis3);
      if (latticeDim.get() > 3) {
        ParameterGUI::draw(&basis4);
        if (latticeDim.get() > 4) {
          ParameterGUI::draw(&basis5);
        }
      }
      ImGui::Unindent();
    }

    ImGui::NewLine();

    ParameterGUI::draw(&showSlice);
    if (showSlice.get()) {
      ParameterGUI::draw(&sliceDim);

      ImGui::NewLine();

      ParameterGUI::draw(&millerNum);
      ImGui::Indent();
      ParameterGUI::draw(&intMiller);
      ParameterGUI::draw(&miller1);
      ParameterGUI::draw(&miller2);
      ParameterGUI::draw(&miller3);
      if (latticeDim.get() > 3) {
        ParameterGUI::draw(&miller4);
        if (latticeDim.get() > 4) {
          ParameterGUI::draw(&miller5);
        }
      }
      ImGui::Unindent();
    }

    ImGui::NewLine();

    ParameterGUI::draw(&showBox);
    ParameterGUI::draw(&windowDepth);
    ImGui::End();
    imguiEndFrame();
  }

  void onDraw(Graphics &g) override {
    g.clear(0);

    g.depthTesting(false);
    g.blending(true);
    g.blendAdd();

    if (showLattice.get()) {
      viewer.drawLattice(g);
    }

    if (showBox.get()) {
      viewer.drawBox(g);
    }

    if (showSlice.get()) {
      viewer.drawSlice(g);
    }

    // hasCapability(Capability::CAP_2DGUI)
    imguiDraw();
  }

  /*bool onKeyDown(const Keyboard &k) override {
    switch (k.key()) {
    case ' ':
      showLattice.set(!showLattice.get());
      break;
    case '3':
      viewer.generate(3);
      viewer.slice->update(miller1.get(), miller2.get(), miller3.get(),
                           windowDepth.get());
      break;
    case '4':
      viewer.generate(4);
      viewer.slice->update(miller1.get(), miller2.get(), miller3.get(),
                           windowDepth.get());
      break;
    case '5':
      viewer.generate(5);
      viewer.slice->update(miller1.get(), miller2.get(), miller3.get(),
                           windowDepth.get());
      break;
    default:
      return false;
    }
  }*/

  void onExit() {
    // hasCapability(Capability::CAP_2DGUI)
    imguiShutdown();
  }
};

int main() {
  MyApp app;
  // app.dimensions(1200, 800);
  app.dimensions(1920, 1080);
  app.start();
}
