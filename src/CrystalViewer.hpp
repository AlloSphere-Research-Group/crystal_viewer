#ifndef CRYSTAL_VIEWER_HPP
#define CRYSTAL_VIEWER_HPP
/*
 * Copyright 2023 AlloSphere Research Group
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Kon Hyong Kim
 */

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

const std::string instancing_vert = R"(
#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
uniform float scale;
uniform float useColorCode;
uniform vec4 color;
uniform vec3 boxMin;
uniform vec3 boxMax;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 offset;
layout (location = 2) in vec4 colors;

out vec4 colorCode;

float insideBox(vec3 v) {
    vec3 s = step(boxMin, v) - step(boxMax, v);
    return s.x * s.y * s.z; 
}

void main()
{
  vec4 p = vec4(scale * position + offset, 1.0);
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * p;
  
  //colorCode = color;
  colorCode = useColorCode * colors + (1 - useColorCode) * color;
  float ib = insideBox(offset);
  colorCode.a = 0.05 + 0.95 * ib;
  // we also have access to index of instance,
  // for example, when drawing 100 instances,
  // `gl_InstanceID` goes 0 to 99
  // t = float(gl_InstanceID) / (100000.0 - 1.0);
}
)";

const std::string instancing_frag = R"(
#version 330

in vec4 colorCode;

layout (location = 0) out vec4 frag_out0;

void main()
{
  frag_out0 = colorCode;
}
)";

const std::string edge_instancing_vert = R"(
#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
uniform vec3 boxMin;
uniform vec3 boxMax;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 offset;
layout (location = 2) in vec3 end_offset;

out vec4 edge_coords;
out float inBox;

float insideBox(vec3 v) {
    vec3 s = step(boxMin, v) - step(boxMax, v);
    return s.x * s.y * s.z; 
}

void main()
{
  vec4 p = vec4(position + offset, 1.0);
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * p;

  vec4 p_end = vec4(position + end_offset, 1.0);  

  edge_coords = al_ProjectionMatrix * al_ModelViewMatrix * p_end;
  inBox = insideBox(offset) * insideBox(end_offset);
}
)";

const std::string edge_instancing_geo = R"(
#version 330

in vec4 edge_coords[];
in float inBox[];

out float isInBox;

layout(lines) in;
layout(line_strip, max_vertices=2) out;

void main()
{
  isInBox = inBox[0];

  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  gl_Position = edge_coords[0];
  EmitVertex();
  
  EndPrimitive();
}
)";

const std::string edge_instancing_frag = R"(
#version 330

uniform vec4 color;

in float isInBox;

layout (location = 0) out vec4 frag_out0;

void main()
{
  frag_out0 = vec4(color.rgb, 0.05 + color.a * 0.95 * isInBox);
}
)";

class CrystalViewer {
public:
  bool init(ParameterServer &parameterServer) {
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

    instancing_shader.compile(instancing_vert, instancing_frag);

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

    edge_instancing_shader.compile(edge_instancing_vert, edge_instancing_frag,
                                   edge_instancing_geo);

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

    return registerCallbacks(parameterServer);
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

  void setGUIFrame(NavInputControl &navControl) {
    ImGui::Begin("Crystal");

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

    ParameterGUI::draw(&showLattice);
    if (showLattice.get()) {
      ImGui::SameLine();
      ParameterGUI::draw(&enableLatticeEdge);
    }

    ParameterGUI::draw(&showSlice);
    ImGui::SameLine();
    ParameterGUI::draw(&enableSliceEdge);

    ParameterGUI::draw(&showSlicePlane);

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
      ParameterGUI::draw(&recomputeEdges);
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

    ParameterGUI::draw(&boxMin);
    ParameterGUI::draw(&boxMax);

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

    ImGui::End();
  }

  void draw(Graphics &g, Nav &nav) {
    g.depthTesting(false);
    g.blending(true);
    g.blendAdd();

    g.pushMatrix();

    // if (showLattice.get()) {
    //   if (enableLatticeEdge.get()) {
    //     drawLatticeEdges(g);
    //   }
    //   drawLattice(g);
    // }

    // if (showSlicePlane.get()) {
    //   drawBox(g);
    // }

    if (showSlice.get()) {
      if (enableSliceEdge.get()) {
        // drawSliceEdges(g);
      }
      drawSlice(g);
    }

    slice->drawPickable(g);

    g.popMatrix();
  }

  void drawLattice(Graphics &g) {
    lattice->updateBuffer(latticeVertices, latticeColors);

    g.shader(instancing_shader);
    g.shader().uniform("scale", latticeSphereSize.get());
    g.shader().uniform("useColorCode", 0.f);
    g.shader().uniform("color", latticeSphereColor.get());
    g.shader().uniform("boxMin", boxMin.get());
    g.shader().uniform("boxMax", boxMax.get());
    g.update();

    latticeSphere.vao().bind();
    latticeSphere.indexBuffer().bind();
    glDrawElementsInstanced(GL_TRIANGLES, latticeSphere.indices().size(),
                            GL_UNSIGNED_INT, 0, lattice->getVertexNum());
  }

  void drawLatticeEdges(Graphics &g) {
    lattice->updateEdgeBuffers(latticeEdgeStarts, latticeEdgeEnds);

    g.shader(edge_instancing_shader);
    g.shader().uniform("color", latticeEdgeColor.get());
    g.shader().uniform("boxMin", boxMin.get());
    g.shader().uniform("boxMax", boxMax.get());
    g.update();

    latticeEdge.vao().bind();
    glDrawArraysInstanced(GL_LINES, 0, latticeEdge.vertices().size(),
                          lattice->getEdgeNum());
  }

  void drawSlice(Graphics &g) {
    // g.color(slicePlaneColor.get());
    // slice->drawPlane(g);

    slice->updateVertexBuffer(sliceVertices, sliceColors);

    g.shader(instancing_shader);
    g.shader().uniform("scale", sliceSphereSize.get());
    g.shader().uniform("useColorCode", recomputeEdges.get());
    g.shader().uniform("color", sliceSphereColor.get());
    g.shader().uniform("boxMin", boxMin.get());
    g.shader().uniform("boxMax", boxMax.get());
    g.update();

    sliceSphere.vao().bind();
    sliceSphere.indexBuffer().bind();
    glDrawElementsInstanced(GL_TRIANGLES, sliceSphere.indices().size(),
                            GL_UNSIGNED_INT, 0, slice->getVertexNum());
  }

  void drawSliceEdges(Graphics &g) {
    slice->updateEdgeBuffers(sliceEdgeStarts, sliceEdgeEnds);

    g.shader(edge_instancing_shader);
    g.shader().uniform("color", sliceEdgeColor.get());
    g.shader().uniform("boxMin", boxMin.get());
    g.shader().uniform("boxMax", boxMax.get());
    g.update();

    sliceEdge.vao().bind();
    glDrawArraysInstanced(GL_LINES, 0, sliceEdge.vertices().size(),
                          slice->getEdgeNum());
  }

  void drawBox(Graphics &g) {
    // TODO: edit box colors
    g.color(0.3f, 0.3f, 1.0f, 0.1f);
    slice->drawBox(g);

    g.color(1.f, 1.f, 0.f, 0.2f);
    slice->drawBoxNodes(g, sliceSphere);
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

      generate(value, sliceDim.get());

      latticeSize.setNoCalls(latticeSize.getDefault());
    });

    latticeSize.registerChangeCallback([&](int value) {
      lattice->createLattice(value);
      slice->update();
    });

    basisNum.registerChangeCallback([&](int value) {
      basis1.setNoCalls(lattice->getBasis(value, 0));
      basis2.setNoCalls(lattice->getBasis(value, 1));
      basis3.setNoCalls(lattice->getBasis(value, 2));
      if (lattice->dim > 3) {
        basis4.setNoCalls(lattice->getBasis(value, 3));
        if (lattice->dim > 4) {
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
      if (lattice->dim > 3) {
        basis4.setNoCalls(lattice->getBasis(basisNum.get(), 3));
        if (lattice->dim > 4) {
          basis5.setNoCalls(lattice->getBasis(basisNum.get(), 4));
        }
      }
    });

    enableLatticeEdge.registerChangeCallback([&](bool value) {
      lattice->enableEdges = value;
      if (value) {
        lattice->updateEdges();
      }
    });

    enableSliceEdge.registerChangeCallback([&](bool value) {
      slice->enableEdges = value;
      if (value) {
        slice->updateEdges();
      }
    });

    slicePlaneSize.registerChangeCallback(
        [&](float value) { slice->setWindow(value, sliceDepth.get()); });

    sliceDim.registerChangeCallback([&](int value) {
      millerNum.max(crystalDim.get() - value - 1);
      if (millerNum.get() > crystalDim.get() - value - 1) {
        millerNum.set(crystalDim.get() - value - 1);
      }

      generate(crystalDim.get(), value);
    });

    sliceDepth.registerChangeCallback([&](float value) {
      slice->setWindow(slicePlaneSize.get(), value);
      slice->update();
    });

    recomputeEdges.registerChangeCallback([&](bool value) {
      slice->recomputeEdges = value;
      slice->updateEdges();
    });

    edgeThreshold.registerChangeCallback([&](float value) {
      slice->edgeThreshold = value;
      slice->update();
    });

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

    exportTxt.registerChangeCallback([&](bool value) {
      std::string newPath = File::conformPathToOS(dataDir + fileName);
      slice->exportToTxt(newPath);
    });

    exportJson.registerChangeCallback([&](bool value) {
      std::string newPath = File::conformPathToOS(dataDir + fileName);
      slice->exportToJson(newPath);
    });

    parameterServer << crystalDim << latticeSize << basisNum << basis1 << basis2
                    << basis3 << basis4 << basis5 << resetBasis << showLattice
                    << enableLatticeEdge << showSlice << enableSliceEdge
                    << showSlicePlane << latticeSphereSize << latticeSphereColor
                    << latticeEdgeColor << sliceSphereSize << sliceSphereColor
                    << sliceEdgeColor << slicePlaneSize << slicePlaneColor
                    << sliceDim << sliceDepth << recomputeEdges << edgeThreshold
                    << millerNum << intMiller << miller1 << miller2 << miller3
                    << miller4 << miller5 << boxMin << boxMax;

    return true;
  }

  AbstractSlice *getSlice() { return slice.get(); }

private:
  std::shared_ptr<AbstractLattice> lattice;
  std::shared_ptr<AbstractSlice> slice;

  VAOMesh latticeSphere, latticeEdge, sliceSphere, sliceEdge;
  BufferObject latticeVertices, latticeColors, latticeEdgeStarts,
      latticeEdgeEnds;
  BufferObject sliceVertices, sliceColors, sliceEdgeStarts, sliceEdgeEnds;
  ShaderProgram instancing_shader, edge_instancing_shader;

  ParameterInt crystalDim{"crystalDim", "", 3, 3, 5};
  ParameterInt latticeSize{"latticeSize", "", 1, 1, 6};

  ParameterInt basisNum{"basisNum", "", 0, 0, 2};
  Parameter basis1{"basis1", "", 1, -5, 5};
  Parameter basis2{"basis2", "", 0, -5, 5};
  Parameter basis3{"basis3", "", 0, -5, 5};
  Parameter basis4{"basis4", "", 0, -5, 5};
  Parameter basis5{"basis5", "", 0, -5, 5};
  Trigger resetBasis{"resetBasis", ""};

  ParameterBool showLattice{"showLattice", "", 0};
  ParameterBool enableLatticeEdge{"enableLatticeEdge", "", 0};
  ParameterBool showSlice{"showSlice", "", 1};
  ParameterBool enableSliceEdge{"enableSliceEdge", "", 0};
  ParameterBool showSlicePlane{"showSlicePlane", "", 0};

  Parameter latticeSphereSize{"latticeSphereSize", "", 0.02, 0.001, 1};
  ParameterColor latticeSphereColor{"latticeSphereColor", "", Color(1.f, 0.8f)};
  ParameterColor latticeEdgeColor{"latticeEdgeColor", "", Color(1.f, 0.5f)};

  Parameter sliceSphereSize{"sliceSphereSize", 0.04, 0.001, 1};
  ParameterColor sliceSphereColor{"sliceSphereColor", "",
                                  Color(1.f, 0.f, 0.f, 1.f)};
  ParameterColor sliceEdgeColor{"sliceEdgeColor", "", Color(1.f, 0.5f)};

  Parameter slicePlaneSize{"slicePlaneSize", "", 0.f, 0, 30.f};
  ParameterColor slicePlaneColor{"slicePlaneColor", "",
                                 Color(0.3f, 0.3f, 1.f, 0.3f)};

  ParameterInt sliceDim{"sliceDim", "", 2, 2, 2};
  Parameter sliceDepth{"sliceDepth", "", 0, 0, 10.f};

  ParameterBool recomputeEdges{"recomputeEdges", "", 1.f};
  Parameter edgeThreshold{"edgeThreshold", "", 1.1f, 0.f, 2.f};

  ParameterInt millerNum{"millerNum", "", 0, 0, 0};
  ParameterBool intMiller{"intMiller", ""};
  Parameter miller1{"miller1", "", 1, -5, 5};
  Parameter miller2{"miller2", "", 0, -5, 5};
  Parameter miller3{"miller3", "", 0, -5, 5};
  Parameter miller4{"miller4", "", 0, -5, 5};
  Parameter miller5{"miller5", "", 0, -5, 5};

  ParameterVec3 boxMin{"boxMin", "", Vec3f{0, 0, 0}};
  ParameterVec3 boxMax{"boxMax", "", Vec3f{1, 1, 1}};

  std::string dataDir;
  char filePath[128]{};
  char fileName[128]{};
  Trigger exportTxt{"ExportTxt", ""};
  Trigger exportJson{"ExportJson", ""};
};

#endif // CRYSTAL_VIEWER_HPP