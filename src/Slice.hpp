#ifndef SLICE_HPP
#define SLICE_HPP

#include <array>
#include <cmath>
#include <string>
#include <vector>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "al/graphics/al_BufferObject.hpp"
#include "al/graphics/al_Graphics.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_VAOMesh.hpp"
#include "al/math/al_Random.hpp"
#include "al/math/al_Vec.hpp"
#include "al/types/al_Color.hpp"
#include "al/ui/al_PickableManager.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "Lattice.hpp"

using namespace al;

struct AbstractSlice {
  virtual void update() = 0;

  virtual void updateNodes() = 0;
  virtual void updatePickables() = 0;

  virtual float getMiller(int millerNum, unsigned int index) = 0;
  virtual void setMiller(float value, int millerNum, unsigned int index) = 0;
  virtual void roundMiller() = 0;

  virtual void setDepth(float newDepth) = 0;

  virtual int getVertexNum() = 0;
  virtual int getEdgeNum() = 0;

  virtual void uploadVertices(BufferObject &vertexbuffer,
                              BufferObject &colorBuffer) = 0;
  virtual void uploadEdges(BufferObject &startBuffer,
                           BufferObject &endBuffer) = 0;

  virtual void drawPickables(Graphics &g) = 0;

  virtual void exportToTxt(std::string &filePath) = 0;
  virtual void exportToJson(std::string &filePath) = 0;

  int latticeDim;
  int sliceDim;

  float sliceDepth{0.f};
  float edgeThreshold{1.1f};

  PickableManager pickableManager;
  VAOMesh box;

  std::atomic<bool> dirty{false};
  std::atomic<bool> valid{false};

  bool shouldUploadVertices{false};
  bool shouldUploadEdges{false};
};

struct CrystalNode {
  unsigned int id;
  Vec3f pos;
  int overlap;
  PickableBB pickable;
  std::vector<std::pair<int, Vec3f>> neighbours;

  CrystalNode(std::string name) : pickable(name) {}

  void addNeighbour(CrystalNode neighbourNode) {
    Vec3f vecToNeighbour = neighbourNode.pos - pos;
    neighbours.push_back({neighbourNode.id, vecToNeighbour});
  }

  void sortNeighbours() {
    for (int i = 0; i < neighbours.size(); ++i) {
      for (int j = i + 1; j < neighbours.size(); ++j) {
        Vec3f diff = neighbours[i].second - neighbours[j].second;

        if (diff[0] > 0) {
          neighbours[i].swap(neighbours[j]);
        } else if (diff[0] == 0.f) {
          std::cout << "remove, diff_x same: " << neighbours[i].second
                    << std::endl;
          if (diff[1] > 0) {
            neighbours[i].swap(neighbours[j]);
          } else if (diff[1] == 0.f) {
            std::cout << "remove, diff_xy same" << std::endl;
            if (diff[2] > 0) {
              neighbours[i].swap(neighbours[j]);
            } else if (diff[2] == 0.f) {
              std::cout << "remove diff same wtf" << std::endl;
            }
          }
        }
      }
    }
  }
};

template <int N, int M> struct Slice : AbstractSlice {
  Lattice<N> *lattice;

  std::vector<CrystalNode> nodes;

  std::array<Vec<N, float>, N - M> millerIndices;
  std::array<Vec<N, float>, N - M> normals;
  std::array<Vec<N, float>, M> sliceBasis;

  std::vector<Vec3f> projectedVertices;
  std::vector<Color> colors;
  std::vector<Vec3f> edgeStarts;
  std::vector<Vec3f> edgeEnds;

  std::vector<CrystalNode *> unitCells;
  VAOMesh unitCellMesh;

  Slice() {
    latticeDim = N;
    sliceDim = M;

    for (int i = 0; i < N - M; ++i) {
      millerIndices[i] = 0.f;
      millerIndices[i][i] = 1.f;
    }
  }

  Slice(std::shared_ptr<AbstractSlice> oldSlice,
        std::shared_ptr<Lattice<N>> latticePtr) {
    latticeDim = N;
    sliceDim = M;

    lattice = latticePtr.get();

    if (oldSlice == nullptr) {
      for (int i = 0; i < N - M; ++i) {
        millerIndices[i] = 0.f;
        millerIndices[i][i] = 1.f;
      }
    } else {
      sliceDepth = oldSlice->sliceDepth;
      edgeThreshold = oldSlice->edgeThreshold;

      int oldLatticeDim = oldSlice->latticeDim;
      int oldSliceDim = oldSlice->sliceDim;

      for (int i = 0; i < N - M; ++i) {
        millerIndices[i] = 0.f;

        if (i < oldLatticeDim - oldSliceDim) {
          for (int j = 0; j < N && j < oldLatticeDim; ++j) {
            millerIndices[i][j] = oldSlice->getMiller(i, j);
          }
        } else {
          millerIndices[i][i] = 1.f;
        }
      }
    }

    addWireBox(box, 0.2f);
    box.update();

    update();
  }

  virtual void update() {
    dirty = true;

    computeNormals();

    for (auto &b : sliceBasis) {
      std::cout << "basis: " << b << std::endl;
    }

    // TODO reset other unitcell related stuff
    unitCells.clear();
    unitCellMesh.reset();

    nodes.clear();

    projectedVertices.clear();
    pickableManager.clear();

    Vec<N - M, float> dist;

    for (auto &vertex : lattice->vertices) {
      // distance to hyperplane
      for (int i = 0; i < N - M; ++i) {
        dist[i] = vertex.dot(normals[i]);
      }

      if (dist.mag() < sliceDepth) {
        Vec<N, float> newVertex = vertex;

        for (int i = 0; i < N - M; ++i) {
          newVertex -= dist[i] * normals[i];
        }

        std::cout << " newVtx: " << newVertex << std::endl;

        Vec3f projVertex = project(newVertex);

        std::cout << " projVtx: " << projVertex << std::endl;

        bool skip = false;
        for (auto &node : nodes) {
          Vec3f diff = projVertex - node.pos;
          std::cout << "diff: " << diff << std::endl;
          if (diff.sumAbs() < 1E-4) {
            std::cout << "overlapped" << std::endl;
            node.overlap++;
            skip = true;
            break;
          }
        }

        if (skip) {
          std::cout << "skipped" << std::endl;
          continue;
        }

        CrystalNode newNode(std::to_string(nodes.size()));
        newNode.id = nodes.size();
        newNode.pos = projVertex;
        // std::cout << "projVertex: " << projVertex << std::endl;
        // std::cout << " node.pos: " << newNode.pos << std::endl;
        newNode.pickable.set(box);
        newNode.pickable.pose.setPos(newNode.pos);
        nodes.push_back(newNode);
      }
    }

    for (auto &node : nodes) {
      projectedVertices.push_back(node.pos);
      pickableManager << node.pickable;
    }

    updateNodes();
  }

  virtual void updateNodes() {
    colors.clear();

    edgeStarts.clear();
    edgeEnds.clear();

    // TODO: remove this check
    for (int i = 0; i < nodes.size(); ++i) {
      for (int j = 0; j < i; ++j) {
        Vec3f v = nodes[i].pos - nodes[j].pos;
        if (v.sumAbs() < 1E-4) {
          std::cout << "**********This should've been checked before\n"
                    << nodes[i].pos << " - " << nodes[j].pos << std::endl;
        }
      }
    }

    for (int i = 0; i < nodes.size(); ++i) {
      for (int j = i + 1; j < nodes.size(); ++j) {
        Vec3f diff = nodes[i].pos - nodes[j].pos;
        if (diff.mag() < edgeThreshold) {
          nodes[i].addNeighbour(nodes[j]);
          nodes[j].addNeighbour(nodes[i]);

          edgeStarts.push_back(nodes[i].pos);
          edgeEnds.push_back(nodes[j].pos);
        }
      }
    }

    for (auto &node : nodes) {
      node.sortNeighbours();

      // TODO: add environment check
      HSV hsv(node.neighbours.size() / 8.f);
      hsv.wrapHue();
      colors.emplace_back(hsv);
    }

    shouldUploadVertices = true;
    shouldUploadEdges = true;
  }

  virtual void uploadVertices(BufferObject &vertexBuffer,
                              BufferObject &colorBuffer) {
    if (shouldUploadVertices) {
      vertexBuffer.bind();
      vertexBuffer.data(projectedVertices.size() * 3 * sizeof(float),
                        projectedVertices.data());

      colorBuffer.bind();
      colorBuffer.data(colors.size() * 4 * sizeof(float), colors.data());
      shouldUploadVertices = false;
    }
  }

  virtual void uploadEdges(BufferObject &startBuffer, BufferObject &endBuffer) {
    if (shouldUploadEdges) {
      startBuffer.bind();
      startBuffer.data(edgeStarts.size() * 3 * sizeof(float),
                       edgeStarts.data());

      endBuffer.bind();
      endBuffer.data(edgeEnds.size() * 3 * sizeof(float), edgeEnds.data());

      shouldUploadEdges = false;
    }
  }

  virtual void updatePickables() {
    for (auto &node : nodes) {
      auto &pickable = node.pickable;

      if (pickable.selected.get() && pickable.hover.get()) {
        // TODO: output this to UI
        std::cout << "Node: " << node.id << std::endl;
        std::cout << " neighbours: " << node.neighbours.size() << std::endl;
        for (auto &n : node.neighbours) {
          std::cout << "  " << n.first << ": " << n.second << std::endl;
        }

        bool hasPoint = false;
        for (std::vector<CrystalNode *>::iterator it = unitCells.begin();
             it != unitCells.end();) {
          if (*it == &node) {
            hasPoint = true;
            it = unitCells.erase(it);
            pickable.selected = false;
          } else {
            it++;
          }
        }

        if (!hasPoint && unitCells.size() <= M) {
          unitCells.push_back(&node);

          if (unitCells.size() == M + 1) {
            Vec3f origin = unitCells[0]->pos;
            std::array<Vec3f, M> unitBasis;
            Vec3f endCorner = origin;

            for (int i = 0; i < M; ++i) {
              unitBasis[i] = unitCells[i + 1]->pos - origin;
              endCorner += unitBasis[i];
            }

            unitCellMesh.reset();
            unitCellMesh.primitive(Mesh::LINES);

            for (int i = 0; i < M; ++i) {
              Vec3f &newPoint = unitCells[i + 1]->pos;
              unitCellMesh.vertex(origin);
              unitCellMesh.vertex(newPoint);

              if (M == 2) {
                unitCellMesh.vertex(newPoint);
                unitCellMesh.vertex(endCorner);
              } else if (M == 3) {
                for (int k = 0; k < M; ++k) {
                  if (k != i) {
                    unitCellMesh.vertex(newPoint);
                    unitCellMesh.vertex(newPoint + unitBasis[k]);

                    unitCellMesh.vertex(newPoint + unitBasis[k]);
                    unitCellMesh.vertex(endCorner);
                  }
                }
              }
            }

            unitCellMesh.update();
          }
        }

        break;
      }
    }
  }

  virtual void drawPickables(Graphics &g) {
    for (auto &node : nodes) {
      auto &pickable = node.pickable;
      g.color(1, 1, 1);
      pickable.drawBB(g);
    }

    g.color(1, 1, 0);
    for (auto *u : unitCells) {
      g.pushMatrix();
      g.translate(u->pos);
      g.draw(box);
      g.popMatrix();
    }

    g.draw(unitCellMesh);
  }

  virtual void setMiller(float value, int millerNum, unsigned int index) {
    if (millerNum >= N - M || index >= N) {
      std::cerr << "Error: Miller write index out of bounds" << std::endl;
      return;
    }

    millerIndices[millerNum][index] = value;

    update();
  }

  virtual void roundMiller() {
    for (auto &miller : millerIndices) {
      for (auto &v : miller) {
        v = std::round(v);
      }
    }

    update();
  }

  virtual float getMiller(int millerNum, unsigned int index) {
    return millerIndices[millerNum][index];
  }

  virtual void setDepth(float newDepth) {
    sliceDepth = newDepth;

    update();
  }

  Vec<M, float> project(Vec<N, float> &point) {
    Vec<M, float> projVec{0.f};

    for (int i = 0; i < M; ++i) {
      // sliceBasis is normalized
      projVec[i] = point.dot(sliceBasis[i]); //  / sliceBasis[i].mag()
    }

    return projVec;
  }

  void computeNormals() {
    for (int i = 0; i < N - M; ++i) {
      normals[i] = 0;
      for (int j = 0; j < N; ++j) {
        normals[i] += millerIndices[i][j] * lattice->basis[j];
      }
      normals[i].normalize();
    }

    for (int i = 0; i < M; ++i) {
      Vec<N, float> &newBasis = sliceBasis[i];
      newBasis = lattice->basis[i];
      newBasis.normalize();

      for (auto &n : normals) {
        newBasis = newBasis - newBasis.dot(n) * n;
      }
      std::cout << "newbasis_afternormal: " << newBasis << std::endl;
      for (int j = 0; j < i; ++j) {
        newBasis = newBasis - newBasis.dot(sliceBasis[j]) * sliceBasis[j];
      }

      std::cout << "newbasis: " << newBasis << std::endl;

      if (newBasis.mag() < 1e-20) {
        std::cout << "Unable to create orthogonal basis, randomizing"
                  << std::endl;

        do {
          for (int j = 0; j < N; ++j) {
            newBasis[j] = rnd::uniformS();
          }
          newBasis.normalize();

          for (int j = 0; j < i; ++j) {
            newBasis = newBasis - newBasis.dot(sliceBasis[j]);
          }
        } while (newBasis.mag() < 1e-20);

        newBasis.normalize();
      }
    }
  }

  virtual int getVertexNum() { return projectedVertices.size(); }
  virtual int getEdgeNum() { return edgeStarts.size(); }

  // TODO: confine this to unit cell
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

    for (auto &v : millerIndices) {
      for (int i = 0; i < N - 1; ++i) {
        txtOut << std::to_string(v[i]) + " ";
      }
      txtOut << std::to_string(v[N - 1]) << std::endl;
    }

    txtOut << std::endl;

    for (int i = 0; i < sliceBasis.size(); ++i) {
      for (int j = 0; j < N - 1; ++j) {
        txtOut << std::to_string(sliceBasis[i][j]) + " ";
      }
      txtOut << std::to_string(sliceBasis[i][N - 1]) << std::endl;
    }

    txtOut << std::endl;

    for (auto &v : projectedVertices) {
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

    for (auto &v : millerIndices) {
      newJson["miller_index"].push_back(v);
    }

    for (auto &v : sliceBasis) {
      newJson["projection_basis"].push_back(v);
    }

    for (auto &v : projectedVertices) {
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