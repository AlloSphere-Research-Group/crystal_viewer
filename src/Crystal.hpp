#ifndef CRYSTAL_HPP
#define CRYSTAL_HPP

/*
 * Copyright 2022 AlloSphere Research Group
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
 *        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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
 * authors: Kon Hyong Kim
 */

#include <memory>
#include <mutex>
#include <vector>

#include "al/graphics/al_Graphics.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_VAOMesh.hpp"
#include "al/math/al_Quat.hpp"
#include "al/math/al_Vec.hpp"

using namespace al;

struct LatticeBase {
  int dim;

  virtual ~LatticeBase() {}

  virtual void setBasis(float value, unsigned int basisNum,
                        unsigned int vecIdx) = 0;
  virtual float getBasis(unsigned int basisNum, unsigned int vecIdx) = 0;
  virtual void createLattice(int size) = 0;
  virtual void update() = 0;
  virtual void drawLattice(Graphics &g, VAOMesh &mesh) = 0;
};

template <int N> struct Lattice : LatticeBase {
  std::vector<Vec<N, float>> basis;
  std::vector<Vec<N, int>> vertexIdx;
  std::vector<std::pair<int, int>> edgeIdx;
  std::vector<Vec<N, float>> vertices;

  int latticeSize{3};
  VAOMesh edges;

  Lattice() {
    dim = N;

    for (int i = 0; i < N; ++i) {
      Vec<N, float> newVec(0);
      newVec[i] = 1.f;
      basis.push_back(newVec);
    }

    createLattice();
  }

  Lattice(std::shared_ptr<LatticeBase> oldLattice) {
    dim = N;

    if (oldLattice == nullptr) {
      for (int i = 0; i < N; ++i) {
        Vec<N, float> newVec(0);
        newVec[i] = 1.f;
        basis.push_back(newVec);
      }
    } else {
      int oldDim = oldLattice->dim;

      for (int i = 0; i < dim; ++i) {
        Vec<N, float> newVec(0);
        if (i < oldDim) {
          for (int j = 0; j < dim && j < oldDim; ++j) {
            newVec[j] = oldLattice->getBasis(i, j);
          }
        } else {
          newVec[i] = 1.f;
        }
        basis.push_back(newVec);
      }
    }

    createLattice();
  }

  virtual void setBasis(float value, unsigned int basisNum,
                        unsigned int vecIdx) {
    if (basisNum >= N || vecIdx >= N) {
      std::cerr << "Error: Basis vector write index out of bounds" << std::endl;
      return;
    }
    basis[basisNum][vecIdx] = value;

    update();
  }

  virtual float getBasis(unsigned int basisNum, unsigned int vecIdx) {
    if (basisNum >= N || vecIdx >= N) {
      std::cerr << "Error: Basis vector read index out of bounds" << std::endl;
      return 0.f;
    }
    return basis[basisNum][vecIdx];
  }

  virtual void createLattice(int size) {
    latticeSize = size;
    createLattice();
  }

  void createLattice() {
    vertexIdx.clear();

    Vec<N, int> newIdx(-latticeSize);
    vertexIdx.push_back(newIdx);

    while (true) {
      newIdx[0] += 1;
      for (int i = 0; i < N - 1; ++i) {
        if (newIdx[i] > latticeSize) {
          newIdx[i] = -latticeSize;
          newIdx[i + 1] += 1;
        }
      }
      if (newIdx[N - 1] > latticeSize)
        break;

      vertexIdx.push_back(newIdx);
    }

    update();
  }

  void clear() {
    edgeIdx.clear();
    vertices.clear();
  }

  virtual void update() {
    // TODO: check if clear can be avoided
    clear();

    for (auto &v : vertexIdx) {
      Vec<N, float> newVec(0);
      for (int i = 0; i < N; ++i) {
        newVec += (float)v[i] * basis[i];
      }
      vertices.push_back(newVec);
    }

    for (int i = 0; i < vertexIdx.size() - 1; ++i) {
      for (int j = i + 1; j < vertexIdx.size(); ++j) {
        if ((vertexIdx[i] - vertexIdx[j]).magSqr() == 1) {
          edgeIdx.push_back({i, j});
        }
      }
    }

    edges.reset();
    edges.primitive(Mesh::LINES);
    for (auto &e : edgeIdx) {
      edges.vertex(vertices[e.first]);
      edges.vertex(vertices[e.second]);
    }
    edges.update();
  }

  virtual void drawLattice(Graphics &g, VAOMesh &mesh) {
    for (auto &v : vertices) {
      g.pushMatrix();
      // TODO: consider projection
      g.translate(v[0], v[1], v[2]);
      g.draw(mesh);
      g.popMatrix();
    }

    g.draw(edges);
  }
};

struct SliceBase {
  int latticeDim;
  int sliceDim;
  float windowSize{15.f};
  float windowDepth{0.f};

  virtual ~SliceBase(){};

  virtual float getMiller(int millerNum, unsigned int index) = 0;
  virtual void setMiller(float value, int millerNum, unsigned int index) = 0;
  virtual void setWindow(float newWindowSize, float newWindowDepth) = 0;
  virtual void update() = 0;
  virtual void drawSlice(Graphics &g, VAOMesh &mesh) = 0;
  virtual void drawPlane(Graphics &g) = 0;
  virtual void drawBox(Graphics &g) = 0;
  virtual void drawBoxNodes(Graphics &g, VAOMesh &mesh) = 0;
};

template <int N, int M> struct Slice : SliceBase {
  Lattice<N> *lattice;

  std::vector<Vec<N, float>> millerIdx;
  std::vector<Vec<N, float>> normal;

  std::vector<Vec<N, float>> planeVertices;
  std::vector<unsigned int> boxVertices;
  VAOMesh slicePlane, sliceBox, planeEdges, boxEdges;

  Slice() {
    latticeDim = N;
    sliceDim = M;
    millerIdx.resize(N - M);
    normal.resize(N - M);

    for (int i = 0; i < N - M; ++i) {
      millerIdx[i] = 0.f;
      millerIdx[i][i] = 1.f;
    }
  }

  Slice(std::shared_ptr<SliceBase> oldSlice,
        std::shared_ptr<Lattice<N>> latticePtr) {
    latticeDim = N;
    sliceDim = M;
    millerIdx.resize(N - M);
    normal.resize(N - M);
    lattice = latticePtr.get();

    if (oldSlice == nullptr) {
      for (int i = 0; i < N - M; ++i) {
        millerIdx[i] = 0.f;
        millerIdx[i][i] = 1.f;
      }
    } else {
      windowSize = oldSlice->windowSize;
      windowDepth = oldSlice->windowDepth;
      int oldLatticeDim = oldSlice->latticeDim;
      int oldSliceDim = oldSlice->sliceDim;

      for (int i = 0; i < N - M; ++i) {
        millerIdx[i] = 0.f;
        if (i < oldLatticeDim - oldSliceDim) {
          for (int j = 0; j < N && j < oldLatticeDim; ++j) {
            millerIdx[i][j] = oldSlice->getMiller(i, j);
          }
        } else {
          millerIdx[i][i] = 1.f;
        }
      }
    }

    updateWindow();
    update();
  }

  virtual float getMiller(int millerNum, unsigned int index) {
    return millerIdx[millerNum][index];
  }

  virtual void setMiller(float value, int millerNum, unsigned int index) {
    if (millerNum >= N - M || index >= N) {
      std::cerr << "Error: Miller write index out of bounds" << std::endl;
      return;
    }

    millerIdx[millerNum][index] = value;

    update();
  }

  virtual void setWindow(float newWindowSize, float newWindowDepth) {
    windowSize = newWindowSize;
    windowDepth = newWindowDepth;

    updateWindow();
  }

  void updateWindow() {
    slicePlane.reset();
    addSurface(slicePlane, 2, 2, windowSize, windowSize);
    slicePlane.update();

    sliceBox.reset();
    addCuboid(sliceBox, 0.5f * windowSize, 0.5f * windowSize, windowDepth);
    sliceBox.update();
  }

  Vec<N - M, float> project(Vec<N, float> &point) {
    Vec<N - M, float> projVec{0.f};

    // normal vector has been normalized
    for (int i = 0; i < N - M; ++i) {
      projVec[i] = point.dot(normal[i]);
    }

    return projVec;
  }

  virtual void update() {
    for (int i = 0; i < N - M; ++i) {
      normal[i] = 0;
      for (int j = 0; j < N; ++j) {
        normal[i] += millerIdx[i][j] * lattice->basis[j];
      }
      normal[i].normalize();
    }

    planeVertices.clear();
    boxVertices.clear();
    for (int i = 0; i < lattice->vertexIdx.size(); ++i) {
      Vec<N - M, float> dist = project(lattice->vertices[i]);
      if (dist.mag() < windowDepth) {
        boxVertices.push_back(i);
        Vec<N, float> projVertex = lattice->vertices[i];
        for (int j = 0; j < N - M; ++j) {
          projVertex -= dist[j] * normal[j];
        }
        planeVertices.push_back(projVertex);
      }
    }

    planeEdges.reset();
    planeEdges.primitive(Mesh::LINES);
    boxEdges.reset();
    boxEdges.primitive(Mesh::LINES);
    for (auto &e : lattice->edgeIdx) {
      Vec<N - M, float> dist1 = project((lattice->vertices[e.first]));
      Vec<N - M, float> dist2 = project((lattice->vertices[e.second]));
      if (dist1.mag() < windowDepth && dist2.mag() < windowDepth) {
        boxEdges.vertex(lattice->vertices[e.first]);
        boxEdges.vertex(lattice->vertices[e.second]);
        Vec<N, float> projVertex1 = lattice->vertices[e.first];
        Vec<N, float> projVertex2 = lattice->vertices[e.second];
        for (int i = 0; i < N - M; ++i) {
          projVertex1 -= dist1[i] * normal[i];
          projVertex2 -= dist2[i] * normal[i];
        }
        planeEdges.vertex(projVertex1);
        planeEdges.vertex(projVertex2);
      }
    }
    planeEdges.update();
    boxEdges.update();
  }

  virtual void drawSlice(Graphics &g, VAOMesh &mesh) {
    for (auto &v : planeVertices) {
      g.pushMatrix();
      // TODO: consider projection
      g.translate(v[0], v[1], v[2]);
      g.scale(2.f);
      g.draw(mesh);
      g.popMatrix();
    }

    g.draw(planeEdges);
  }

  virtual void drawPlane(Graphics &g) {
    g.pushMatrix();
    // TODO: calculate bivector
    Quatf rot = Quatf::getRotationTo(
        Vec3f(0.f, 0.f, 1.f), Vec3f(normal[0][0], normal[0][1], normal[0][2]));
    g.rotate(rot);
    g.draw(slicePlane);
    g.popMatrix();
  }

  virtual void drawBox(Graphics &g) {
    g.pushMatrix();
    // TODO: calculate bivector
    Quatf rot = Quatf::getRotationTo(
        Vec3f(0.f, 0.f, 1.f), Vec3f(normal[0][0], normal[0][1], normal[0][2]));
    g.rotate(rot);
    g.draw(sliceBox);
    g.popMatrix();
  }

  virtual void drawBoxNodes(Graphics &g, VAOMesh &mesh) {
    for (auto &i : boxVertices) {
      g.pushMatrix();
      // TODO: consider projection
      g.translate(lattice->vertices[i][0], lattice->vertices[i][1],
                  lattice->vertices[i][2]);
      g.scale(2.f);
      g.draw(mesh);
      g.popMatrix();
    }

    g.draw(boxEdges);
  }
};

struct CrystalViewer {
  int dim{3};
  std::shared_ptr<LatticeBase> lattice;
  std::shared_ptr<SliceBase> slice;

  VAOMesh sphereMesh;

  void init() {
    addSphere(sphereMesh, 0.02);
    sphereMesh.update();
  }

  void generate(int newDim) {
    switch (newDim) {
    case 3: {
      auto newLattice = std::make_shared<Lattice<3>>(lattice);
      auto newSlice = std::make_shared<Slice<3, 2>>(slice, newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
      break;
    }
    case 4: {
      auto newLattice = std::make_shared<Lattice<4>>(lattice);
      auto newSlice = std::make_shared<Slice<4, 2>>(slice, newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
      break;
    }
    case 5: {
      auto newLattice = std::make_shared<Lattice<5>>(lattice);
      auto newSlice = std::make_shared<Slice<5, 2>>(slice, newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
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

  float getBasis(int basisNum, unsigned int vecIdx) {
    return lattice->getBasis(basisNum, vecIdx);
  }

  void setBasis(float value, int basisNum, unsigned int vecIdx) {
    lattice->setBasis(value, basisNum, vecIdx);
    updateSlice();
  }

  float getMiller(int millerNum, unsigned int index) {
    return slice->getMiller(millerNum, index);
  }

  void setMiller(float value, int millerNum, unsigned index) {
    slice->setMiller(value, millerNum, index);
  }

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
};

#endif // CRYSTAL_HPP