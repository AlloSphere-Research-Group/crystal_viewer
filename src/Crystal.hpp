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
#include <variant>
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

  virtual void drawLattice(Graphics &g, VAOMesh &mesh) = 0;
};

template <int N> struct Lattice : LatticeBase {
  std::vector<Vec<N, float>> basis;
  std::vector<Vec<N, int>> vertexIdx;
  std::vector<std::pair<int, int>> edgeIdx;
  std::vector<Vec<N, float>> vertices;

  VAOMesh edges;

  Lattice() {
    dim = N;

    for (int i = 0; i < N; ++i) {
      Vec<N, float> newVec(0);
      newVec[i] = 1.f;
      basis.push_back(newVec);
    }

    update();
  }

  void clear() {
    vertexIdx.clear();
    edgeIdx.clear();
    vertices.clear();
  }

  void update(int size = 5) {
    clear();

    Vec<N, int> newIdx(-size);
    vertexIdx.push_back(newIdx);

    while (true) {
      newIdx[0] += 1;
      for (int i = 0; i < N - 1; ++i) {
        if (newIdx[i] > size) {
          newIdx[i] = -size;
          newIdx[i + 1] += 1;
        }
      }
      if (newIdx[N - 1] > size)
        break;

      vertexIdx.push_back(newIdx);
    }

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

  virtual void update(float m1, float m2, float m3, float windowSize) = 0;
  virtual void drawSlice(Graphics &g, VAOMesh &mesh) = 0;
  virtual void drawPlane(Graphics &g) = 0;
};

template <int N, int M> struct Slice : SliceBase {
  Lattice<N> *lattice;

  std::vector<Vec<N, float>> millerIdx;
  std::vector<Vec<N, float>> normal;

  std::vector<Vec<N, float>> planeVertices;
  VAOMesh slicePlane, planeEdges;

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

  virtual void update(float m1, float m2, float m3, float windowSize) {
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
    for (int i = 0; i < lattice->vertexIdx.size(); ++i) {
      Vec<N - M, float> dist = distanceToPlane(lattice->vertices[i]);
      if (dist.mag() < windowSize) {
        // TODO: move along bivector
        Vec<N, float> projVert = lattice->vertices[i] - dist[0] * normal[0];
        planeVertices.push_back(projVert);
      }
    }

    planeEdges.reset();
    planeEdges.primitive(Mesh::LINES);
    for (auto &e : lattice->edgeIdx) {
      Vec<N - M, float> dist1 = distanceToPlane((lattice->vertices[e.first]));
      Vec<N - M, float> dist2 = distanceToPlane((lattice->vertices[e.second]));
      if (dist1.mag() < windowSize && dist2.mag() < windowSize) {
        planeEdges.vertex(lattice->vertices[e.first] - dist1[0] * normal[0]);
        planeEdges.vertex(lattice->vertices[e.second] - dist2[0] * normal[0]);
      }
    }
    planeEdges.update();
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

  void generate(int newDim = 3) {
    // TODO: transfer values to new lattice
    // if (lattice)
    //   lattice.reset(nullptr);
    // if (slice)
    //   slice.reset(nullptr);
    switch (newDim) {
    case 3: {
      auto newLattice = std::make_shared<Lattice3>();
      auto newSlice = std::make_shared<Slice<3, 2>>();
      newSlice->init(newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
      break;
    }
    case 4: {
      auto newLattice = std::make_shared<Lattice4>();
      auto newSlice = std::make_shared<Slice<4, 2>>();
      newSlice->init(newLattice);

      lattice = std::static_pointer_cast<LatticeBase>(newLattice);
      slice = std::static_pointer_cast<SliceBase>(newSlice);
      break;
    }
    case 5: {
      auto newLattice = std::make_shared<Lattice5>();
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

  void drawLattice(Graphics &g) {
    g.color(1.f, 0.2f);
    lattice->drawLattice(g, sphereMesh);
  }

  void drawSlice(Graphics &g) {
    g.color(1.f, 0.8f);
    slice->drawSlice(g, sphereMesh);

    g.color(0.2f, 0.2f, 0.5f, 0.3f);
    slice->drawPlane(g);
  }
};

#endif // CRYSTAL_HPP