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
    addSphere(sphereMesh, sphereSize);
    sphereMesh.update();
  }

  void setLatticeSphereSize(float newSize) {
    sphereMesh.fitToSphere(newSize);
    sphereMesh.update();
  }

  void generate(int newDim) {
    switch (newDim) {
    case 3: {
      auto newLattice = std::make_shared<Lattice<3>>(lattice);
      auto newSlice = std::make_shared<Slice<3, 2>>(slice, newLattice);

      lattice = std::static_pointer_cast<AbstractLattice>(newLattice);
      slice = std::static_pointer_cast<AbtractSlice>(newSlice);
      break;
    }
    case 4: {
      auto newLattice = std::make_shared<Lattice<4>>(lattice);
      auto newSlice = std::make_shared<Slice<4, 2>>(slice, newLattice);

      lattice = std::static_pointer_cast<AbstractLattice>(newLattice);
      slice = std::static_pointer_cast<AbtractSlice>(newSlice);
      break;
    }
    case 5: {
      auto newLattice = std::make_shared<Lattice<5>>(lattice);
      auto newSlice = std::make_shared<Slice<5, 2>>(slice, newLattice);

      lattice = std::static_pointer_cast<AbstractLattice>(newLattice);
      slice = std::static_pointer_cast<AbtractSlice>(newSlice);
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

  void setEdgeThreshold(float edgeThreshold) {
    slice->setEdgeThreshold(edgeThreshold);
    updateSlice();
  }

  int getDim() { return dim; }

  float getBasis(int basisNum, unsigned int vecIdx) {
    return lattice->getBasis(basisNum, vecIdx);
  }

  void setBasis(float value, int basisNum, unsigned int vecIdx,
                bool callUpdate = true) {
    lattice->setBasis(value, basisNum, vecIdx);
    // TODO: add autoupdate to slice
    if (callUpdate) {
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

  void updateLattice() { lattice->update(); }

  float getMiller(int millerNum, unsigned int index) {
    return slice->getMiller(millerNum, index);
  }

  void setMiller(float value, int millerNum, unsigned index,
                 bool callUpdate = true) {
    slice->setMiller(value, millerNum, index, callUpdate);
  }

  void roundMiller(bool callUpdate = true) { slice->roundMiller(callUpdate); }

  Vec3f getNormal() { return slice->getNormal(); }

  void setWindow(float windowSize, float windowDepth) {
    slice->setWindow(windowSize, windowDepth);
  }

  void updateSlice() { slice->update(); }

  void drawLattice(Graphics &g) {
    g.color(1.f, 0.1f);
    lattice->drawLattice(g, sphereMesh);
  }

  void drawSlice(Graphics &g) {
    g.color(0.f, 0.f, 1.f, 0.3f);
    slice->drawPlane(g);

    g.color(1.f, 0.8f);
    slice->drawSlice(g, sphereMesh);
  }

  void drawBox(Graphics &g) {
    g.color(0.3f, 0.3f, 1.0f, 0.1f);
    slice->drawBox(g);

    g.color(1.f, 1.f, 0.f, 0.2f);
    slice->drawBoxNodes(g, sphereMesh);
  }

  void exportSliceTxt(std::string &filePath) { slice->exportToTxt(filePath); }

  void exportSliceJson(std::string &filePath) { slice->exportToJson(filePath); }

private:
  int dim{3};

  std::shared_ptr<AbstractLattice> lattice;
  std::shared_ptr<AbtractSlice> slice;

  VAOMesh sphereMesh;

  float sphereSize{0.02};

  Color latticeSphereColor;
  Color latticeEdgeColor;
  Color sliceSphereColor;
  Color sliceEdgeColor;
};

#endif // CRYSTAL_VIEWER_HPP