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

layout (location = 0) in vec3 position;
// attibute at location 1 will be set to have
// "attribute divisor = 1" which means for given buffer for attibute 1,
// index for reading that buffer will increase per-instance, not per-vertex
// divisor = 0 is default value and means per-vertex increase
layout (location = 1) in vec3 offset;

// out float t;

void main()
{
  // to multiply position vector with matrices,
  // 4th component must be 1 (homogeneous coord)
  vec4 p = vec4(scale * position + offset, 1.0);
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * p;
  // we also have access to index of instance,
  // for example, when drawing 100 instances,
  // `gl_InstanceID` goes 0 to 99
  // t = float(gl_InstanceID) / (100000.0 - 1.0);
}
)";

const std::string instancing_frag = R"(
#version 330

uniform vec4 color;
layout (location = 0) out vec4 frag_out0;
void main()
{
  frag_out0 = color;
}
)";

const std::string edge_instancing_vert = R"(
#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 offset;
layout (location = 2) in vec3 end_offset;

out vec4 edge_coords;

void main()
{
  vec4 p = vec4(position + offset, 1.0);
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * p;

  vec4 p_end = vec4(position + end_offset, 1.0);  

  edge_coords = al_ProjectionMatrix * al_ModelViewMatrix * p_end;
}
)";

const std::string edge_instancing_geo = R"(
#version 330

in vec4 edge_coords[];

layout(lines) in;
layout(line_strip, max_vertices=2) out;

void main()
{
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  gl_Position = edge_coords[0];
  EmitVertex();

  EndPrimitive();
}
)";

class CrystalViewer {
public:
  bool init() {
    generate(crystalDim.get(), sliceDim.get());

    addSphere(latticeSphere, 1.0);
    latticeSphere.update();

    latticeOffsets.bufferType(GL_ARRAY_BUFFER);
    latticeOffsets.usage(GL_DYNAMIC_DRAW);
    latticeOffsets.create();

    addSphere(sliceSphere, 1.0);
    sliceSphere.update();

    sliceOffsets.bufferType(GL_ARRAY_BUFFER);
    sliceOffsets.usage(GL_DYNAMIC_DRAW);
    sliceOffsets.create();

    instancing_shader.compile(instancing_vert, instancing_frag);

    auto &latticeVAO = latticeSphere.vao();
    latticeVAO.bind();
    latticeVAO.enableAttrib(1);
    latticeVAO.attribPointer(1, latticeOffsets, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    auto &sliceVAO = sliceSphere.vao();
    sliceVAO.bind();
    sliceVAO.enableAttrib(1);
    sliceVAO.attribPointer(1, sliceOffsets, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    edgeLine.vertex(Vec3f(0));
    edgeLine.vertex(Vec3f(1, 1, 1));
    edgeLine.update();

    edgeOffsets.bufferType(GL_ARRAY_BUFFER);
    edgeOffsets.usage(GL_DYNAMIC_DRAW);
    edgeOffsets.create();

    edgeEndOffsets.bufferType(GL_ARRAY_BUFFER);
    edgeEndOffsets.usage(GL_DYNAMIC_DRAW);
    edgeEndOffsets.create();

    edge_instancing_shader.compile(edge_instancing_vert, instancing_frag,
                                   edge_instancing_geo);

    auto &edgeVAO = edgeLine.vao();
    edgeVAO.bind();
    edgeVAO.enableAttrib(1);
    edgeVAO.attribPointer(1, edgeOffsets, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    edgeVAO.enableAttrib(2);
    edgeVAO.attribPointer(2, edgeEndOffsets, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(2, 1);

    return registerCallbacks();
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

  Vec3f getNormal() { return slice->getNormal(); }

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
      ParameterGUI::draw(&enableLatticeEdge);
    }

    ParameterGUI::draw(&showSlice);
    ImGui::SameLine();
    ParameterGUI::draw(&enableSliceEdge);

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
      ParameterGUI::draw(&recreateEdges);
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

    if (lockCameraToSlice.get()) {
      Quatf rot = Quatf::getRotationTo(getNormal(), nav.uf());
      g.rotate(rot);
    }

    if (showLattice.get()) {
      if (enableLatticeEdge.get()) {
        drawLatticeEdges(g);
      }
      drawLattice(g);

      drawLatticeParticles(g);
    }

    if (showSlicePlane.get()) {
      drawBox(g);
    }

    if (showSlice.get()) {
      if (enableSliceEdge.get()) {
        drawSliceEdges(g);
      }
      drawSlice(g);
    }

    g.popMatrix();
  }

  void drawLattice(Graphics &g) {
    // g.color(latticeSphereColor.get());
    // lattice->drawLattice(g, latticeSphere, latticeSphereSize.get());
    lattice->updateBuffer(latticeOffsets);

    g.shader(instancing_shader);
    g.shader().uniform("scale", latticeSphereSize.get());
    g.shader().uniform("color", latticeSphereColor.get());
    g.update();

    latticeSphere.vao().bind();
    latticeSphere.indexBuffer().bind();
    glDrawElementsInstanced(GL_TRIANGLES, latticeSphere.indices().size(),
                            GL_UNSIGNED_INT, 0, lattice->getLatticeVertexNum());
  }

  void drawLatticeEdges(Graphics &g) {
    g.color(latticeEdgeColor.get());
    lattice->drawEdges(g);
  }

  void drawLatticeParticles(Graphics &g) {
    lattice->drawParticles(g, latticeSphere);
  }

  void drawSlice(Graphics &g) {
    g.color(slicePlaneColor.get());
    slice->drawPlane(g);

    // g.color(sliceSphereColor.get());
    slice->updateBuffer(sliceOffsets);

    g.shader(instancing_shader);
    g.shader().uniform("scale", sliceSphereSize.get());
    g.shader().uniform("color", sliceSphereColor.get());
    g.update();

    sliceSphere.vao().bind();
    sliceSphere.indexBuffer().bind();
    glDrawElementsInstanced(GL_TRIANGLES, sliceSphere.indices().size(),
                            GL_UNSIGNED_INT, 0, slice->getSliceVertexNum());

    // slice->drawSlice(g, sliceSphere, sliceSphereSize.get());
  }

  void drawSliceEdges(Graphics &g) {
    // g.color(sliceEdgeColor.get());
    // slice->drawEdges(g);

    slice->updateEdgeBuffers(edgeOffsets, edgeEndOffsets);

    g.shader(edge_instancing_shader);
    g.shader().uniform("color", sliceEdgeColor.get());
    g.update();

    edgeLine.vao().bind();
    glDrawArraysInstanced(GL_LINES, 0, edgeLine.vertices().size(),
                          slice->getSliceEdgeNum());
  }

  void drawBox(Graphics &g) {
    // TODO: edit box colors
    g.color(0.3f, 0.3f, 1.0f, 0.1f);
    slice->drawBox(g);

    g.color(1.f, 1.f, 0.f, 0.2f);
    slice->drawBoxNodes(g, sliceSphere);
  }

  bool registerCallbacks() {
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

      if (newAxis.size() > 3) {
        newAxis.resize(3);
      }
      lattice->setAxis(newAxis);
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

      if (newAxis.size() > 3) {
        newAxis.resize(3);
      }
      lattice->setAxis(newAxis);
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

      if (newAxis.size() > 3) {
        newAxis.resize(3);
      }
      lattice->setAxis(newAxis);
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

      if (newAxis.size() > 3) {
        newAxis.resize(3);
      }
      lattice->setAxis(newAxis);
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

      if (newAxis.size() > 3) {
        newAxis.resize(3);
      }
      lattice->setAxis(newAxis);
    });

    particle1.registerChangeCallback(
        [&](float value) { lattice->setParticle(value, 0); });

    particle2.registerChangeCallback(
        [&](float value) { lattice->setParticle(value, 1); });

    particle3.registerChangeCallback(
        [&](float value) { lattice->setParticle(value, 2); });

    particle4.registerChangeCallback(
        [&](float value) { lattice->setParticle(value, 3); });

    particle5.registerChangeCallback(
        [&](float value) { lattice->setParticle(value, 4); });

    particleSphereSize.registerChangeCallback(
        [&](float value) { lattice->setParticleSize(value); });

    particleSphereColor.registerChangeCallback(
        [&](Color value) { lattice->setParticleColor(value); });

    addParticle.registerChangeCallback(
        [&](bool value) { lattice->addParticle(); });

    removeParticle.registerChangeCallback(
        [&](bool value) { lattice->removeParticle(); });

    enableLatticeEdge.registerChangeCallback(
        [&](bool value) { lattice->enableEdges = value; });

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

    recreateEdges.registerChangeCallback([&](bool value) {
      slice->recreateEdges = value;
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

    return true;
  }

private:
  std::shared_ptr<AbstractLattice> lattice;
  std::shared_ptr<AbstractSlice> slice;

  VAOMesh latticeSphere, sliceSphere, edgeLine;
  BufferObject latticeOffsets, sliceOffsets, edgeOffsets, edgeEndOffsets;
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
  ParameterBool enableLatticeEdge{"enableLatticeEdge", "", 0};
  ParameterBool showSlice{"showSlice", "", 1};
  ParameterBool enableSliceEdge{"enableSliceEdge", "", 0};
  ParameterBool showSlicePlane{"showSlicePlane", "", 0};
  ParameterBool lockCameraToSlice{"lockCameraToSlice", "", 0};

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

  ParameterBool recreateEdges{"recreateEdges", ""};
  Parameter edgeThreshold{"edgeThreshold", "", 1.1f, 0.f, 2.f};

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
};

#endif // CRYSTAL_VIEWER_HPP