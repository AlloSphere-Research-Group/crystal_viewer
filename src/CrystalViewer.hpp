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
    createCrystal(crystalDim.get(), sliceDim.get());

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

  void createCrystal(int newDim, int newSliceDim) {
    switch (newDim) {
    case 3: {
      auto newLattice = std::make_shared<Lattice<3>>(lattice);
      auto newSlice = std::make_shared<Slice<3, 2>>(slice, newLattice);

      lattice = newLattice;
      lattice->latticeSize = latticeSize.get();
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
      lattice->latticeSize = latticeSize.get();
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
      lattice->latticeSize = latticeSize.get();
      break;
    }
    default:
      std::cerr << "Dimension " << newDim << " not supported." << std::endl;
      return;
    }
  }

  void draw(Graphics &g, Nav &nav) {

    if(needsCreate){
      createCrystal(crystalDim.get(), sliceDim.get());
      needsCreate = false;
    }
    
    lattice->pollUpdate(); // update if needs update
    slice->pollUpdate(); // update if needs update

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

  void setBasis(Vec5f &value, int basisNum) {
    lattice->setBasis(value, basisNum);
    // slice->update();
    slice->needsUpdate = true;
  }

  void setDimensionHints(float value) {
    basis0.setHint("dimension", value);
    basis1.setHint("dimension", value);
    basis2.setHint("dimension", value);
    basis3.setHint("dimension", value);
    basis4.setHint("dimension", value);
    miller0.setHint("dimension", value);
    miller1.setHint("dimension", value);
    miller2.setHint("dimension", value);
    hyperplane0.setHint("dimension", value);
    hyperplane1.setHint("dimension", value);
    hyperplane2.setHint("dimension", value);
    sliceBasis0.setHint("dimension", value);
    sliceBasis1.setHint("dimension", value);
    sliceBasis2.setHint("dimension", value);
    sliceBasis3.setHint("dimension", value);
  }

  void setHideHints(int crystal_dim, int slice_dim) {
    if (crystal_dim > 3) {
      basis3.removeHint("hide");
      if (crystal_dim > 4) {
        basis4.removeHint("hide");
      } else {
        basis4.setHint("hide", 1.f);
      }
    } else {
      basis3.setHint("hide", 1.f);
      basis4.setHint("hide", 1.f);
    }

    if (slice_dim > 2) {
      sliceBasis2.removeHint("hide");
      if (slice_dim > 3) {
        sliceBasis3.removeHint("hide");
      } else {
        sliceBasis3.setHint("hide", 1.f);
      }
    } else {
      sliceBasis2.setHint("hide", 1.f);
      sliceBasis3.setHint("hide", 1.f);
    }

    if (crystal_dim - slice_dim > 1) {
      miller1.removeHint("hide");
      hyperplane1.removeHint("hide");
      if (crystal_dim - slice_dim > 2) {
        miller2.removeHint("hide");
        hyperplane2.removeHint("hide");
      } else {
        miller2.setHint("hide", 1.f);
        hyperplane2.setHint("hide", 1.f);
      }
    } else {
      miller1.setHint("hide", 1.f);
      miller2.setHint("hide", 1.f);
      hyperplane1.setHint("hide", 1.f);
      hyperplane2.setHint("hide", 1.f);
    }
  }

  bool registerCallbacks(ParameterServer &parameterServer){
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

    setDimensionHints(crystalDim.getDefault());
    setHideHints(crystalDim.getDefault(), sliceDim.getDefault());

    crystalDim.registerChangeCallback([&](int value) {
      sliceDim.max(value - 1);
      if (sliceDim.get() > value - 1) {
        sliceDim.set(value - 1);
      }

      setDimensionHints((float)value);
      setHideHints(value, sliceDim.get());

      // latticeSize.setNoCalls(latticeSize.getDefault());
      
      needsCreate = true;
    });

    sliceDim.registerChangeCallback([&](int value) {
      setHideHints(crystalDim.get(), value);

      // latticeSize.setNoCalls(latticeSize.getDefault());
      
      needsCreate = true;
    });

    latticeSize.registerChangeCallback([&](int value) {
      // lattice->generateLattice(value);
      lattice->latticeSize = value;
      lattice->needsUpdate = true;
      slice->needsUpdate = true;
    });

    basis0.registerChangeCallback([&](Vec5f value) { setBasis(value, 0); });

    basis1.registerChangeCallback([&](Vec5f value) { setBasis(value, 1); });

    basis2.registerChangeCallback([&](Vec5f value) { setBasis(value, 2); });

    basis3.registerChangeCallback([&](Vec5f value) { setBasis(value, 3); });

    basis4.registerChangeCallback([&](Vec5f value) { setBasis(value, 4); });

    resetBasis.registerChangeCallback([&](bool value) {
      basis0.setNoCalls(basis0.getDefault());
      basis1.setNoCalls(basis1.getDefault());
      basis2.setNoCalls(basis2.getDefault());
      basis3.setNoCalls(basis3.getDefault());
      basis4.setNoCalls(basis4.getDefault());
      lattice->resetBasis();

      // slice->update();
      slice->needsUpdate = true;
    });

    sliceDepth.registerChangeCallback(
        [&](float value) { slice->setDepth(value); });

    edgeThreshold.registerChangeCallback(
        [&](float value) { slice->setThreshold(value); });

    intMiller.registerChangeCallback([&](float value) {
      if (value) {
        miller0.setHint("format", 0);
        miller1.setHint("format", 0);
        miller2.setHint("format", 0);

        slice->roundMiller();

        miller0.setNoCalls(slice->getMiller(0));
        miller1.setNoCalls(slice->getMiller(1));
        miller2.setNoCalls(slice->getMiller(2));
      } else {
        miller0.removeHint("format");
        miller1.removeHint("format");
        miller2.removeHint("format");
      }
    });

    miller0.registerChangeCallback(
        [&](Vec5f value) { slice->setMiller(value, 0); });

    miller1.registerChangeCallback(
        [&](Vec5f value) { slice->setMiller(value, 1); });

    miller2.registerChangeCallback(
        [&](Vec5f value) { slice->setMiller(value, 2); });

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

    parameterServer << crystalDim << sliceDim << latticeSize
                    << basis0 << basis1 << basis2 << basis3 << basis4
                    << resetBasis << showLattice << showSlice << sphereSize
                    << edgeColor << sliceDepth << edgeThreshold
                    << intMiller 
                    << miller0 << miller1 << miller2
                    << hyperplane0 << hyperplane1 << hyperplane2
                    << sliceBasis0 << sliceBasis1 << sliceBasis2 << sliceBasis3
                    << resetUnitCell;

    presets << crystalDim << sliceDim << latticeSize << showLattice << showSlice
            << sphereSize << edgeColor << sliceDepth << edgeThreshold
            << intMiller
            << miller0 << miller1 << miller2
            << hyperplane0 << hyperplane1 << hyperplane2
            << sliceBasis0 << sliceBasis1 << sliceBasis2 << sliceBasis3;

    return true;
  }

  static bool PresetMapToTextList(void* data, int n, const char** out_text){
    const std::map<int, std::string>* m = (std::map<int,std::string>*)data;
    if(m->find(n) == m->end()) *out_text = "";
    else *out_text = m->at(n).c_str();
    return true;
  }

  void setGUIFrame(NavInputControl &navControl) {
    ImGui::Begin("Crystal");

    ParameterGUI::draw(&crystalDim);
    ParameterGUI::draw(&sliceDim);
    ParameterGUI::draw(&latticeSize);

    if (ImGui::CollapsingHeader("Edit Basis Vector",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      // ImGui::PushStyleColor(ImGuiCol_FrameBg,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.5f, 0.3f));
      // ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.6f, 0.3f));
      // ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.7f, 0.3f));
      // ImGui::PushStyleColor(ImGuiCol_SliderGrab,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.9f, 0.5f));
      // ParameterGUI::draw(&basisNum);
      // ImGui::PopStyleColor(4);
      ImGui::Indent();
      ParameterGUI::draw(&basis0);
      ParameterGUI::draw(&basis1);
      ParameterGUI::draw(&basis2);
      ParameterGUI::draw(&basis3);
      ParameterGUI::draw(&basis4);
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
    }

    if (ImGui::CollapsingHeader("Edit Miller Indices",
                                ImGuiTreeNodeFlags_CollapsingHeader |
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
      // ImGui::PushStyleColor(ImGuiCol_FrameBg,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.5f, 0.3f));
      // ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.6f, 0.3f));
      // ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.7f, 0.3f));
      // ImGui::PushStyleColor(ImGuiCol_SliderGrab,
      //                       (ImVec4)ImColor::HSV(0.15f, 0.9f, 0.5f));
      // ParameterGUI::draw(&millerNum);
      // ImGui::PopStyleColor(4);

      ParameterGUI::draw(&intMiller);
      ImGui::Indent();
      ParameterGUI::draw(&miller0);
      ParameterGUI::draw(&miller1);
      ParameterGUI::draw(&miller2);
      ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Edit Hyperplane Normals",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ImGui::Indent();
      ParameterGUI::draw(&hyperplane0);
      ParameterGUI::draw(&hyperplane1);
      ParameterGUI::draw(&hyperplane2);
      ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Edit Slice Basis",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ImGui::Indent();
      ParameterGUI::draw(&sliceBasis0);
      ParameterGUI::draw(&sliceBasis1);
      ParameterGUI::draw(&sliceBasis2);
      ParameterGUI::draw(&sliceBasis3);
      ImGui::Unindent();
    }

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

    if (ImGui::CollapsingHeader("Presets",
                                ImGuiTreeNodeFlags_CollapsingHeader)) {
      ImGui::Indent();

      std::map<int, std::string> savedPresets = presets.availablePresets();
      static int itemCurrent = 1;
      int lastItem = itemCurrent;
      ImGui::ListBox("presets", &itemCurrent, PresetMapToTextList, (void*)&savedPresets, savedPresets.size());
      
      if(lastItem != itemCurrent && savedPresets.find(itemCurrent) != savedPresets.end())
        strcpy(presetName, savedPresets.at(itemCurrent).c_str());

      ImGui::InputText("preset name", presetName, IM_ARRAYSIZE(presetName));
      if (ImGui::IsItemActive()) {
        navControl.active(false);
      } else {
        navControl.active(true);
      }
      ParameterGUI::draw(&savePreset);
      ImGui::SameLine();
      ParameterGUI::draw(&loadPreset);
      ImGui::Unindent();
    }

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

  PresetHandler presets{"data/presets", true};

  bool needsCreate{false};

  ParameterInt crystalDim{"crystalDim", "", 3, 3, 5};
  ParameterInt sliceDim{"sliceDim", "", 2, 2, 2};
  ParameterInt latticeSize{"latticeSize", "", 1, 1, 15};

  ParameterVec5 basis0{"basis0", ""};
  ParameterVec5 basis1{"basis1", ""};
  ParameterVec5 basis2{"basis2", ""};
  ParameterVec5 basis3{"basis3", ""};
  ParameterVec5 basis4{"basis4", ""};
  Trigger resetBasis{"resetBasis", ""};

  ParameterBool showLattice{"showLattice", "", 0};
  ParameterBool showSlice{"showSlice", "", 1};

  Parameter sphereSize{"sphereSize", "", 0.04, 0.001, 1};
  ParameterColor edgeColor{"edgeColor", "", Color(1.f, 0.3f)};

  Parameter sliceDepth{"sliceDepth", "", 1.0f, 0, 1000.f};
  Parameter edgeThreshold{"edgeThreshold", "", 1.1f, 0.f, 2.f};

  ParameterBool intMiller{"intMiller", ""};
  ParameterVec5 miller0{"miller0", ""};
  ParameterVec5 miller1{"miller1", ""};
  ParameterVec5 miller2{"miller2", ""};

  ParameterVec5 hyperplane0{"hyperplane0", ""};
  ParameterVec5 hyperplane1{"hyperplane1", ""};
  ParameterVec5 hyperplane2{"hyperplane2", ""};

  ParameterVec5 sliceBasis0{"sliceBasis0", ""};
  ParameterVec5 sliceBasis1{"sliceBasis1", ""};
  ParameterVec5 sliceBasis2{"sliceBasis2", ""};
  ParameterVec5 sliceBasis3{"sliceBasis3", ""};

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