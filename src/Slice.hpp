#ifndef SLICE_HPP
#define SLICE_HPP
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

#include "Lattice.hpp"
#include <al/math/al_Random.hpp>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

struct AbstractSlice {
  int latticeDim;
  int sliceDim;
  bool autoUpdate{true};
  bool enableEdges{false};
  float windowSize{0.f};
  float windowDepth{0.f};

  virtual ~AbstractSlice(){};

  virtual float getMiller(int millerNum, unsigned int index) = 0;
  virtual void setMiller(float value, int millerNum, unsigned int index) = 0;
  virtual void roundMiller() = 0;
  virtual Vec3f getNormal() = 0;
  virtual void setWindow(float newWindowSize, float newWindowDepth) = 0;
  virtual void update() = 0;
  virtual void drawSlice(Graphics &g, VAOMesh &mesh,
                         const float &sphereSize) = 0;
  virtual void drawEdges(Graphics &g) = 0;
  virtual void drawPlane(Graphics &g) = 0;
  virtual void drawBox(Graphics &g) = 0;
  virtual void drawBoxNodes(Graphics &g, VAOMesh &mesh) = 0;
  virtual void exportToTxt(std::string &filePath) = 0;
  virtual void exportToJson(std::string &filePath) = 0;
};

template <int N, int M> struct Slice : AbstractSlice {
  Lattice<N> *lattice;

  std::vector<Vec<N, float>> millerIdx;
  std::vector<Vec<N, float>> normal;
  std::vector<Vec<N, float>> sliceBasis;

  std::vector<Vec<N, float>> planeVertices;
  std::vector<Vec<M, float>> subspaceVertices;
  std::vector<std::pair<unsigned int, unsigned int>> boxVertices;
  VAOMesh slicePlane, sliceBox, planeEdges, boxEdges;

  Slice() {
    latticeDim = N;
    sliceDim = M;
    millerIdx.resize(N - M);
    normal.resize(N);
    sliceBasis.resize(M);

    for (int i = 0; i < N - M; ++i) {
      millerIdx[i] = 0.f;
      millerIdx[i][i] = 1.f;
    }
  }

  Slice(std::shared_ptr<AbstractSlice> oldSlice,
        std::shared_ptr<Lattice<N>> latticePtr) {
    latticeDim = N;
    sliceDim = M;
    millerIdx.resize(N - M);
    normal.resize(N);
    sliceBasis.resize(M);
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

    if (autoUpdate) {
      update();
    }
  }

  virtual void roundMiller() {
    for (auto &miller : millerIdx) {
      for (auto &v : miller) {
        v = std::round(v);
      }
    }
    if (autoUpdate) {
      update();
    }
  }

  virtual Vec3f getNormal() {
    Vec3f newNorm{0};
    for (int i = 0; i < 3; ++i) {
      newNorm[i] = normal[0][i];
    }
    return newNorm;
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

  Vec<N - M, float> distanceToPlane(Vec<N, float> &point) {
    Vec<N - M, float> projVec{0.f};

    // normal vector has been normalized
    for (int i = 0; i < N - M; ++i) {
      projVec[i] = point.dot(normal[i]) / normal[i].mag();
    }

    return projVec;
  }

  Vec<M, float> project(Vec<N, float> &point) {
    Vec<M, float> projVec{0.f};

    for (int i = 0; i < M; ++i) {
      projVec[i] = point.dot(normal[N - M + i]) / normal[N - M + i].mag();
    }

    return projVec;
  }

  void computeNormal() {
    for (int i = 0; i < N - M; ++i) {
      normal[i] = 0;
      for (int j = 0; j < N; ++j) {
        normal[i] += millerIdx[i][j] * lattice->basis[j];
      }
      normal[i].normalize();
    }

    for (int i = N - M; i < N; ++i) {
      normal[i] = lattice->basis[i];
      normal[i].normalize();

      for (int j = 0; j < i; ++j) {
        normal[i] = normal[i] - normal[i].dot(normal[j]);
      }

      if (normal[i].mag() < 1e-20) {
        do {
          for (int j = 0; j < N; ++j) {
            // TODO: is random the right move here?
            normal[i][j] = rnd::uniformS();
          }

          normal[i].normalize();

          for (int j = 0; j < i; ++j) {
            normal[i] = normal[i] - normal[i].dot(normal[j]);
          }
        } while (normal[i].mag() < 1e-20);
      }

      normal[i].normalize();
    }
  }

  virtual void update() {
    computeNormal();

    planeVertices.clear();
    boxVertices.clear();
    subspaceVertices.clear();

    int index = 0;
    for (int i = 0; i < lattice->vertexIdx.size(); ++i) {
      Vec<N - M, float> dist = distanceToPlane(lattice->vertices[i]);
      if (dist.mag() < windowDepth) {
        Vec<N, float> projVertex = lattice->vertices[i];
        for (int j = 0; j < N - M; ++j) {
          projVertex -= dist[j] * normal[j];
        }

        planeVertices.push_back(projVertex);

        subspaceVertices.push_back(project(projVertex));

        boxVertices.push_back({i, index});
        index++;
      }
    }

    planeEdges.reset();
    planeEdges.primitive(Mesh::LINES);
    boxEdges.reset();
    boxEdges.primitive(Mesh::LINES);

    for (auto &e : lattice->edgeIdx) {
      // auto it1 = std::find_if(
      //     boxVertices.begin(), boxVertices.end(),
      //     [&e](const std::pair<unsigned int, unsigned int> &element) {
      //       return element.first == e.first;
      //     });
      // auto it2 = std::find_if(
      //     boxVertices.begin(), boxVertices.end(),
      //     [&e](const std::pair<unsigned int, unsigned int> &element) {
      //       return element.first == e.second;
      //     });

      Vec<N - M, float> dist1 = distanceToPlane((lattice->vertices[e.first]));
      Vec<N - M, float> dist2 = distanceToPlane((lattice->vertices[e.second]));
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

  virtual void drawSlice(Graphics &g, VAOMesh &mesh, const float &sphereSize) {
    for (auto &v : subspaceVertices) {
      // for (auto &v : planeVertices) {
      g.pushMatrix();
      // TODO: consider projection
      // if (lattice->viewAxis.size() == 3) {
      //  g.translate(v[lattice->viewAxis[0]], v[lattice->viewAxis[1]],
      //              v[lattice->viewAxis[2]]);
      //} else if (lattice->viewAxis.size() == 2) {
      //  g.translate(v[lattice->viewAxis[0]], v[lattice->viewAxis[1]]);
      //} else if (lattice->viewAxis.size() == 1) {
      //  g.translate(v[lattice->viewAxis[0]], 0);
      //}
      //
      if (M == 2) {
        g.translate(v[0], v[1]);
      } else if (M == 3) {
        g.translate(v[0], v[1], v[2]);
      }
      // g.translate(v[0], v[1], v[2]);
      g.scale(sphereSize);
      g.draw(mesh);
      g.popMatrix();
    }
  }

  virtual void drawEdges(Graphics &g) { g.draw(planeEdges); }

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
    for (auto &bv : boxVertices) {
      g.pushMatrix();
      // TODO: consider projection
      g.translate(lattice->vertices[bv.first][0],
                  lattice->vertices[bv.first][1],
                  lattice->vertices[bv.first][2]);
      g.scale(0.04f);
      g.draw(mesh);
      g.popMatrix();
    }

    g.draw(boxEdges);
  }

  virtual void exportToTxt(std::string &filePath) {
    filePath += ".txt";
    std::ofstream txtOut(filePath);

    if (!txtOut.good()) {
      std::cerr << "Failed to open file: " << filePath << std::endl;
      return;
    }

    for (auto &v : lattice->basis) {
      for (int i = 0; i < N - 1; ++i) {
        txtOut << std::to_string(v[i]) + " ";
      }
      txtOut << std::to_string(v[N - 1]) << std::endl;
    }

    txtOut << std::endl;

    for (auto &v : millerIdx) {
      for (int i = 0; i < N - 1; ++i) {
        txtOut << std::to_string(v[i]) + " ";
      }
      txtOut << std::to_string(v[N - 1]) << std::endl;
    }

    txtOut << std::endl;

    for (int i = N - M; i < normal.size(); ++i) {
      for (int j = 0; j < N - 1; ++j) {
        txtOut << std::to_string(normal[i][j]) + " ";
      }
      txtOut << std::to_string(normal[i][N - 1]) << std::endl;
    }

    txtOut << std::endl;

    for (auto &v : subspaceVertices) {
      // for (auto &v : planeVertices) {
      for (int i = 0; i < N - 1; ++i) {
        txtOut << std::to_string(v[i]) + " ";
      }
      txtOut << std::to_string(v[N - 1]) << std::endl;
    }

    std::cout << "Exported to txt: " << filePath << std::endl;
  }

  virtual void exportToJson(std::string &filePath) {
    filePath += ".json";
    json newJson;

    for (auto &v : lattice->basis) {
      newJson["lattice_basis"].push_back(v);
    }

    for (auto &v : millerIdx) {
      newJson["miller_index"].push_back(v);
    }

    for (int i = N - M; i < normal.size(); ++i) {
      newJson["projection_basis"].push_back(normal[i]);
    }

    for (auto &v : subspaceVertices) {
      newJson["vertices"].push_back(v);
    }

    std::ofstream jsonOut(filePath);
    if (!jsonOut.good()) {
      std::cerr << "Unable to export to : " << filePath << std::endl;
      return;
    }

    jsonOut << std::setw(2) << newJson;

    std::cout << "Exported to json: " << filePath << std::endl;
  }
};

#endif // SLICE_HPP