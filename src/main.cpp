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

  ParameterBool autoUpdate{"autoUpdate", "", 1};
  Trigger manualUpdate{"Update", ""};

  ParameterBool showLattice{"showLattice", "", 1};
  ParameterInt latticeDim{"latticeDim", "", 3, 3, 5};
  ParameterInt latticeSize{"latticeSize", "", 3, 1, 5};

  ParameterInt basisNum{"basisNum", "", 0, 0, 2};
  Parameter basis1{"basis1", "", 1, -5, 5};
  Parameter basis2{"basis2", "", 0, -5, 5};
  Parameter basis3{"basis3", "", 0, -5, 5};
  Parameter basis4{"basis4", "", 0, -5, 5};
  Parameter basis5{"basis5", "", 0, -5, 5};

  Trigger resetBasis{"resetBasis", ""};

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
  Parameter windowSize{"windowSize", "", 15.f, 0, 30.f};
  Parameter windowDepth{"windowDepth", "", 0, 0, 10.f};

  std::string dataDir;
  char fileName[128] = "01";
  Trigger exportTxt{"ExportTxt", ""};
  Trigger exportJson{"ExportJson", ""};

  void onCreate() override {
    lens().near(0.1).far(100).fovy(45);
    nav().pos(0, 0, 4);

    viewer.init();
    viewer.generate(latticeDim.get());

    dataDir = File::conformPathToOS(File::currentPath());

    // remove bin directory from path
    auto posBin = dataDir.find("bin");
    dataDir = dataDir.substr(0, posBin);
    dataDir = File::conformPathToOS(dataDir + "data/");

    if (!al::File::exists(dataDir)) {
      if (!al::Dir::make(dataDir)) {
        std::cerr << "Unable to create directory: " << dataDir << std::endl;
        quit();
      }
    }

    manualUpdate.registerChangeCallback([&](bool value) { viewer.update(); });

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
    });

    latticeSize.registerChangeCallback(
        [&](int value) { viewer.createLattice(value); });

    basisNum.registerChangeCallback([&](int value) { readBasis(value); });

    // TODO: add in callUpdate
    basis1.registerChangeCallback([&](float value) {
      viewer.setBasis(value, basisNum.get(), 0, autoUpdate.get());
    });

    basis2.registerChangeCallback([&](float value) {
      viewer.setBasis(value, basisNum.get(), 1, autoUpdate.get());
    });

    basis3.registerChangeCallback([&](float value) {
      viewer.setBasis(value, basisNum.get(), 2, autoUpdate.get());
    });

    basis4.registerChangeCallback([&](float value) {
      viewer.setBasis(value, basisNum.get(), 3, autoUpdate.get());
    });

    basis5.registerChangeCallback([&](float value) {
      viewer.setBasis(value, basisNum.get(), 4, autoUpdate.get());
    });

    resetBasis.registerChangeCallback([&](bool value) {
      viewer.resetBasis();
      readBasis(basisNum.get());
    });

    sliceDim.registerChangeCallback([&](int value) {
      millerNum.max(latticeDim.get() - value - 1);
      if (millerNum.get() > latticeDim.get() - value - 1) {
        millerNum.set(latticeDim.get() - value - 1);
      }

      // TODO: add 3d slice transition
    });

    intMiller.registerChangeCallback([&](float value) {
      if (value) {
        miller1.setHint("input", -1);
        miller2.setHint("input", -1);
        miller3.setHint("input", -1);
        miller4.setHint("input", -1);
        miller5.setHint("input", -1);

        viewer.roundMiller(autoUpdate.get());
        readMiller(millerNum.get());
      } else {
        miller1.removeHint("input");
        miller2.removeHint("input");
        miller3.removeHint("input");
        miller4.removeHint("input");
        miller5.removeHint("input");
      }
    });

    millerNum.registerChangeCallback([&](int value) { readMiller(value); });

    miller1.registerChangeCallback([&](float value) {
      viewer.setMiller(value, millerNum.get(), 0, autoUpdate.get());
    });

    miller2.registerChangeCallback([&](float value) {
      viewer.setMiller(value, millerNum.get(), 1, autoUpdate.get());
    });

    miller3.registerChangeCallback([&](float value) {
      viewer.setMiller(value, millerNum.get(), 2, autoUpdate.get());
    });

    miller4.registerChangeCallback([&](float value) {
      viewer.setMiller(value, millerNum.get(), 3, autoUpdate.get());
    });

    miller5.registerChangeCallback([&](float value) {
      viewer.setMiller(value, millerNum.get(), 4, autoUpdate.get());
    });

    windowDepth.registerChangeCallback([&](float value) {
      viewer.setWindow(windowSize.get(), value);
      viewer.updateSlice();
    });

    windowSize.registerChangeCallback(
        [&](float value) { viewer.setWindow(value, windowDepth.get()); });

    exportTxt.registerChangeCallback([&](bool value) {
      std::string newPath = File::conformPathToOS(dataDir + fileName);
      viewer.exportSliceTxt(newPath);
    });

    exportJson.registerChangeCallback([&](bool value) {
      std::string newPath = File::conformPathToOS(dataDir + fileName);
      viewer.exportSliceJson(newPath);
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

  void readMiller(int millerNum) {
    miller1.setNoCalls(viewer.getMiller(millerNum, 0));
    miller2.setNoCalls(viewer.getMiller(millerNum, 1));
    miller3.setNoCalls(viewer.getMiller(millerNum, 2));
    miller4.setNoCalls(viewer.getMiller(millerNum, 3));
    miller5.setNoCalls(viewer.getMiller(millerNum, 4));
  }

  void onAnimate(double dt) override {
    // hasCapability(Capability::CAP_2DGUI)
    imguiBeginFrame();
    ImGui::Begin("Crystal");

    ParameterGUI::draw(&autoUpdate);
    if (!autoUpdate.get()) {
      ImGui::SameLine();
      ParameterGUI::draw(&manualUpdate);
    }

    ParameterGUI::draw(&showLattice);
    ParameterGUI::draw(&latticeDim);
    ParameterGUI::draw(&latticeSize);

    ImGui::NewLine();

    if (ImGui::CollapsingHeader("Edit Basis Vector",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg,
                            (ImVec4)ImColor::HSV(0.15f, 0.5f, 0.3f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                            (ImVec4)ImColor::HSV(0.15f, 0.6f, 0.3f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
                            (ImVec4)ImColor::HSV(0.15f, 0.7f, 0.3f));
      ImGui::PushStyleColor(ImGuiCol_SliderGrab,
                            (ImVec4)ImColor::HSV(0.15f, 0.9f, 0.5f));
      ParameterGUI::draw(&basisNum);
      ImGui::PopStyleColor(4);

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
      ParameterGUI::draw(&resetBasis);
      ImGui::Unindent();
    }

    ImGui::NewLine();

    ParameterGUI::draw(&showSlice);
    if (showSlice.get()) {
      ParameterGUI::draw(&sliceDim);

      ImGui::NewLine();

      ImGui::PushStyleColor(ImGuiCol_FrameBg,
                            (ImVec4)ImColor::HSV(0.15f, 0.5f, 0.3f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                            (ImVec4)ImColor::HSV(0.15f, 0.6f, 0.3f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
                            (ImVec4)ImColor::HSV(0.15f, 0.7f, 0.3f));
      ImGui::PushStyleColor(ImGuiCol_SliderGrab,
                            (ImVec4)ImColor::HSV(0.15f, 0.9f, 0.5f));
      ParameterGUI::draw(&millerNum);
      ImGui::PopStyleColor(4);

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
    ParameterGUI::draw(&windowSize);
    ParameterGUI::draw(&windowDepth);

    ImGui::NewLine();

    ImGui::Text("%s", dataDir.c_str());
    ImGui::InputText("fileName", fileName, IM_ARRAYSIZE(fileName));
    if (ImGui::IsItemActive()) {
      navControl().active(false);
    } else {
      navControl().active(true);
    }

    ParameterGUI::draw(&exportTxt);
    ImGui::SameLine();
    ParameterGUI::draw(&exportJson);

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

  void onExit() {
    // hasCapability(Capability::CAP_2DGUI)
    imguiShutdown();
  }
};

int main() {
  MyApp app;
  app.dimensions(1200, 800);
  // app.dimensions(1920, 1080);
  app.start();
}
