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
  virtual void updatePickables(bool updateNodes) = 0;

  virtual float getMiller(int millerNum, unsigned int index) = 0;
  virtual void setMiller(float value, int millerNum, unsigned int index) = 0;
  virtual void roundMiller() = 0;

  virtual void setDepth(float newDepth) = 0;
  virtual void setThreshold(float newThreshold) = 0;

  virtual int getVertexNum() = 0;
  virtual int getEdgeNum() = 0;

  virtual void uploadVertices(BufferObject &vertexbuffer,
                              BufferObject &colorBuffer) = 0;
  virtual void uploadEdges(BufferObject &startBuffer,
                           BufferObject &endBuffer) = 0;

  virtual void drawPickables(Graphics &g) = 0;

  virtual void resetUnitCell() = 0;

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
  unsigned int overlap{0};
  unsigned int environment;
  PickableBB pickable;
  std::vector<std::pair<int, Vec3f>> neighbours;

  Vec3f unitCellCoord;
  bool insideUnitCell;

  CrystalNode(std::string name) : pickable(name) {}

  void addNeighbour(CrystalNode &neighbourNode) {
    Vec3f vecToNeighbour = neighbourNode.pos - pos;
    neighbours.push_back({neighbourNode.id, vecToNeighbour});
  }

  void sortNeighbours() {
    for (int i = 0; i < neighbours.size(); ++i) {
      for (int j = i + 1; j < neighbours.size(); ++j) {
        Vec3f diff = neighbours[i].second - neighbours[j].second;

        if (diff[0] > 1E-4) {
          neighbours[i].swap(neighbours[j]);
        } else if (abs(diff[0]) <= 1E-4) {
          if (diff[1] > 1E-4) {
            neighbours[i].swap(neighbours[j]);
          } else if (abs(diff[1]) <= 1E-4) {
            // std::cout << "remove, diff_xy same" << std::endl;
            if (diff[2] > 1E-4) {
              neighbours[i].swap(neighbours[j]);
            } else if (abs(diff[2]) <= 1E-4) {
              std::cout << "remove diff same wtf" << std::endl;
            }
          }
        }
      }
    }
  }

  bool compareNeighbours(CrystalNode *newNode) {
    std::vector<std::pair<int, Vec3f>> &newNeighbours = newNode->neighbours;

    if (newNeighbours.size() != neighbours.size()) {
      return false;
    }

    for (int i = 0; i < newNeighbours.size(); ++i) {
      Vec3f diff = neighbours[i].second - newNeighbours[i].second;

      if (diff.sumAbs() > 1E-4) {
        return false;
      }
    }

    return true;
  }
};

template <int N, int M> struct Slice : AbstractSlice {
  Lattice<N> *lattice;

  std::vector<CrystalNode> nodes;

  std::array<Vec<N, float>, N - M> millerIndices;
  std::array<Vec<N, float>, N - M> normals;
  std::array<Vec<N, float>, M> sliceBasis;

  std::vector<Vec3f> projectedVertices;

  std::vector<CrystalNode *> environments;
  std::vector<Color> colors;
  std::vector<Vec3f> edgeStarts;
  std::vector<Vec3f> edgeEnds;

  std::array<Vec3f, M> unitBasis;
  std::vector<CrystalNode *> cornerNodes;
  std::vector<CrystalNode *> unitCellNodes;
  VAOMesh unitCellMesh;
  bool hasUnitCell{false};

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

    // TODO: reset other unitcell related stuff
    cornerNodes.clear();
    unitCellNodes.clear();
    unitCellMesh.reset();
    hasUnitCell = false;

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

        Vec3f projVertex = project(newVertex);

        bool skip = false;
        for (auto &node : nodes) {
          Vec3f diff = projVertex - node.pos;
          if (diff.sumAbs() < 1E-4) {
            node.overlap++;
            skip = true;
            break;
          }
        }

        if (skip) {
          continue;
        }

        CrystalNode newNode(std::to_string(nodes.size()));
        newNode.id = nodes.size();
        newNode.pos = projVertex;
        newNode.pickable.set(box);
        newNode.pickable.pose.setPos(newNode.pos);
        nodes.push_back(std::move(newNode));
      }
    }

    for (auto &node : nodes) {
      projectedVertices.push_back(node.pos);
      pickableManager << node.pickable;
    }

    updateNodes();
  }

  virtual void updateNodes() {
    environments.clear();
    colors.clear();

    edgeStarts.clear();
    edgeEnds.clear();

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

      bool newEnvironment = true;
      for (int i = 0; i < environments.size(); ++i) {
        if (node.compareNeighbours(environments[i])) {
          newEnvironment = false;
          node.environment = i;
          break;
        }
      }

      if (newEnvironment) {
        node.environment = environments.size();
        environments.push_back(&node);
      }
    }

    std::cout << "environment size: " << environments.size() << std::endl;

    for (auto &node : nodes) {
      HSV hsv(float(node.environment) / environments.size());
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

  virtual void updatePickables(bool updateNodes) {
    for (auto &node : nodes) {
      auto &pickable = node.pickable;

      if (pickable.selected.get() && pickable.hover.get()) {
        if (!updateNodes) {
          // TODO: output this to UI
          std::cout << "Node: " << node.id << std::endl;
          std::cout << " overlap: " << node.overlap << std::endl;
          std::cout << " env: " << node.environment << std::endl;
          std::cout << " neighbours: " << node.neighbours.size() << std::endl;
          // for (auto &n : node.neighbours) {
          //   std::cout << "  " << n.first << ": " << n.second << std::endl;
          // }
          return;
        }

        bool hasPoint = false;
        for (std::vector<CrystalNode *>::iterator it = cornerNodes.begin();
             it != cornerNodes.end();) {
          if (*it == &node) {
            hasPoint = true;
            it = cornerNodes.erase(it);
            pickable.selected = false;
            if (hasUnitCell) {
              unitCellNodes.clear();
              unitCellMesh.reset();
              for (auto &c : colors) {
                c.a = 1.f;
              }
              shouldUploadVertices = true;
              hasUnitCell = false;
            }
          } else {
            it++;
          }
        }

        if (!hasPoint && cornerNodes.size() <= M) {
          cornerNodes.push_back(&node);

          // Construct Unit Cell
          if (cornerNodes.size() == M + 1) {
            Vec3f origin = cornerNodes[0]->pos;
            Vec3f endCorner = origin;

            for (int i = 0; i < M; ++i) {
              unitBasis[i] = cornerNodes[i + 1]->pos - origin;
              endCorner += unitBasis[i];
            }

            unitCellMesh.reset();
            unitCellMesh.primitive(Mesh::LINES);

            for (int i = 0; i < M; ++i) {
              Vec3f &newPoint = cornerNodes[i + 1]->pos;
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

            Mat3f unitCellMatInv;
            unitCellMatInv.setIdentity();
            for (int i = 0; i < unitBasis.size(); ++i) {
              unitCellMatInv.setCol3(unitBasis[i][0], unitBasis[i][1],
                                     unitBasis[i][2], i);
            }

            if (!invert(unitCellMatInv)) {
              std::cout << "Unable to create unit cell: Co-planar vectors"
                        << std::endl;
              break;
            }

            // check if inside unitCell
            for (int i = 0; i < nodes.size(); ++i) {
              Vec3f pos = nodes[i].pos - origin;
              Vec3f unitCellCoord = unitCellMatInv * pos;
              nodes[i].unitCellCoord = unitCellCoord;

              if (unitCellCoord.min() > -1E-4 &&
                  unitCellCoord.max() < (1.f + 1E-4)) {
                nodes[i].insideUnitCell = true;
                unitCellNodes.push_back(&nodes[i]);
                colors[i].a = 1.f;
              } else {
                nodes[i].insideUnitCell = false;
                colors[i].a = 0.1f;
              }
            }

            shouldUploadVertices = true;
            hasUnitCell = true;
            std::cout << "unitCell: " << unitCellNodes.size() << std::endl;
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
    for (auto *u : cornerNodes) {
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

  virtual void setThreshold(float newThreshold) {
    edgeThreshold = newThreshold;
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
      for (int j = 0; j < i; ++j) {
        newBasis = newBasis - newBasis.dot(sliceBasis[j]) * sliceBasis[j];
      }

      if (newBasis.sumAbs() < 1E-4) {
        // std::cout << "Unable to create orthogonal basis, using remaining
        // basis"
        //           << std::endl;
        for (int remainingIdx = M; remainingIdx < N; ++remainingIdx) {
          newBasis = lattice->basis[remainingIdx];
          newBasis.normalize();

          for (auto &n : normals) {
            newBasis = newBasis - newBasis.dot(n) * n;
          }
          for (int j = 0; j < i; ++j) {
            newBasis = newBasis - newBasis.dot(sliceBasis[j]) * sliceBasis[j];
          }

          if (newBasis.sumAbs() >= 1E-4) {
            break;
          }
        }

        if (newBasis.sumAbs() < 1E-4) {
          std::cout << "Unable to create orthogonal basis, randomizing"
                    << std::endl;
          do {
            for (int j = 0; j < N; ++j) {
              newBasis[j] = rnd::uniformS();
            }
            newBasis.normalize();

            for (int j = 0; j < i; ++j) {
              newBasis = newBasis - newBasis.dot(sliceBasis[j]) * sliceBasis[j];
            }
          } while (newBasis.sumAbs() < 1E-4);
        }
      }

      newBasis.normalize();
    }
  }

  virtual int getVertexNum() { return projectedVertices.size(); }
  virtual int getEdgeNum() { return edgeStarts.size(); }

  virtual void resetUnitCell() {
    cornerNodes.clear();
    unitCellNodes.clear();
    unitCellMesh.reset();
  }

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

    for (auto &basis : sliceBasis) {
      for (int j = 0; j < N - 1; ++j) {
        txtOut << std::to_string(basis[j]) + " ";
      }
      txtOut << std::to_string(basis[N - 1]) << std::endl;
    }

    txtOut << std::endl;

    for (auto &basis : unitBasis) {
      for (int j = 0; j < M - 1; ++j) {
        txtOut << std::to_string(basis[j]) + " ";
      }
      txtOut << std::to_string(basis[M - 1]) << std::endl;
    }

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

    for (auto &b : lattice->basis) {
      newJson["lattice_basis"].push_back(b);
    }

    for (auto &millerIndex : millerIndices) {
      newJson["miller_index"].push_back(millerIndex);
    }

    for (auto &basis : sliceBasis) {
      newJson["projection_basis"].push_back(basis);
    }

    for (auto &basis : unitBasis) {
      newJson["unitCell_basis"].push_back(basis);
    }

    for (auto *node : unitCellNodes) {
      newJson["vertices"].push_back(node->pos);
    }

    // for (auto &v : projectedVertices) {
    //   newJson["vertices"].push_back(v);
    // }

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