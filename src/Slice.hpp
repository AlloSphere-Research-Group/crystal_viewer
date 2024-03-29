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
#include "Node.hpp"

using namespace al;

struct AbstractSlice {
  virtual void update() = 0;
  virtual bool pollUpdate() = 0;

  virtual void updateNodes() = 0;
  virtual bool updatePickables(std::array<std::string, 4> &nodeInfo,
                               bool modifyUnitCell) = 0;
  virtual void updateUnitCellInfo(std::array<std::string, 5> &unitCellInfo,
                                  Vec4i &cornerNodes) = 0;
  virtual void updateNodeInfo(std::array<std::string, 4> &nodeInfo,
                              CrystalNode *node = nullptr) = 0;
  virtual void setMiller(Vec5f &value, unsigned int millerNum) = 0;
  virtual void roundMiller() = 0;
  virtual void resetMiller() = 0;
  virtual Vec5f getMiller(unsigned int millerNum) = 0;
  virtual void setNormal(Vec5f &value, unsigned int normalNum) = 0;
  virtual Vec5f getNormal(unsigned int normalNum) = 0;
  virtual void setSliceBasis(Vec5f &value, unsigned int sliceBasisNum) = 0;
  virtual Vec5f getSliceBasis(unsigned int sliceBasisNum) = 0;

  virtual void setDepth(float newDepth) = 0;
  virtual void setThreshold(float newThreshold) = 0;

  virtual int getVertexNum() = 0;
  virtual int getEdgeNum() = 0;

  virtual void uploadVertices(BufferObject &vertexbuffer,
                              BufferObject &colorBuffer) = 0;
  virtual void uploadEdges(BufferObject &startBuffer,
                           BufferObject &endBuffer) = 0;

  virtual void drawPickables(Graphics &g) = 0;

  virtual void loadUnitCell(int cornerNode0, int cornerNode1, int cornerNode2,
                            int cornerNode3) = 0;
  virtual void resetUnitCell() = 0;

  virtual void exportToTxt(std::string &filePath) = 0;
  virtual void exportToJson(std::string &filePath) = 0;

  int latticeDim;
  int sliceDim;

  float sliceDepth{1.0f};
  float edgeThreshold{1.1f};

  PickableManager pickableManager;
  VAOMesh box;

  bool needsUpdate{true};
  std::atomic<bool> dirty{false};
  std::atomic<bool> valid{false};

  bool shouldUploadVertices{false};
  bool shouldUploadEdges{false};
};

template <int N, int M> struct Slice : AbstractSlice {
  Lattice<N> *lattice;

  std::vector<CrystalNode> nodes;

  std::array<Vec<N, float>, N - M> millerIndices;
  std::array<Vec<N, float>, N - M> normals;
  std::array<bool, N - M> isManualNormal;
  std::array<Vec<N, float>, M> sliceBasis;
  std::array<bool, M> isManualSliceBasis;

  std::vector<Vec3f> projectedVertices;

  std::vector<CrystalNode *> environments;
  std::vector<Color> colors;
  std::vector<Vec3f> edgeStarts;
  std::vector<Vec3f> edgeEnds;

  UnitCell unitCell;

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
        if (i < oldLatticeDim - oldSliceDim) {
          millerIndices[i] = oldSlice->getMiller(i);
        } else {
          millerIndices[i] = 0.f;
          millerIndices[i][i] = 1.f;
        }
      }
    }

    addWireBox(box, 0.2f);
    box.update();

    update();
  }

  virtual bool pollUpdate() {
    if (needsUpdate) {
      update();
      needsUpdate = false;
      return true;
    }
    return false;
  }

  virtual void update() {
    dirty = true;

    computeNormals();

    unitCell.clear();

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
          if (diff.sumAbs() < compareThreshold) {
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

  // returns true if unit cell has been modified
  virtual bool updatePickables(std::array<std::string, 4> &nodeInfo,
                               bool modifyUnitCell) {
    for (auto &node : nodes) {
      auto &pickable = node.pickable;

      if (pickable.selected.get() && pickable.hover.get()) {
        if (!modifyUnitCell) {
          updateNodeInfo(nodeInfo, &node);
          return false;
        }

        if (unitCell.hasPoint(&node)) {
          pickable.selected = false;
          updateUnitCell();
          return true;
        } else if (unitCell.addNode(&node, sliceDim)) {
          updateUnitCell();
          return true;
        }

        return false;
      }
    }
    return false;
  }

  void updateUnitCell() {
    // update node metadata based on completed unit cell
    if (unitCell.hasMesh()) {
      Mat3f unitCellMatInv;
      unitCellMatInv.setIdentity();
      for (int i = 0; i < unitCell.unitBasis.size(); ++i) {
        Vec3f &basis = unitCell.unitBasis[i];
        unitCellMatInv.setCol3(basis[0], basis[1], basis[2], i);
      }

      if (!invert(unitCellMatInv)) {
        std::cout << "Unable to create unit cell: Co-planar vectors"
                  << std::endl;
        return;
      }

      // check if nodes are inside unitCell
      for (int i = 0; i < nodes.size(); ++i) {
        Vec3f pos = nodes[i].pos - unitCell.cornerNodes[0]->pos;
        Vec3f unitCellCoord = unitCellMatInv * pos;
        nodes[i].unitCellCoord = unitCellCoord;

        if (unitCellCoord.min() > -compareThreshold &&
            unitCellCoord.max() < (1.f + compareThreshold)) {
          nodes[i].insideUnitCell = true;
          nodes[i].isInteriorNode = true;
          // check if node is on faces
          // TODO: add in origin later to count as interior?
          for (int j = 0; j < unitCellCoord.size(); ++j) {
            if (unitCellCoord[j] < compareThreshold &&
                unitCellCoord[j] > (1.f - compareThreshold)) {
              nodes[i].isInteriorNode = false;
            }
          }
          unitCell.unitCellNodes.push_back(&nodes[i]);
          // TODO: change color later
          colors[i].a = 1.f;
        } else {
          nodes[i].insideUnitCell = false;
          nodes[i].isInteriorNode = false;
          colors[i].a = 0.1f;
        }
      }
    } else {
      for (auto &c : colors) {
        c.a = 1.f;
      }
    }
    shouldUploadVertices = true;
  }

  virtual void updateUnitCellInfo(std::array<std::string, 5> &unitCellInfo,
                                  Vec4i &cornerNodes) {
    for (auto &info : unitCellInfo) {
      info = "";
    }
    for (int i = 0; i < unitCell.unitBasis.size(); ++i) {
      unitCellInfo[i] = "Vec " + std::to_string(i) + ": {" +
                        std::to_string(unitCell.unitBasis[i][0]) + ", " +
                        std::to_string(unitCell.unitBasis[i][1]) + ", " +
                        std::to_string(unitCell.unitBasis[i][2]) + "}, Mag: " +
                        std::to_string(unitCell.unitBasis[i].mag());
    }

    cornerNodes.set(-1);
    for (int i = 0; i < unitCell.cornerNodes.size(); ++i) {
      cornerNodes[i] = unitCell.cornerNodes[i]->id;
    }
  }

  virtual void updateNodeInfo(std::array<std::string, 4> &nodeInfo,
                              CrystalNode *node = nullptr) {
    nodeInfo[0] = "Node: ";
    nodeInfo[1] = " overlap: ";
    nodeInfo[2] = " env: ";
    nodeInfo[3] = " neighbours: ";
    if (node) {
      nodeInfo[0] += std::to_string(node->id);
      nodeInfo[1] += std::to_string(node->overlap);
      nodeInfo[2] += std::to_string(node->environment);
      nodeInfo[3] += std::to_string(node->neighbours.size());
    }
  }

  virtual void drawPickables(Graphics &g) {
    for (auto &node : nodes) {
      auto &pickable = node.pickable;
      g.color(1, 1, 1);
      pickable.drawBB(g);
    }

    g.color(1, 1, 0);
    for (auto *cornerNode : unitCell.cornerNodes) {
      g.pushMatrix();
      g.translate(cornerNode->pos);
      g.draw(box);
      g.popMatrix();
    }

    g.draw(unitCell.unitCellMesh);
  }

  virtual void setMiller(Vec5f &value, unsigned int millerNum) {
    if (millerNum >= millerIndices.size()) {
      std::cerr << "Error: Miller write index out of bounds(" << millerNum
                << ")" << std::endl;
      return;
    }

    millerIndices[millerNum] = value;

    needsUpdate = true;
  }

  virtual void roundMiller() {
    for (auto &miller : millerIndices) {
      for (auto &v : miller) {
        v = std::round(v);
      }
    }
    needsUpdate = true;
  }

  virtual void resetMiller() {
    for (int i = 0; i < millerIndices.size(); ++i) {
      millerIndices[i] = 0.f;
      millerIndices[i][i] = 1.f;
    }
    needsUpdate = true;
  }

  virtual Vec5f getMiller(unsigned int millerNum) {
    if (millerNum >= millerIndices.size()) {
      std::cerr << "Error: Miller index read out of bounds" << std::endl;
      return Vec5f();
    }
    return Vec5f(millerIndices[millerNum]);
  }

  virtual void setNormal(Vec5f &value, unsigned int normalNum) {
    if (normalNum >= normals.size()) {
      std::cerr << "Error: Normal write out of bounds(" << normalNum << ")"
                << std::endl;
      return;
    }

    normals[normalNum] = value;

    // TODO: add in additional flags to control update
    isManualNormal[normalNum] = true;
    // needsUpdate = true;
  }

  virtual Vec5f getNormal(unsigned int normalNum) {
    if (normalNum >= normals.size()) {
      std::cerr << "Error: Normal read out of bounds" << std::endl;
      return Vec5f();
    }
    return Vec5f(normals[normalNum]);
  }

  virtual void setSliceBasis(Vec5f &value, unsigned int sliceBasisNum) {
    if (sliceBasisNum >= sliceBasis.size()) {
      std::cerr << "Error: Slice Basis write out of bounds(" << sliceBasisNum
                << ")" << std::endl;
      return;
    }

    sliceBasis[sliceBasisNum] = value;

    // TODO: add in additional flags to control update
    isManualSliceBasis[sliceBasisNum] = true;
    // needsUpdate = true;
  }

  virtual Vec5f getSliceBasis(unsigned int sliceBasisNum) {
    if (sliceBasisNum >= sliceBasis.size()) {
      std::cerr << "Error: Slice Basis read out of bounds" << std::endl;
      return Vec5f();
    }
    return Vec5f(sliceBasis[sliceBasisNum]);
  }

  virtual void setDepth(float newDepth) {
    sliceDepth = newDepth;
    needsUpdate = true;
  }

  virtual void setThreshold(float newThreshold) {
    edgeThreshold = newThreshold;
    needsUpdate = true;
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

      if (newBasis.sumAbs() < compareThreshold) {
        // std::cout << "Unable to create orthogonal basis, using remaining
        // basis"
        //           << std::endl;
        // TODO: add in ability to adjust basis
        for (int remainingIdx = M; remainingIdx < N; ++remainingIdx) {
          newBasis = lattice->basis[remainingIdx];
          // if (remainingIdx == M) {
          //   newBasis =
          //       Vec4f(1.f, -0.5f * std::sqrt(2.f), 0.f, 0.5f *
          //       std::sqrt(2.f));
          // }
          newBasis.normalize();

          for (auto &n : normals) {
            newBasis = newBasis - newBasis.dot(n) * n;
          }
          for (int j = 0; j < i; ++j) {
            newBasis = newBasis - newBasis.dot(sliceBasis[j]) * sliceBasis[j];
          }

          if (newBasis.sumAbs() >= compareThreshold) {
            break;
          }
        }

        if (newBasis.sumAbs() < compareThreshold) {
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
          } while (newBasis.sumAbs() < compareThreshold);
        }
      }

      newBasis.normalize();
    }

    for (auto &m : isManualNormal) {
      m = false;
    }

    for (auto &m : isManualSliceBasis) {
      m = false;
    }
  }

  virtual int getVertexNum() { return projectedVertices.size(); }
  virtual int getEdgeNum() { return edgeStarts.size(); }

  virtual void loadUnitCell(int cornerNode0, int cornerNode1, int cornerNode2,
                            int cornerNode3) {
    unitCell.clear();
    if (cornerNode0 >= 0) {
      unitCell.addNode(&nodes[cornerNode0], sliceDim);
      if (cornerNode1 >= 0) {
        unitCell.addNode(&nodes[cornerNode1], sliceDim);
        if (cornerNode2 >= 0) {
          unitCell.addNode(&nodes[cornerNode2], sliceDim);
          if (cornerNode3 >= 0) {
            unitCell.addNode(&nodes[cornerNode3], sliceDim);
          }
        }
      }

      updateUnitCell();
    }
  }

  virtual void resetUnitCell() {
    unitCell.clear();
    // TODO: add in color adjustment
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

    for (auto &basis : unitCell.unitBasis) {
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

    for (auto &basis : unitCell.unitBasis) {
      newJson["unitCell_basis"].push_back(basis);
    }

    for (auto *node : unitCell.unitCellNodes) {
      newJson["unitCell_positions"].push_back(node->pos);
      if (node->isInteriorNode) {
        newJson["unitCell_interior_fract_coords"].push_back(
            node->unitCellCoord);
      }
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