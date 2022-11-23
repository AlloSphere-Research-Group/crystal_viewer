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

      if (oldDim == 3) {
        auto oldLatticePtr = std::dynamic_pointer_cast<Lattice3>(oldLattice);
        for (int i = 0; i < dim; ++i) {
          Vec<N, float> newVec(0);
          if (i < oldDim) {
            for (int j = 0; j < dim && j < oldDim; ++j) {
              newVec[j] = oldLatticePtr->basis[i][j];
            }
          } else {
            newVec[i] = 1.f;
          }
          basis.push_back(newVec);
        }
      } else if (oldDim == 4) {
        auto oldLatticePtr = std::dynamic_pointer_cast<Lattice4>(oldLattice);
        for (int i = 0; i < dim; ++i) {
          Vec<N, float> newVec(0);
          if (i < oldDim) {
            for (int j = 0; j < dim && j < oldDim; ++j) {
              newVec[j] = oldLatticePtr->basis[i][j];
            }
          } else {
            newVec[i] = 1.f;
          }
          basis.push_back(newVec);
        }
      } else if (oldDim == 5) {
        auto oldLatticePtr = std::dynamic_pointer_cast<Lattice5>(oldLattice);
        for (int i = 0; i < dim; ++i) {
          Vec<N, float> newVec(0);
          if (i < oldDim) {
            for (int j = 0; j < dim && j < oldDim; ++j) {
              newVec[j] = oldLatticePtr->basis[i][j];
            }
          } else {
            newVec[i] = 1.f;
          }
          basis.push_back(newVec);
        }
      } else {
        std::cerr << "Lattice dimension " << oldDim << " not supported."
                  << std::endl;
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

typedef Lattice<3> Lattice3;
typedef Lattice<4> Lattice4;
typedef Lattice<5> Lattice5;

struct SliceBase {
  int latticeDim;
  int sliceDim;
  virtual ~SliceBase(){};

  virtual void update(int millerNum, float m1, float m2, float m3, float m4,
                      float m5, float windowDepth) = 0;
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
  std::vector<Vec<N, float>> boxVertices;
  VAOMesh slicePlane, sliceBox, planeEdges, boxEdges;

  Slice() {
    millerIdx.resize(N - M);
    normal.resize(N - M);
    latticeDim = N;
    sliceDim = M;
  }

  void init(std::shared_ptr<Lattice<N>> latticePtr) {
    addSurface(slicePlane, 2, 2, 15, 15);
    slicePlane.update();

    lattice = latticePtr.get();
  }

  Vec<N - M, float> distanceToPlane(Vec<N, float> &point) {
    Vec<N - M, float> dist{0};

    for (int i = 0; i < N - M; ++i) {
      for (int j = 0; j < N; ++j) {
        dist[i] += point[j] * normal[i][j];
      }
      dist[i] /= normal[i].mag();
    }
    return dist;
  }

  virtual void update(int millerNum, float m1, float m2, float m3, float m4,
                      float m5, float windowDepth) {
    // TODO: generalize to other dimensions
    millerIdx[0][0] = m1;
    millerIdx[0][1] = m2;
    millerIdx[0][2] = m3;

    normal[0] = 0;
    for (int i = 0; i < N; ++i) {
      normal[0] += millerIdx[0][i] * lattice->basis[i];
    }
    normal[0].normalize();

    planeVertices.clear();
    boxVertices.clear();
    for (int i = 0; i < lattice->vertexIdx.size(); ++i) {
      Vec<N - M, float> dist = distanceToPlane(lattice->vertices[i]);
      if (dist.mag() < windowDepth) {
        // TODO: move along bivector
        Vec<N, float> projVert = lattice->vertices[i];
        boxVertices.push_back(projVert);

        projVert = projVert - dist[0] * normal[0];
        planeVertices.push_back(projVert);
      }
    }

    planeEdges.reset();
    planeEdges.primitive(Mesh::LINES);
    boxEdges.reset();
    boxEdges.primitive(Mesh::LINES);
    for (auto &e : lattice->edgeIdx) {
      Vec<N - M, float> dist1 = distanceToPlane((lattice->vertices[e.first]));
      Vec<N - M, float> dist2 = distanceToPlane((lattice->vertices[e.second]));
      if (dist1.mag() < windowDepth && dist2.mag() < windowDepth) {
        Vec<N, float> edgeVertex1 = lattice->vertices[e.first];
        Vec<N, float> edgeVertex2 = lattice->vertices[e.second];
        boxEdges.vertex(edgeVertex1);
        boxEdges.vertex(edgeVertex2);
        edgeVertex1 = edgeVertex1 - dist1[0] * normal[0];
        edgeVertex2 = edgeVertex2 - dist2[0] * normal[0];
        planeEdges.vertex(edgeVertex1);
        planeEdges.vertex(edgeVertex2);
      }
    }
    planeEdges.update();
    boxEdges.update();

    sliceBox.reset();
    addCuboid(sliceBox, 7.5f, 7.5f, windowDepth);
    sliceBox.update();
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
    for (auto &v : boxVertices) {
      g.pushMatrix();
      // TODO: consider projection
      g.translate(v[0], v[1], v[2]);
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
      auto newLattice = std::make_shared<Lattice3>(lattice);
      auto newSlice = std::make_shared<Slice<3, 2>>();
      newSlice->init(newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
      break;
    }
    case 4: {
      auto newLattice = std::make_shared<Lattice4>(lattice);
      auto newSlice = std::make_shared<Slice<4, 2>>();
      newSlice->init(newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
      break;
    }
    case 5: {
      auto newLattice = std::make_shared<Lattice5>(lattice);
      auto newSlice = std::make_shared<Slice<5, 2>>();
      newSlice->init(newLattice);

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

  void createLattice(int latticeSize) { lattice->createLattice(latticeSize); }

  float getBasis(int basisNum, unsigned int vecIdx) {
    return lattice->getBasis(basisNum, vecIdx);
  }

  void setBasis(float value, int basisNum, unsigned int vecIdx) {
    lattice->setBasis(value, basisNum, vecIdx);
  }

  void drawLattice(Graphics &g) {
    g.color(1.f, 0.1f);
    lattice->drawLattice(g, sphereMesh);
  }

  void updateSlice(int millerNum, float m1, float m2, float m3, float m4,
                   float m5, float windowDepth) {
    slice->update(millerNum, m1, m2, m3, m4, m5, windowDepth);
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