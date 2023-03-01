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

#include "al/graphics/al_Graphics.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_VAOMesh.hpp"

#include "Lattice.hpp"
#include "Slice.hpp"

class CrystalViewer {
public:
  void init() {
    addSphere(sphereMesh, 1.0);
    sphereMesh.update();
  }

  void generate(int newDim) {
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
      auto newSlice = std::make_shared<Slice<4, 2>>(slice, newLattice);

      lattice = newLattice;
      slice = newSlice;
      break;
    }
    case 5: {
      auto newLattice = std::make_shared<Lattice<5>>(lattice);
      auto newSlice = std::make_shared<Slice<5, 2>>(slice, newLattice);

      lattice = newLattice;
      slice = newSlice;
      break;
    }
    default:
      std::cerr << "Dimension " << newDim << " not supported." << std::endl;
      return;
    }

    dim = newDim;
  }

  void createLattice(int latticeSize) {
    lattice->createLattice(latticeSize);
    updateSlice();
  }

  int getDim() { return dim; }

  float getBasis(int basisNum, unsigned int vecIdx) {
    return lattice->getBasis(basisNum, vecIdx);
  }

  void setBasis(float value, int basisNum, unsigned int vecIdx) {
    lattice->setBasis(value, basisNum, vecIdx);
    if (autoUpdate) {
      updateSlice();
    }
  }

  void resetBasis() {
    lattice->resetBasis();
    updateSlice();
  }

  void update() {
    updateLattice();
    updateSlice();
  }

  inline void updateLattice() { lattice->update(); }

  void setAxis(std::vector<int> &newAxis) {
    std::vector<int> axisIdx;
    for (int i = 0; i < 3 && i < newAxis.size(); ++i) {
      axisIdx.push_back(newAxis[i]);
    }
    lattice->setAxis(axisIdx);
  }

  void setParticle(float value, unsigned int index) {
    lattice->setParticle(value, index);
  }

  void addParticle() { lattice->addParticle(); }

  void removeParticle() { lattice->removeParticle(); }

  float getMiller(int millerNum, unsigned int index) {
    return slice->getMiller(millerNum, index);
  }

  void setMiller(float value, int millerNum, unsigned int index) {
    slice->setMiller(value, millerNum, index);
  }

  void roundMiller() { slice->roundMiller(); }

  Vec3f getNormal() { return slice->getNormal(); }

  // TODO: see if var can be localized to viewer
  void setWindow(float windowSize, float windowDepth) {
    slice->setWindow(windowSize, windowDepth);
  }

  inline void updateSlice() { slice->update(); }

  void drawLattice(Graphics &g) {
    g.color(latticeSphereColor);
    lattice->drawLattice(g, sphereMesh, latticeSphereSize);
  }

  void drawLatticeEdges(Graphics &g) {
    g.color(latticeEdgeColor);
    lattice->drawEdges(g);
  }

  void drawLatticeParticles(Graphics &g) {
    lattice->drawParticles(g, sphereMesh);
  }

  void drawSlice(Graphics &g) {
    g.color(slicePlaneColor);
    slice->drawPlane(g);

    g.color(sliceSphereColor);
    slice->drawSlice(g, sphereMesh, sliceSphereSize);
  }

  void drawSliceEdges(Graphics &g) {
    g.color(sliceEdgeColor);
    slice->drawEdges(g);
  }

  void drawBox(Graphics &g) {
    // TODO: edit box colors
    g.color(0.3f, 0.3f, 1.0f, 0.1f);
    slice->drawBox(g);

    g.color(1.f, 1.f, 0.f, 0.2f);
    slice->drawBoxNodes(g, sphereMesh);
  }

  void exportSliceTxt(std::string &filePath) { slice->exportToTxt(filePath); }

  void exportSliceJson(std::string &filePath) { slice->exportToJson(filePath); }

  void setAutoUpdate(const bool &update) {
    autoUpdate = update;
    lattice->autoUpdate = update;
    slice->autoUpdate = update;
  }

  void enableLatticeEdges(const bool &value) { lattice->enableEdges = value; }

  void enableSliceEdges(const bool &value) { slice->enableEdges = value; }

  inline void setParticlesphereSize(const float &newSize) {
    lattice->setParticleSize(newSize);
  }

  inline void setParticleSphereColor(const Color &newColor) {
    lattice->setParticleColor(newColor);
  }

  inline void setLatticeSphereSize(const float &newSize) {
    latticeSphereSize = newSize;
  }
  inline void setLatticeSphereColor(const Color &newColor) {
    latticeSphereColor = newColor;
  }
  inline void setLatticeEdgeColor(const Color &newColor) {
    latticeEdgeColor = newColor;
  }
  inline void setSliceSphereSize(const float &newSize) {
    sliceSphereSize = newSize;
  }
  inline void setSliceSphereColor(const Color &newColor) {
    sliceSphereColor = newColor;
  }
  inline void setSliceEdgeColor(const Color &newColor) {
    sliceEdgeColor = newColor;
  }
  inline void setSlicePlaneSize(const float &newSize) {
    slicePlaneSize = newSize;
  }
  inline void setSlicePlaneColor(const Color &newColor) {
    slicePlaneColor = newColor;
  }

private:
  int dim{3};
  bool autoUpdate{true};

  std::shared_ptr<AbstractLattice> lattice;
  std::shared_ptr<AbstractSlice> slice;

  VAOMesh sphereMesh;

  float particleSphereSize{0.02f};
  float latticeSphereSize{0.02f};
  Color latticeSphereColor{1.f, 0.8f};
  Color latticeEdgeColor{1.f, 0.5f};
  float sliceSphereSize{0.04f};
  Color sliceSphereColor{1.f};
  Color sliceEdgeColor{1.f, 0.5f};
  float slicePlaneSize{15.f};
  Color slicePlaneColor{0.3f, 0.3f, 1.f, 0.3f};
};

#endif // CRYSTAL_VIEWER_HPP