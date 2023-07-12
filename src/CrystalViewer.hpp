#ifndef CRYSTAL_VIEWER_HPP
#define CRYSTAL_VIEWER_HPP

#include <memory>

#include "al/graphics/al_BufferObject.hpp"
#include "al/graphics/al_Graphics.hpp"
#include "al/graphics/al_Shader.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_VAOMesh.hpp"
#include "al/io/al_ControlNav.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Quat.hpp"
#include "al/math/al_Vec.hpp"
#include "al/ui/al_ParameterGUI.hpp"

#include "Lattice.hpp"
#include "Slice.hpp"

class CrystalViewer {
public:
  bool init() {
    generate(crystalDim.get(), sliceDim.get());

    addSphere(latticeSphere, 1.0);

    latticeSphere.update();

    latticeVertices.bufferType(GL_ARRAY_BUFFER);
    latticeVertices.usage(GL_DYNAMIC_DRAW);
    latticeVertices.create();

    latticeColors.bufferType(GL_ARRAY_BUFFER);
    latticeColors.usage(GL_DYNAMIC_DRAW);
    latticeColors.create();

    addSphere(sliceSphere, 1.0);
    sliceSphere.update();

    sliceVertices.bufferType(GL_ARRAY_BUFFER);
    sliceVertices.usage(GL_DYNAMIC_DRAW);
    sliceVertices.create();

    sliceColors.bufferType(GL_ARRAY_BUFFER);
    sliceColors.usage(GL_DYNAMIC_DRAW);
    sliceColors.create();

    auto &latticeVAO = latticeSphere.vao();
    latticeVAO.bind();
    latticeVAO.enableAttrib(1);
    latticeVAO.attribPointer(1, latticeVertices, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    latticeVAO.enableAttrib(2);
    latticeVAO.attribPointer(2, latticeColors, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(2, 1);

    auto &sliceVAO = sliceSphere.vao();
    sliceVAO.bind();
    sliceVAO.enableAttrib(1);
    sliceVAO.attribPointer(1, sliceVertices, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    sliceVAO.enableAttrib(2);
    sliceVAO.attribPointer(2, sliceColors, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(2, 1);

    latticeEdge.vertex(Vec3f(0));
    latticeEdge.vertex(Vec3f(1, 1, 1));
    latticeEdge.update();

    latticeEdgeStarts.bufferType(GL_ARRAY_BUFFER);
    latticeEdgeStarts.usage(GL_DYNAMIC_DRAW);
    latticeEdgeStarts.create();

    latticeEdgeEnds.bufferType(GL_ARRAY_BUFFER);
    latticeEdgeEnds.usage(GL_DYNAMIC_DRAW);
    latticeEdgeEnds.create();

    sliceEdge.vertex(Vec3f(0));
    sliceEdge.vertex(Vec3f(1, 1, 1));
    sliceEdge.update();

    sliceEdgeStarts.bufferType(GL_ARRAY_BUFFER);
    sliceEdgeStarts.usage(GL_DYNAMIC_DRAW);
    sliceEdgeStarts.create();

    sliceEdgeEnds.bufferType(GL_ARRAY_BUFFER);
    sliceEdgeEnds.usage(GL_DYNAMIC_DRAW);
    sliceEdgeEnds.create();

    auto &latticeEdgeVAO = latticeEdge.vao();
    latticeEdgeVAO.bind();
    latticeEdgeVAO.enableAttrib(1);
    latticeEdgeVAO.attribPointer(1, latticeEdgeStarts, 3, GL_FLOAT, GL_FALSE, 0,
                                 0);
    glVertexAttribDivisor(1, 1);

    latticeEdgeVAO.enableAttrib(2);
    latticeEdgeVAO.attribPointer(2, latticeEdgeEnds, 3, GL_FLOAT, GL_FALSE, 0,
                                 0);
    glVertexAttribDivisor(2, 1);

    auto &sliceEdgeVAO = sliceEdge.vao();
    sliceEdgeVAO.bind();
    sliceEdgeVAO.enableAttrib(1);
    sliceEdgeVAO.attribPointer(1, sliceEdgeStarts, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    sliceEdgeVAO.enableAttrib(2);
    sliceEdgeVAO.attribPointer(2, sliceEdgeEnds, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(2, 1);

    searchPaths.addAppPaths();
    searchPaths.addRelativePath("src", false);
    searchPaths.addRelativePath("../src", false);
    // searchPaths.print();

    reloadShaders();

    return true;
  }

  void reloadShaders() {
    loadShader(instancing_shader, "instancing_vert.glsl",
               "instancing_frag.glsl");
    loadShader(edge_instancing_shader, "edge_instancing_vert.glsl",
               "edge_instancing_frag.glsl", "edge_instancing_geom.glsl");
  }

  void generate(int newDim, int newSliceDim) {
    switch (newDim) {
    case 3: {
      auto newLattice = std::make_shared<Lattice<3>>(lattice);
      auto newSlice = std::make_shared<Slice<3, 2>>(slice, newLattice);

      lattice = newLattice;
      slice = newSlice;
      break;
    }
    case 4: {
      auto newLattice = std::make_shared<Lattice<4>>(lattice);
      if (newSliceDim == 2) {
        auto newSlice = std::make_shared<Slice<4, 2>>(slice, newLattice);
        slice = newSlice;
      } else if (newSliceDim == 3) {
        auto newSlice = std::make_shared<Slice<4, 3>>(slice, newLattice);
        slice = newSlice;
      }
      lattice = newLattice;
      break;
    }
    case 5: {
      auto newLattice = std::make_shared<Lattice<5>>(lattice);
      if (newSliceDim == 2) {
        auto newSlice = std::make_shared<Slice<5, 2>>(slice, newLattice);
        slice = newSlice;
      } else if (newSliceDim == 3) {
        auto newSlice = std::make_shared<Slice<5, 3>>(slice, newLattice);
        slice = newSlice;
      }
      lattice = newLattice;
      break;
    }
    default:
      std::cerr << "Dimension " << newDim << " not supported." << std::endl;
      return;
    }
  }

  void setBasis(float value, int basisNum, unsigned int vecIdx) {
    lattice->setBasis(value, basisNum, vecIdx);
    slice->update();
  }

  void draw(Graphics &g, Nav &nav) {
    g.depthTesting(false);
    g.blending(true);
    g.blendAdd();

    g.pushMatrix();

    if (showLattice.get()) {
      drawLatticeEdges(g);
      drawLattice(g);
    }

    if (showSlice.get()) {
      drawSliceEdges(g);
      drawSlice(g);
    }

    slice->drawPickables(g);

    g.popMatrix();
  }

  void drawLattice(Graphics &g) {
    lattice->uploadVertices(latticeVertices, latticeColors);

    g.shader(instancing_shader);
    instancing_shader.uniform("scale", sphereSize.get());
    g.update();

    latticeSphere.vao().bind();
    latticeSphere.indexBuffer().bind();
    glDrawElementsInstanced(GL_TRIANGLES, latticeSphere.indices().size(),
                            GL_UNSIGNED_INT, 0, lattice->getVertexNum());
  }

  void drawLatticeEdges(Graphics &g) {
    lattice->uploadEdges(latticeEdgeStarts, latticeEdgeEnds);

    g.shader(edge_instancing_shader);
    edge_instancing_shader.uniform("color", edgeColor.get());
    g.update();

    latticeEdge.vao().bind();
    glDrawArraysInstanced(GL_LINES, 0, latticeEdge.vertices().size(),
                          lattice->getEdgeNum());
  }

  void drawSlice(Graphics &g) {
    slice->uploadVertices(sliceVertices, sliceColors);

    g.shader(instancing_shader);
    instancing_shader.uniform("scale", sphereSize.get());
    g.update();

    sliceSphere.vao().bind();
    sliceSphere.indexBuffer().bind();
    glDrawElementsInstanced(GL_TRIANGLES, sliceSphere.indices().size(),
                            GL_UNSIGNED_INT, 0, slice->getVertexNum());
  }

  void drawSliceEdges(Graphics &g) {
    slice->uploadEdges(sliceEdgeStarts, sliceEdgeEnds);

    g.shader(edge_instancing_shader);
    edge_instancing_shader.uniform("color", edgeColor.get());
    g.update();

    sliceEdge.vao().bind();
    glDrawArraysInstanced(GL_LINES, 0, sliceEdge.vertices().size(),
                          slice->getEdgeNum());
  }

  bool registerCallbacks(ParameterServer &parameterServer) {
    dataDir = File::conformPathToOS(File::currentPath());

    // remove bin directory from path
    auto posBin = dataDir.find("bin");
    dataDir = dataDir.substr(0, posBin);
    dataDir = File::conformPathToOS(dataDir + "data/");

    if (!al::File::exists(dataDir)) {
      if (!al::Dir::make(dataDir)) {
        std::cerr << "Unable to create directory: " << dataDir << std::endl;
        return false;
      }
    }

    dataDir.copy(filePath, sizeof filePath - 1);

    edgeColor.setHint("showAlpha", true);
    edgeColor.setHint("hsv", true);

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
      latticeSize.setNoCalls(latticeSize.getDefault());
      generate(value, sliceDim.get());
    });

    sliceDim.registerChangeCallback([&](int value) {
      millerNum.max(crystalDim.get() - value - 1);
      if (millerNum.get() > crystalDim.get() - value - 1) {
        millerNum.set(crystalDim.get() - value - 1);
      }
      latticeSize.setNoCalls(latticeSize.getDefault());
      generate(crystalDim.get(), value);
    });

    latticeSize.registerChangeCallback([&](int value) {
      lattice->generateLattice(value);
      slice->update();
    });

    basisNum.registerChangeCallback([&](int value) {
      basis1.setNoCalls(lattice->getBasis(value, 0));
      basis2.setNoCalls(lattice->getBasis(value, 1));
      basis3.setNoCalls(lattice->getBasis(value, 2));
      if (lattice->latticeDim > 3) {
        basis4.setNoCalls(lattice->getBasis(value, 3));
        if (lattice->latticeDim > 4) {
          basis5.setNoCalls(lattice->getBasis(value, 4));
        }
      }
    });

    basis1.registerChangeCallback(
        [&](float value) { setBasis(value, basisNum.get(), 0); });

    basis2.registerChangeCallback(
        [&](float value) { setBasis(value, basisNum.get(), 1); });

    basis3.registerChangeCallback(
        [&](float value) { setBasis(value, basisNum.get(), 2); });

    basis4.registerChangeCallback(
        [&](float value) { setBasis(value, basisNum.get(), 3); });

    basis5.registerChangeCallback(
        [&](float value) { setBasis(value, basisNum.get(), 4); });

    resetBasis.registerChangeCallback([&](bool value) {
      lattice->resetBasis();
      slice->update();

      basis1.setNoCalls(lattice->getBasis(basisNum.get(), 0));
      basis2.setNoCalls(lattice->getBasis(basisNum.get(), 1));
      basis3.setNoCalls(lattice->getBasis(basisNum.get(), 2));
      if (lattice->latticeDim > 3) {
        basis4.setNoCalls(lattice->getBasis(basisNum.get(), 3));
        if (lattice->latticeDim > 4) {
          basis5.setNoCalls(lattice->getBasis(basisNum.get(), 4));
        }
      }
    });

    sliceDepth.registerChangeCallback(
        [&](float value) { slice->setDepth(value); });

    edgeThreshold.registerChangeCallback(
        [&](float value) { slice->setThreshold(value); });

    millerNum.registerChangeCallback([&](int value) {
      miller1.setNoCalls(slice->getMiller(value, 0));
      miller2.setNoCalls(slice->getMiller(value, 1));
      miller3.setNoCalls(slice->getMiller(value, 2));
      miller4.setNoCalls(slice->getMiller(value, 3));
      miller5.setNoCalls(slice->getMiller(value, 4));
    });

    intMiller.registerChangeCallback([&](float value) {
      if (value) {
        miller1.setHint("input", -1);
        miller2.setHint("input", -1);
        miller3.setHint("input", -1);
        miller4.setHint("input", -1);
        miller5.setHint("input", -1);

        slice->roundMiller();

        miller1.setNoCalls(slice->getMiller(millerNum.get(), 0));
        miller2.setNoCalls(slice->getMiller(millerNum.get(), 1));
        miller3.setNoCalls(slice->getMiller(millerNum.get(), 2));
        miller4.setNoCalls(slice->getMiller(millerNum.get(), 3));
        miller5.setNoCalls(slice->getMiller(millerNum.get(), 4));
      } else {
        miller1.removeHint("input");
        miller2.removeHint("input");
        miller3.removeHint("input");
        miller4.removeHint("input");
        miller5.removeHint("input");
      }
    });

    miller1.registerChangeCallback(
        [&](float value) { slice->setMiller(value, millerNum.get(), 0); });

    miller2.registerChangeCallback(
        [&](float value) { slice->setMiller(value, millerNum.get(), 1); });

    miller3.registerChangeCallback(
        [&](float value) { slice->setMiller(value, millerNum.get(), 2); });

    miller4.registerChangeCallback(
        [&](float value) { slice->setMiller(value, millerNum.get(), 3); });

    miller5.registerChangeCallback(
        [&](float value) { slice->setMiller(value, millerNum.get(), 4); });

    resetUnitCell.registerChangeCallback(
        {[&](bool value) { slice->resetUnitCell(); }});

    exportTxt.registerChangeCallback([&](bool value) {
      std::string newPath = File::conformPathToOS(dataDir + fileName);
      slice->exportToTxt(newPath);
    });

    exportJson.registerChangeCallback([&](bool value) {
      std::string newPath = File::conformPathToOS(dataDir + fileName);
      slice->exportToJson(newPath);
    });

    savePreset.registerChangeCallback(
        [&](float value) { presets.storePreset(presetName); });

    loadPreset.registerChangeCallback(
        [&](float value) { presets.recallPreset(presetName); });

    parameterServer << crystalDim << sliceDim << latticeSize << basisNum
                    << basis1 << basis2 << basis3 << basis4 << basis5
                    << resetBasis << showLattice << showSlice << sphereSize
                    << edgeColor << sliceDepth << edgeThreshold << millerNum
                    << intMiller << miller1 << miller2 << miller3 << miller4
                    << miller5 << resetUnitCell;

    // TODO: add basis and miller to preset
    presets << crystalDim << sliceDim << latticeSize << showLattice << showSlice
            << sphereSize << edgeColor << sliceDepth << edgeThreshold
            << intMiller;

    return true;
  }

  void setGUIFrame(NavInputControl &navControl) {
    ImGui::Begin("Crystal");

    ParameterGUI::draw(&crystalDim);
    ParameterGUI::draw(&sliceDim);
    ParameterGUI::draw(&latticeSize);

    if (ImGui::CollapsingHeader("Edit Basis Vectort",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ImGui::Indent();
      ParameterGUI::draw(&basisVec0);
      ParameterGUI::draw(&basisVec1);
      ParameterGUI::draw(&basisVec2);
      ParameterGUI::draw(&basisVec3);
      ParameterGUI::draw(&basisVec4);
      ImGui::Unindent();
    }
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

    ParameterGUI::draw(&showLattice);
    ImGui::SameLine();
    ParameterGUI::draw(&showSlice);

    if (ImGui::CollapsingHeader("Edit Display Settings",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ParameterGUI::draw(&sphereSize);
      ParameterGUI::draw(&edgeColor);
    }

    ImGui::NewLine();

    if (showSlice.get()) {
      ParameterGUI::draw(&sliceDepth);
      ParameterGUI::draw(&edgeThreshold);

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

    ImGui::Text("Hyperplane Basis 0");
    ImGui::Indent();
    ImGui::Text("Hyperplane Basis 1");
    ImGui::Text("Hyperplane Basis 2");
    ImGui::Text("Hyperplane Basis 3");
    ImGui::Text("Hyperplane Basis 4");
    ImGui::Unindent();

    ParameterGUI::draw(&sliceBasis0);

    ParameterGUI::draw(&resetUnitCell);

    ImGui::NewLine();

    ImGui::InputText("filePath", filePath, IM_ARRAYSIZE(filePath));
    if (ImGui::IsItemActive()) {
      navControl.active(false);
    } else {
      navControl.active(true);
    }
    ImGui::InputText("fileName", fileName, IM_ARRAYSIZE(fileName));
    if (ImGui::IsItemActive()) {
      navControl.active(false);
    } else {
      navControl.active(true);
    }

    ParameterGUI::draw(&exportTxt);
    ImGui::SameLine();
    ParameterGUI::draw(&exportJson);

    // ImGui::InputText("preset", presetName, IM_ARRAYSIZE(presetName));
    // if (ImGui::IsItemActive()) {
    //   navControl.active(false);
    // } else {
    //   navControl.active(true);
    // }

    // TODO: fix this
    // ParameterGUI::draw(&savePreset);
    // ImGui::SameLine();
    // ParameterGUI::draw(&loadPreset);

    ImGui::End();
  }

  void watchFile(std::string path) {
    File file(searchPaths.find(path).filepath());
    watchedFiles[path] = WatchedFile{file, file.modified()};
  }

  bool watchCheck() {
    bool changed = false;
    if (floor(al_system_time()) > watchCheckTime) {
      watchCheckTime = floor(al_system_time());
      for (std::map<std::string, WatchedFile>::iterator i =
               watchedFiles.begin();
           i != watchedFiles.end(); i++) {
        WatchedFile &watchedFile = (*i).second;
        if (watchedFile.modified != watchedFile.file.modified()) {
          watchedFile.modified = watchedFile.file.modified();
          changed = true;
        }
      }
    }
    return changed;
  }

  // read in a .glsl file
  // add it to the watch list of files
  // replace shader #include directives with corresponding file code
  std::string loadGlsl(std::string filename) {
    watchFile(filename);
    std::string code = File::read(searchPaths.find(filename).filepath());
    size_t from = code.find("#include \"");
    if (from != std::string::npos) {
      size_t capture = from + strlen("#include \"");
      size_t to = code.find("\"", capture);
      std::string include_filename = code.substr(capture, to - capture);
      std::string replacement =
          File::read(searchPaths.find(include_filename).filepath());
      code = code.replace(from, to - from + 2, replacement);
      // printf("code: %s\n", code.data());
    }
    return code;
  }

  void loadShader(ShaderProgram &program, std::string vp_filename,
                  std::string fp_filename) {
    std::string vp = loadGlsl(vp_filename);
    std::string fp = loadGlsl(fp_filename);
    program.compile(vp, fp);
  }

  void loadShader(ShaderProgram &program, std::string vp_filename,
                  std::string fp_filename, std::string gp_filename) {
    std::string vp = loadGlsl(vp_filename);
    std::string fp = loadGlsl(fp_filename);
    std::string gp = loadGlsl(gp_filename);
    program.compile(vp, fp, gp);
  }

  std::shared_ptr<AbstractLattice> lattice;
  std::shared_ptr<AbstractSlice> slice;

private:
  SearchPaths searchPaths;
  struct WatchedFile {
    File file;
    al_sec modified;
  };
  std::map<std::string, WatchedFile> watchedFiles;
  al_sec watchCheckTime;

  ShaderProgram instancing_shader, edge_instancing_shader;

  VAOMesh latticeSphere, latticeEdge, sliceSphere, sliceEdge;
  BufferObject latticeVertices, latticeColors, latticeEdgeStarts,
      latticeEdgeEnds;
  BufferObject sliceVertices, sliceColors, sliceEdgeStarts, sliceEdgeEnds;

  PresetHandler presets{"presets", true};

  ParameterInt crystalDim{"crystalDim", "", 3, 3, 5};
  ParameterInt sliceDim{"sliceDim", "", 2, 2, 2};
  ParameterInt latticeSize{"latticeSize", "", 1, 1, 15};

  ParameterVec5 basisVec0{"basisVec0", ""};
  ParameterVec5 basisVec1{"basisVec1", ""};
  ParameterVec5 basisVec2{"basisVec2", ""};
  ParameterVec5 basisVec3{"basisVec3", ""};
  ParameterVec5 basisVec4{"basisVec4", ""};

  ParameterInt basisNum{"basisNum", "", 0, 0, 2};
  Parameter basis1{"basis1", "", 1, -5, 5};
  Parameter basis2{"basis2", "", 0, -5, 5};
  Parameter basis3{"basis3", "", 0, -5, 5};
  Parameter basis4{"basis4", "", 0, -5, 5};
  Parameter basis5{"basis5", "", 0, -5, 5};
  Trigger resetBasis{"resetBasis", ""};

  ParameterBool showLattice{"showLattice", "", 0};
  ParameterBool showSlice{"showSlice", "", 1};

  Parameter sphereSize{"sphereSize", "", 0.04, 0.001, 1};
  ParameterColor edgeColor{"edgeColor", "", Color(1.f, 0.3f)};

  Parameter sliceDepth{"sliceDepth", "", 0, 0, 1000.f};
  Parameter edgeThreshold{"edgeThreshold", "", 1.1f, 0.f, 2.f};

  ParameterInt millerNum{"millerNum", "", 0, 0, 0};
  ParameterBool intMiller{"intMiller", ""};
  Parameter miller1{"miller1", "", 1, -5, 5};
  Parameter miller2{"miller2", "", 0, -5, 5};
  Parameter miller3{"miller3", "", 0, -5, 5};
  Parameter miller4{"miller4", "", 0, -5, 5};
  Parameter miller5{"miller5", "", 0, -5, 5};

  ParameterVec5 sliceBasis0{"sliceBasis0", ""};

  Trigger resetUnitCell{"resetUnitCell", ""};

  std::string dataDir;
  char filePath[128]{};
  char fileName[128]{};
  Trigger exportTxt{"exportTxt", ""};
  Trigger exportJson{"exportJson", ""};

  char presetName[128]{};
  Trigger savePreset{"savePreset", ""};
  Trigger loadPreset{"loadPreset", ""};
};

#endif // CRYSTAL_VIEWER_HPP