#ifndef CRYSTAL_HPP
#define CRYSTAL_HPP
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

#include <cmath>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "al/graphics/al_Graphics.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_VAOMesh.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Quat.hpp"
#include "al/math/al_Vec.hpp"

using namespace al;

struct AbstractLattice {
  virtual void createLattice(int size) = 0;
  virtual void update() = 0;
  virtual void updateEdges() = 0;

  virtual void setBasis(float value, unsigned int basisNum,
                        unsigned int vecIdx) = 0;
  virtual float getBasis(unsigned int basisNum, unsigned int vecIdx) = 0;
  virtual void resetBasis() = 0;

  virtual int getVertexNum() = 0;
  virtual int getEdgeNum() = 0;

  virtual void updateBuffer(BufferObject &buffer,
                            BufferObject &colorBuffer) = 0;
  virtual void updateEdgeBuffers(BufferObject &startBuffer,
                                 BufferObject &endBuffer) = 0;

  int dim;
  int latticeSize{1};

  bool enableEdges{false};

  bool dirty{false};
  bool dirtyEdges{false};
};

template <int N> struct Lattice : AbstractLattice {
  std::array<Vec<N, float>, N> basis;
  std::vector<Vec<N, float>> extraPoints;

  std::vector<Vec<N, float>> unitCell;

  std::vector<Vec<N, int>> vertexIdx;
  std::vector<std::pair<int, int>> edgeIdx;

  std::vector<Vec<N, float>> vertices;

  std::vector<Vec3f> projectedVertices;
  std::vector<Vec3f> edgeStarts;
  std::vector<Vec3f> edgeEnds;

  // TODO: remove dummyColor
  Color dummyColor;

  Lattice() {
    dim = N;

    for (int i = 0; i < N; ++i) {
      Vec<N, float> newVec(0);
      newVec[i] = 1.f;
      basis[i] = newVec;
    }

    createLattice();
  }

  Lattice(std::shared_ptr<AbstractLattice> oldLattice) {
    dim = N;

    if (oldLattice == nullptr) {
      for (int i = 0; i < N; ++i) {
        Vec<N, float> newVec(0);
        newVec[i] = 1.f;

        basis[i] = newVec;
      }
    } else {
      enableEdges = oldLattice->enableEdges;

      for (int i = 0; i < dim; ++i) {
        Vec<N, float> newVec(0);
        if (i < oldLattice->dim) {
          for (int j = 0; j < std::min(dim, oldLattice->dim); ++j) {
            newVec[j] = oldLattice->getBasis(i, j);
          }
        } else {
          newVec[i] = 1.f;
        }

        basis[i] = newVec;
      }
    }

    createLattice();
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

    vertices.resize(vertexIdx.size());
    projectedVertices.resize(vertexIdx.size());

    // edgeIdx.clear();

    // for (int i = 0; i < vertexIdx.size() - 1; ++i) {
    //   for (int j = i + 1; j < vertexIdx.size(); ++j) {
    //     if ((vertexIdx[i] - vertexIdx[j]).sumAbs() == 1) {
    //       edgeIdx.push_back({i, j});
    //     }
    //   }
    // }

    // edgeStarts.resize(edgeIdx.size());
    // edgeEnds.resize(edgeIdx.size());

    update();
  }

  virtual void update() {
    dirty = true;

    for (int i = 0; i < vertexIdx.size(); ++i) {
      Vec<N, float> newVec(0);
      for (int j = 0; j < N; ++j) {
        newVec += (float)vertexIdx[i][j] * basis[j];
      }
      vertices[i] = newVec;
      projectedVertices[i] = project(newVec);
    }

    updateEdges();
  }

  virtual void updateEdges() {
    if (!enableEdges)
      return;

    dirtyEdges = true;

    for (int i = 0; i < edgeIdx.size(); ++i) {
      // edgeStarts[i] = projectedVertices[edgeIdx[i].first];
      // edgeEnds[i] = projectedVertices[edgeIdx[i].second];
    }
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

  virtual void resetBasis() {
    for (int i = 0; i < N; ++i) {
      Vec<N, float> newVec(0);
      newVec[i] = 1.f;
      basis[i] = newVec;
    }

    update();
  }

  virtual float getBasis(unsigned int basisNum, unsigned int vecIdx) {
    if (basisNum >= N || vecIdx >= N) {
      std::cerr << "Error: Basis vector read index out of bounds" << std::endl;
      return 0.f;
    }
    return basis[basisNum][vecIdx];
  }

  virtual int getVertexNum() { return projectedVertices.size(); }
  virtual int getEdgeNum() { return edgeStarts.size(); }

  // Vec<N - 1, float> stereographicProjection(Vec<N, float>& point) {
  //   Vec<N - 1, float> newPoint;
  //   for (int i = 0; i < N - 1; ++i) {
  //     newPoint[i] = point[i] / (1.f - point[N - 1]);
  //   }
  //   return newPoint;
  // }

  // Vec3f stereographic(Vec<N, float> &point) {
  //   if (N < 4) {
  //     Vec3f newPoint;
  //     for (int i = 0; i < 2; ++i) {
  //       newPoint[i] = point[i] / (4.f - point[2]);
  //     }
  //     newPoint[2] = 0.f;
  //     return newPoint;
  //   }

  //  Vec4f newPoint4;
  //  if (N > 5) {
  //    std::cerr << "ERROR: Higher than 5D" << std::endl;
  //    return Vec3f{0};
  //  } else if (N == 5) {
  //    for (int i = 0; i < 4; ++i) {
  //      newPoint4[i] = point[i] / (4.f - point[4]);
  //    }
  //  } else if (N == 4) {
  //    newPoint4 = point;
  //  }

  //  Vec3f newPoint;
  //  for (int i = 0; i < 3; ++i) {
  //    newPoint[i] = newPoint4[i] / (4.f - newPoint4[3]);
  //  }

  //  return newPoint;
  //}

  inline Vec3f project(Vec<N, float> &point) {
    return Vec3f{point[0], point[1], point[2]};
  }

  virtual void updateBuffer(BufferObject &buffer, BufferObject &colorBuffer) {
    if (dirty) {
      buffer.bind();
      buffer.data(projectedVertices.size() * 3 * sizeof(float),
                  projectedVertices.data());

      colorBuffer.bind();
      colorBuffer.data(4 * sizeof(float), &dummyColor);
      dirty = false;
    }
  }

  virtual void updateEdgeBuffers(BufferObject &startBuffer,
                                 BufferObject &endBuffer) {
    if (dirtyEdges) {
      startBuffer.bind();
      startBuffer.data(edgeStarts.size() * 3 * sizeof(float),
                       edgeStarts.data());

      endBuffer.bind();
      endBuffer.data(edgeEnds.size() * 3 * sizeof(float), edgeEnds.data());

      dirtyEdges = false;
    }
  }
};

#endif // CRYSTAL_HPP