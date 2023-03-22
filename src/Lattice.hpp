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
  virtual void setBasis(float value, unsigned int basisNum,
                        unsigned int vecIdx) = 0;
  virtual void resetBasis() = 0;
  virtual float getBasis(unsigned int basisNum, unsigned int vecIdx) = 0;

  virtual int getLatticeVertexNum() = 0;

  virtual void createLattice(int size) = 0;

  virtual void setAxis(std::vector<int> &newAxis) = 0;

  virtual void setParticle(float value, unsigned int index) = 0;
  virtual void setParticleSize(const float newSize) = 0;
  virtual void setParticleColor(const Color &newColor) = 0;
  virtual void addParticle() = 0;
  virtual void removeParticle() = 0;

  virtual void update() = 0;

  virtual void updateBuffer(BufferObject &buffer) = 0;
  virtual void drawLattice(Graphics &g, VAOMesh &mesh,
                           const float &sphereSize) = 0;
  virtual void drawEdges(Graphics &g) = 0;
  virtual void drawParticles(Graphics &g, VAOMesh &mesh) = 0;

  int dim;
  int latticeSize{1};

  bool enableEdges{false};
  bool dirty{false};
  bool busy{false};
};

template <int N> struct Lattice : AbstractLattice {
  std::array<Vec<N, float>, N> basis;

  std::vector<Vec<N, int>> vertexIdx;
  std::vector<Vec<N, float>> vertices;
  std::vector<Vec3f> projectedVertices;

  std::vector<Vec<N, float>> particles;
  std::vector<Color> particleColors;
  std::vector<float> particleSizes;
  std::vector<Vec<N, float>> particleCoords;

  std::vector<int> viewAxis{0, 1, 2};

  std::vector<std::pair<int, int>> edgeIdx;
  VAOMesh edges;

  Lattice() {
    dim = N;
    for (int i = 0; i < N; ++i) {
      Vec<N, float> newVec(0);
      newVec[i] = 1.f;
      // basis.push_back(newVec);
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
        // basis.push_back(newVec);
        basis[i] = newVec;
      }
    } else {
      for (int i = 0; i < dim; ++i) {
        Vec<N, float> newVec(0);
        if (i < oldLattice->dim) {
          for (int j = 0; j < std::min(dim, oldLattice->dim); ++j) {
            newVec[j] = oldLattice->getBasis(i, j);
          }
        } else {
          newVec[i] = 1.f;
        }
        // basis.push_back(newVec);
        basis[i] = newVec;
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

  virtual int getLatticeVertexNum() { return projectedVertices.size(); }

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

  virtual void setAxis(std::vector<int> &newAxis) { viewAxis = newAxis; }

  virtual void setParticle(float value, unsigned int index) {
    if (particles.empty()) {
      std::cerr << "no particles added" << std::endl;
      return;
    }

    particles.back()[index] = value;

    Vec<N, float> newCoord{0};

    for (int i = 0; i < N; ++i) {
      newCoord += particles.back()[i] * basis[i];
    }
    particleCoords.back() = newCoord;
  }

  virtual void setParticleSize(const float newSize) {
    if (particles.empty()) {
      std::cerr << "no particles added" << std::endl;
      return;
    }
    particleSizes.back() = newSize;
  }

  virtual void setParticleColor(const Color &newColor) {
    if (particles.empty()) {
      std::cerr << "no particles added" << std::endl;
      return;
    }
    particleColors.back() = newColor;
  }

  virtual void addParticle() {
    particles.emplace_back();
    particleColors.emplace_back(1.f, 0.2f, 0.2f, 0.8f);
    particleSizes.emplace_back(0.02f);
    particleCoords.emplace_back();
  }

  virtual void removeParticle() {
    particles.pop_back();
    particleColors.pop_back();
    particleSizes.pop_back();
    particleCoords.pop_back();
  }

  void computeEdges() {
    for (int i = 0; i < vertexIdx.size() - 1; ++i) {
      for (int j = i + 1; j < vertexIdx.size(); ++j) {
        if ((vertexIdx[i] - vertexIdx[j]).sumAbs() == 1) {
          edgeIdx.push_back({i, j});
        }
      }
    }

    // for (int i = 0; i < vertexIdx.size() - 1; ++i) {
    //   for (int j = i + 1; j < vertexIdx.size(); ++j) {
    //     if ((vertices[i] - vertices[j]).magSqr() < 1.f) {
    //       edgeIdx.push_back({i, j});
    //     }
    //   }
    // }

    edges.reset();
    edges.primitive(Mesh::LINES);
    for (auto &e : edgeIdx) {
      edges.vertex(vertices[e.first]);
      edges.vertex(vertices[e.second]);
    }
    edges.update();
  }

  virtual void update() {
    dirty = true;

    // TODO: check if clear can be avoided
    edgeIdx.clear();
    vertices.clear();
    projectedVertices.clear();

    for (auto &v : vertexIdx) {
      Vec<N, float> newVec(0);
      for (int i = 0; i < N; ++i) {
        newVec += (float)v[i] * basis[i];
      }
      vertices.push_back(newVec);
      projectedVertices.push_back(project(newVec));
    }

    computeEdges();
  }

  // Vec<N - 1, float> stereographicProjection(Vec<N, float>& point) {
  //   Vec<N - 1, float> newPoint;
  //   for (int i = 0; i < N - 1; ++i) {
  //     newPoint[i] = point[i] / (1.f - point[N - 1]);
  //   }
  //   return newPoint;
  // }

  inline Vec3f project(Vec<N, float> &point) {
    return Vec3f{point[0], point[1], point[2]};
  }

  Vec3f stereographic(Vec<N, float> &point) {
    if (N < 4) {
      Vec3f newPoint;
      for (int i = 0; i < 2; ++i) {
        newPoint[i] = point[i] / (4.f - point[2]);
      }
      newPoint[2] = 0.f;
      return newPoint;
    }

    Vec4f newPoint4;
    if (N > 5) {
      std::cerr << "ERROR: Higher than 5D" << std::endl;
      return Vec3f{0};
    } else if (N == 5) {
      for (int i = 0; i < 4; ++i) {
        newPoint4[i] = point[i] / (4.f - point[4]);
      }
    } else if (N == 4) {
      newPoint4 = point;
    }

    Vec3f newPoint;
    for (int i = 0; i < 3; ++i) {
      newPoint[i] = newPoint4[i] / (4.f - newPoint4[3]);
    }

    return newPoint;
  }

  virtual void updateBuffer(BufferObject &buffer) {
    if (dirty) {
      buffer.bind();
      buffer.data(projectedVertices.size() * 3 * sizeof(float),
                  projectedVertices.data());
      dirty = false;
    }
  }

  virtual void drawLattice(Graphics &g, VAOMesh &mesh,
                           const float &sphereSize) {
    for (auto &v : vertices) {
      g.pushMatrix();
      // TODO: project
      if (viewAxis.size() == 3) {
        g.translate(v[viewAxis[0]], v[viewAxis[1]], v[viewAxis[2]]);
      } else if (viewAxis.size() == 2) {
        g.translate(v[viewAxis[0]], v[viewAxis[1]]);
      } else if (viewAxis.size() == 1) {
        g.translate(v[viewAxis[0]], 0);
      }

      // g.translate(v);
      g.scale(sphereSize);
      g.draw(mesh);
      g.popMatrix();
    }
  }

  virtual void drawEdges(Graphics &g) { g.draw(edges); }

  virtual void drawParticles(Graphics &g, VAOMesh &mesh) {
    for (auto &v : vertices) {
      g.pushMatrix();
      if (viewAxis.size() == 3) {
        g.translate(v[viewAxis[0]], v[viewAxis[1]], v[viewAxis[2]]);
      } else if (viewAxis.size() == 2) {
        g.translate(v[viewAxis[0]], v[viewAxis[1]]);
      } else if (viewAxis.size() == 1) {
        g.translate(v[viewAxis[0]], 0);
      }
      for (int i = 0; i < particleCoords.size(); ++i) {
        g.pushMatrix();
        if (viewAxis.size() == 3) {
          g.translate(particleCoords[i][viewAxis[0]],
                      particleCoords[i][viewAxis[1]],
                      particleCoords[i][viewAxis[2]]);
        } else if (viewAxis.size() == 2) {
          g.translate(particleCoords[i][viewAxis[0]],
                      particleCoords[i][viewAxis[1]]);
        } else if (viewAxis.size() == 1) {
          g.translate(particleCoords[i][viewAxis[0]], 0);
        }
        g.color(particleColors[i]);
        g.scale(particleSizes[i]);
        g.draw(mesh);
        g.popMatrix();
      }
      g.popMatrix();
    }
  }
};

#endif // CRYSTAL_HPP