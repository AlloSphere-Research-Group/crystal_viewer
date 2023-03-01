#include <iostream>
#include <memory>
#include <vector>

#include "al/app/al_App.hpp"
#include "al/ui/al_ParameterGUI.hpp"

#include "CrystalViewer.hpp"

using namespace al;

struct MyApp : App {
  CrystalViewer viewer;

  ParameterBool autoUpdate{"autoUpdate", "", 1};
  Trigger manualUpdate{"Update", ""};

  ParameterInt crystalDim{"crystalDim", "", 3, 3, 5};
  ParameterInt latticeSize{"latticeSize", "", 1, 1, 5};

  ParameterInt basisNum{"basisNum", "", 0, 0, 2};
  Parameter basis1{"basis1", "", 1, -5, 5};
  Parameter basis2{"basis2", "", 0, -5, 5};
  Parameter basis3{"basis3", "", 0, -5, 5};
  Parameter basis4{"basis4", "", 0, -5, 5};
  Parameter basis5{"basis5", "", 0, -5, 5};
  Trigger resetBasis{"resetBasis", ""};

  ParameterBool axis1{"X", "", 1};
  ParameterBool axis2{"Y", "", 1};
  ParameterBool axis3{"Z", "", 1};
  ParameterBool axis4{"W", "", 0};
  ParameterBool axis5{"A5", "", 0};

  Parameter particle1{"particle1", "", 0.5, 0, 1};
  Parameter particle2{"particle2", "", 0.5, 0, 1};
  Parameter particle3{"particle3", "", 0.5, 0, 1};
  Parameter particle4{"particle4", "", 0.5, 0, 1};
  Parameter particle5{"particle5", "", 0.5, 0, 1};
  Parameter particleSphereSize{"particleSphereSize", "", 0.02, 0.001, 1};
  ParameterColor particleSphereColor{"particleSphereColor", "",
                                     Color(1.f, 0.2f, 0.2f, 0.8f)};
  Trigger addParticle{"addParticle", ""};
  Trigger removeParticle{"removeParticle", ""};

  ParameterBool showLattice{"showLattice", "", 1};
  ParameterBool showLatticeEdge{"showLatticeEdge", "", 0};
  ParameterBool showSlice{"showSlice", "", 1};
  ParameterBool showSliceEdge{"showSliceEdge", "", 0};
  ParameterBool showSlicePlane{"showSlicePlane", "", 0};
  ParameterBool lockCameraToSlice{"lockCameraToSlice", "", 0};

  Parameter latticeSphereSize{"latticeSphereSize", "", 0.02, 0.001, 1};
  ParameterColor latticeSphereColor{"latticeSphereColor", "", Color(1.f, 0.8f)};
  ParameterColor latticeEdgeColor{"latticeEdgeColor", "", Color(1.f, 0.5f)};

  Parameter sliceSphereSize{"sliceSphereSize", 0.04, 0.001, 1};
  ParameterColor sliceSphereColor{"sliceSphereColor", "",
                                  Color(1.f, 0.f, 0.f, 1.f)};
  ParameterColor sliceEdgeColor{"sliceEdgeColor", "", Color(1.f, 0.5f)};

  Parameter slicePlaneSize{"slicePlaneSize", "", 15.f, 0, 30.f};
  ParameterColor slicePlaneColor{"slicePlaneColor", "",
                                 Color(0.3f, 0.3f, 1.f, 0.3f)};

  ParameterInt sliceDim{"sliceDim", "", 2, 2, 2};
  Parameter sliceDepth{"sliceDepth", "", 0, 0, 10.f};

  ParameterInt millerNum{"millerNum", "", 0, 0, 0};
  ParameterBool intMiller{"intMiller", ""};
  Parameter miller1{"miller1", "", 1, -5, 5};
  Parameter miller2{"miller2", "", 0, -5, 5};
  Parameter miller3{"miller3", "", 0, -5, 5};
  Parameter miller4{"miller4", "", 0, -5, 5};
  Parameter miller5{"miller5", "", 0, -5, 5};

  std::string dataDir;
  char filePath[128]{};
  char fileName[128]{"01"};
  Trigger exportTxt{"ExportTxt", ""};
  Trigger exportJson{"ExportJson", ""};

  void onCreate() override {
    lens().near(0.1).far(100).fovy(45);
    nav().pos(0, 0, 4);

    viewer.init();
    viewer.generate(crystalDim.get());

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

    dataDir.copy(filePath, sizeof filePath - 1);

    particleSphereColor.setHint("showAlpha", true);
    particleSphereColor.setHint("hsv", true);
    latticeSphereColor.setHint("showAlpha", true);
    latticeSphereColor.setHint("hsv", true);
    latticeEdgeColor.setHint("showAlpha", true);
    latticeEdgeColor.setHint("hsv", true);
    sliceSphereColor.setHint("showAlpha", true);
    sliceSphereColor.setHint("hsv", true);
    sliceEdgeColor.setHint("showAlpha", true);
    sliceEdgeColor.setHint("hsv", true);
    slicePlaneColor.setHint("showAlpha", true);
    slicePlaneColor.setHint("hsv", true);

    autoUpdate.registerChangeCallback(
        [&](bool value) { viewer.setAutoUpdate(value); });
    manualUpdate.registerChangeCallback([&](bool value) { viewer.update(); });

    crystalDim.registerChangeCallback([&](int value) {
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

      latticeSize.setNoCalls(1);
    });

    latticeSize.registerChangeCallback(
        [&](int value) { viewer.createLattice(value); });

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

    resetBasis.registerChangeCallback([&](bool value) {
      viewer.resetBasis();
      readBasis(basisNum.get());
    });

    axis1.registerChangeCallback([&](bool value) {
      std::vector<int> newAxis;
      if (value) {
        newAxis.push_back(0);
      }
      if (axis2.get()) {
        newAxis.push_back(1);
      }
      if (axis3.get()) {
        newAxis.push_back(2);
      }
      if (crystalDim.get() > 3) {
        if (axis4.get()) {
          newAxis.push_back(3);
        }
        if (crystalDim.get() > 4) {
          if (axis5.get()) {
            newAxis.push_back(4);
          }
        }
      }

      viewer.setAxis(newAxis);
    });

    axis2.registerChangeCallback([&](bool value) {
      std::vector<int> newAxis;
      if (axis1.get()) {
        newAxis.push_back(0);
      }
      if (value) {
        newAxis.push_back(1);
      }
      if (axis3.get()) {
        newAxis.push_back(2);
      }
      if (crystalDim.get() > 3) {
        if (axis4.get()) {
          newAxis.push_back(3);
        }
        if (crystalDim.get() > 4) {
          if (axis5.get()) {
            newAxis.push_back(4);
          }
        }
      }

      viewer.setAxis(newAxis);
    });

    axis3.registerChangeCallback([&](bool value) {
      std::vector<int> newAxis;
      if (axis1.get()) {
        newAxis.push_back(0);
      }
      if (axis2.get()) {
        newAxis.push_back(1);
      }
      if (value) {
        newAxis.push_back(2);
      }
      if (crystalDim.get() > 3) {
        if (axis4.get()) {
          newAxis.push_back(3);
        }
        if (crystalDim.get() > 4) {
          if (axis5.get()) {
            newAxis.push_back(4);
          }
        }
      }

      viewer.setAxis(newAxis);
    });

    axis4.registerChangeCallback([&](bool value) {
      std::vector<int> newAxis;
      if (axis1.get()) {
        newAxis.push_back(0);
      }
      if (axis2.get()) {
        newAxis.push_back(1);
      }
      if (axis3.get()) {
        newAxis.push_back(2);
      }
      if (crystalDim.get() > 3) {
        if (value) {
          newAxis.push_back(3);
        }
        if (crystalDim.get() > 4) {
          if (axis5.get()) {
            newAxis.push_back(4);
          }
        }
      }

      viewer.setAxis(newAxis);
    });

    axis5.registerChangeCallback([&](bool value) {
      std::vector<int> newAxis;
      if (axis1.get()) {
        newAxis.push_back(0);
      }
      if (axis2.get()) {
        newAxis.push_back(1);
      }
      if (axis3.get()) {
        newAxis.push_back(2);
      }
      if (crystalDim.get() > 3) {
        if (axis4.get()) {
          newAxis.push_back(3);
        }
        if (crystalDim.get() > 4) {
          if (value) {
            newAxis.push_back(4);
          }
        }
      }

      viewer.setAxis(newAxis);
    });

    particle1.registerChangeCallback(
        [&](float value) { viewer.setParticle(value, 0); });

    particle2.registerChangeCallback(
        [&](float value) { viewer.setParticle(value, 1); });

    particle3.registerChangeCallback(
        [&](float value) { viewer.setParticle(value, 2); });

    particle4.registerChangeCallback(
        [&](float value) { viewer.setParticle(value, 3); });

    particle5.registerChangeCallback(
        [&](float value) { viewer.setParticle(value, 4); });

    particleSphereSize.registerChangeCallback(
        [&](float value) { viewer.setParticlesphereSize(value); });

    particleSphereColor.registerChangeCallback(
        [&](Color value) { viewer.setParticleSphereColor(value); });

    addParticle.registerChangeCallback(
        [&](bool value) { viewer.addParticle(); });

    removeParticle.registerChangeCallback(
        [&](bool value) { viewer.removeParticle(); });

    showLatticeEdge.registerChangeCallback(
        [&](bool value) { viewer.enableLatticeEdges(value); });

    latticeSphereSize.registerChangeCallback(
        [&](float value) { viewer.setLatticeSphereSize(value); });

    latticeSphereColor.registerChangeCallback(
        [&](Color value) { viewer.setLatticeSphereColor(value); });

    latticeEdgeColor.registerChangeCallback(
        [&](Color value) { viewer.setLatticeEdgeColor(value); });

    sliceSphereSize.registerChangeCallback(
        [&](float value) { viewer.setSliceSphereSize(value); });

    sliceSphereColor.registerChangeCallback(
        [&](Color value) { viewer.setSliceSphereColor(value); });

    sliceEdgeColor.registerChangeCallback(
        [&](Color value) { viewer.setSliceEdgeColor(value); });

    slicePlaneSize.registerChangeCallback(
        [&](float value) { viewer.setWindow(value, sliceDepth.get()); });

    slicePlaneColor.registerChangeCallback(
        [&](Color value) { viewer.setSlicePlaneColor(value); });

    sliceDim.registerChangeCallback([&](int value) {
      millerNum.max(crystalDim.get() - value - 1);
      if (millerNum.get() > crystalDim.get() - value - 1) {
        millerNum.set(crystalDim.get() - value - 1);
      }

      // TODO: add 3d slice transition
    });

    sliceDepth.registerChangeCallback([&](float value) {
      viewer.setWindow(slicePlaneSize.get(), value);
      viewer.updateSlice();
    });

    intMiller.registerChangeCallback([&](float value) {
      if (value) {
        miller1.setHint("input", -1);
        miller2.setHint("input", -1);
        miller3.setHint("input", -1);
        miller4.setHint("input", -1);
        miller5.setHint("input", -1);

        viewer.roundMiller();
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

    miller1.registerChangeCallback(
        [&](float value) { viewer.setMiller(value, millerNum.get(), 0); });

    miller2.registerChangeCallback(
        [&](float value) { viewer.setMiller(value, millerNum.get(), 1); });

    miller3.registerChangeCallback(
        [&](float value) { viewer.setMiller(value, millerNum.get(), 2); });

    miller4.registerChangeCallback(
        [&](float value) { viewer.setMiller(value, millerNum.get(), 3); });

    miller5.registerChangeCallback(
        [&](float value) { viewer.setMiller(value, millerNum.get(), 4); });

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
    if (viewer.getDim() > 3) {
      basis4.setNoCalls(viewer.getBasis(basisNum, 3));
      if (viewer.getDim() > 4) {
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

    ParameterGUI::draw(&crystalDim);
    ParameterGUI::draw(&latticeSize);

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
      if (crystalDim.get() > 3) {
        ParameterGUI::draw(&basis4);
        if (crystalDim.get() > 4) {
          ParameterGUI::draw(&basis5);
        }
      }
      ParameterGUI::draw(&resetBasis);
      ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Edit Viewing Dimension",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ParameterGUI::draw(&axis1);
      ImGui::SameLine();
      ParameterGUI::draw(&axis2);
      ImGui::SameLine();
      ParameterGUI::draw(&axis3);
      if (crystalDim.get() > 3) {
        ImGui::SameLine();
        ParameterGUI::draw(&axis4);
        if (crystalDim.get() > 4) {
          ImGui::SameLine();
          ParameterGUI::draw(&axis5);
        }
      }
    }

    if (ImGui::CollapsingHeader("Add Particles",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ParameterGUI::draw(&particle1);
      ParameterGUI::draw(&particle2);
      ParameterGUI::draw(&particle3);
      if (crystalDim.get() > 3) {
        ParameterGUI::draw(&particle4);
        if (crystalDim.get() > 4) {
          ParameterGUI::draw(&particle5);
        }
      }
      ParameterGUI::draw(&particleSphereSize);
      ParameterGUI::draw(&particleSphereColor);
      ParameterGUI::draw(&addParticle);
      ImGui::SameLine();
      ParameterGUI::draw(&removeParticle);
    }

    ImGui::NewLine();

    ParameterGUI::draw(&showLattice);
    if (showLattice.get()) {
      ImGui::SameLine();
      ParameterGUI::draw(&showLatticeEdge);
    }

    ParameterGUI::draw(&showSlice);
    ImGui::SameLine();
    ParameterGUI::draw(&showSliceEdge);

    ParameterGUI::draw(&showSlicePlane);
    ImGui::SameLine();
    ParameterGUI::draw(&lockCameraToSlice);

    if (ImGui::CollapsingHeader("Edit Display Settings",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      if (ImGui::BeginTabBar("displaySettings", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Lattice")) {
          ParameterGUI::draw(&latticeSphereSize);
          ParameterGUI::draw(&latticeSphereColor);
          ParameterGUI::draw(&latticeEdgeColor);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Slice")) {
          ParameterGUI::draw(&sliceSphereSize);
          ParameterGUI::draw(&sliceSphereColor);
          ParameterGUI::draw(&sliceEdgeColor);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("SlicingPlane")) {
          ParameterGUI::draw(&slicePlaneSize);
          ParameterGUI::draw(&slicePlaneColor);
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
    }

    ImGui::NewLine();

    if (showSlice.get()) {
      ParameterGUI::draw(&sliceDim);
      ParameterGUI::draw(&sliceDepth);

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
      if (crystalDim.get() > 3) {
        ParameterGUI::draw(&miller4);
        if (crystalDim.get() > 4) {
          ParameterGUI::draw(&miller5);
        }
      }
      ImGui::Unindent();
    }

    ImGui::NewLine();

    ImGui::InputText("filePath", filePath, IM_ARRAYSIZE(filePath));
    if (ImGui::IsItemActive()) {
      navControl().active(false);
    } else {
      navControl().active(true);
    }
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

    g.pushMatrix();

    if (lockCameraToSlice.get()) {
      Quatf rot = Quatf::getRotationTo(viewer.getNormal(), nav().uf());
      g.rotate(rot);
    }

    if (showLattice.get()) {
      if (showLatticeEdge.get()) {
        viewer.drawLatticeEdges(g);
      }
      viewer.drawLattice(g);

      viewer.drawLatticeParticles(g);
    }

    if (showSlicePlane.get()) {
      viewer.drawBox(g);
    }

    if (showSlice.get()) {
      if (showSliceEdge.get()) {
        viewer.drawSliceEdges(g);
      }
      viewer.drawSlice(g);
    }

    g.popMatrix();

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
