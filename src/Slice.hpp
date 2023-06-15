#ifndef SLICE_HPP
#define SLICE_HPP

#include "Lattice.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_PickableManager.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

struct AbstractSlice {
  virtual void update() = 0;
  virtual void updateEdges() = 0;
  virtual void updatePickables() = 0;

  virtual float getMiller(int millerNum, unsigned int index) = 0;
  virtual void setMiller(float value, int millerNum, unsigned int index) = 0;
  virtual void roundMiller() = 0;

  // split to depth and window setting later
  virtual void setWindow(float newWindowSize, float newWindowDepth) = 0;

  virtual int getVertexNum() = 0;
  virtual int getEdgeNum() = 0;

  virtual void uploadVertices(BufferObject &vertexbuffer,
                              BufferObject &colorBuffer) = 0;
  virtual void uploadEdges(BufferObject &startBuffer,
                           BufferObject &endBuffer) = 0;

  virtual void drawPickable(Graphics &g) = 0;
  // virtual void drawPlane(Graphics &g) = 0;
  // virtual void drawBox(Graphics &g) = 0;
  // virtual void drawBoxNodes(Graphics &g, VAOMesh &mesh) = 0;

  virtual void exportToTxt(std::string &filePath) = 0;
  virtual void exportToJson(std::string &filePath) = 0;

  int latticeDim;
  int sliceDim;

  bool dirty{false};
  bool dirtyEdges{false};

  float edgeThreshold{1.1f};
  float windowSize{0.f};
  float windowDepth{0.f};

  PickableManager pickableManager;
  VAOMesh box;
};

template <int N, int M> struct Slice : AbstractSlice {
  struct CrystalNode {
    int id;
    Vec3f pos;
    int overlap;
    PickableBB pickable;

    std::vector<std::pair<int, Vec3f>> neighbours;

    void addNeighbour(CrystalNode neighbourNode) {
      Vec3f dist = neighbourNode.pos - pos;
      neighbours.push_back({neighbourNode.id, dist});
    }
  };

  Lattice<N> *lattice;

  std::vector<CrystalNode> nodes;

  std::vector<Vec<N, float>> millerIdx;
  std::vector<Vec<N, float>> normal;

  std::vector<Vec<N, float>> sliceBasis;

  std::vector<Vec<N, float>> planeVertices;
  std::vector<Vec3f> subspaceVertices;
  std::vector<int> connectivity;
  std::vector<Color> colors;
  std::vector<Vec3f> edgeStarts;
  std::vector<Vec3f> edgeEnds;

  std::vector<Vec3f> unitCell;

  std::vector<std::pair<unsigned int, unsigned int>> boxVertices;
  VAOMesh slicePlane, sliceBox, planeEdges, boxEdges, unitCellMesh;

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
      edgeThreshold = oldSlice->edgeThreshold;
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

    addWireBox(box, 0.1f);
    box.update();

    updateWindow();
    update();
  }

  virtual void update() {
    dirty = true;

    computeNormal();

    planeVertices.clear();
    boxVertices.clear();
    subspaceVertices.clear();
    pickableManager.clear();
    unitCell.clear();
    unitCellMesh.reset();

    nodes.clear();

    int index = 0;

    for (int i = 0; i < lattice->vertices.size(); ++i) {
      Vec<N, float> &vertex = lattice->vertices[i];

      Vec<N - M, float> dist = distanceToPlane(vertex);

      if (dist.mag() < windowDepth) {
        CrystalNode newNode;
        Vec<N, float> projVertex = vertex;

        for (int j = 0; j < N - M; ++j) {
          projVertex -= dist[j] * normal[j];
        }

        bool skip = false;
        for (auto &sv : subspaceVertices) {
          Vec3f projv = project(projVertex) - sv;
          if (projv.sumAbs() < 1E-4) {
            skip = true;
            break;
          }
          // std::cout << projv.sumAbs() << std::endl;
        }

        if (skip) {
          continue;
        }

        // TODO: initialize with name
        //  pickables.emplace_back(std::to_string(index));
        newNode.id = nodes.size();
        newNode.pos = project(projVertex);
        newNode.pickable.set(box);
        newNode.pickable.pose.setPos(newNode.pos);
        nodes.push_back(newNode);

        planeVertices.push_back(projVertex);

        subspaceVertices.emplace_back(project(projVertex));

        index++;
      }
    }

    for (auto &cn : nodes) {
      pickableManager << cn.pickable;
    }

    updateEdges();
  }

  virtual void updateEdges() {
    dirtyEdges = true;
    edgeStarts.clear();
    edgeEnds.clear();

    colors.clear();
    connectivity.resize(subspaceVertices.size());
    for (auto &c : connectivity) {
      c = 0;
    }

    for (int i = 0; i < nodes.size(); ++i) {
      for (int j = 0; j < i - 1; ++j) {
        Vec3f v = nodes[i].pos - nodes[j].pos;
        if (v.sumAbs() < 1E-4) {
          std::cout << nodes[i].pos << " - " << nodes[j].pos << std::endl;
        }
      }
    }

    for (int i = 0; i < nodes.size(); ++i) {
      for (int j = i + 1; j < nodes.size(); ++j) {
        Vec3f dist = nodes[i].pos - nodes[j].pos;
        if (dist.mag() < edgeThreshold) {
          nodes[i].addNeighbour(nodes[j]);
          nodes[j].addNeighbour(nodes[i]);
          edgeStarts.push_back(nodes[i].pos);
          edgeEnds.push_back(nodes[j].pos);
        }
      }
      HSV hsv(nodes[i].neighbours.size() / 8.f);
      hsv.wrapHue();
      colors.emplace_back(hsv);
    }

    // for (int i = 0; i < subspaceVertices.size(); ++i) {
    //   float maxDist = 0;

    //   for (int j = i + 1; j < subspaceVertices.size(); ++j) {
    //     Vec3f dist = subspaceVertices[i] - subspaceVertices[j];
    //     if (dist.mag() < edgeThreshold) {
    //       edgeStarts.push_back(subspaceVertices[i]);
    //       edgeEnds.push_back(subspaceVertices[j]);
    //       maxDist = maxDist < dist.mag() ? dist.mag() : maxDist;
    //       connectivity[i] += 1;
    //       connectivity[j] += 1;
    //     }
    //   }
    //   // HSV hsv(maxDist);
    //   HSV hsv(connectivity[i] / 4.f);
    //   hsv.wrapHue();
    //   colors.emplace_back(hsv);
    // }
  }

  virtual void uploadVertices(BufferObject &vertexBuffer,
                              BufferObject &colorBuffer) {
    if (dirty) {
      vertexBuffer.bind();
      vertexBuffer.data(subspaceVertices.size() * 3 * sizeof(float),
                        subspaceVertices.data());

      colorBuffer.bind();
      colorBuffer.data(colors.size() * 4 * sizeof(float), colors.data());

      dirty = false;
    }
  }

  virtual void uploadEdges(BufferObject &startBuffer, BufferObject &endBuffer) {
    if (dirtyEdges) {
      startBuffer.bind();
      startBuffer.data(edgeStarts.size() * 3 * sizeof(float),
                       edgeStarts.data());

      endBuffer.bind();
      endBuffer.data(edgeEnds.size() * 3 * sizeof(float), edgeEnds.data());

      dirtyEdges = false;
    }
  }

  virtual void updatePickables() {
    for (auto &cn : nodes) {
      auto &pickable = cn.pickable;
      if (pickable.selected.get() && pickable.hover.get()) {
        std::cout << "Node: " << cn.id << std::endl;
        std::cout << " neighbours: " << cn.neighbours.size() << std::endl;
        for (auto &n : cn.neighbours) {
          std::cout << "  " << n.first << ": " << n.second << std::endl;
        }
        bool hasPoint = false;
        for (std::vector<Vec3f>::iterator it = unitCell.begin();
             it != unitCell.end();) {
          if (*it == pickable.pose.get().pos()) {
            hasPoint = true;
            it = unitCell.erase(it);
            pickable.selected = false;
          } else {
            it++;
          }
        }
        if (!hasPoint && unitCell.size() <= M) {
          unitCell.push_back(pickable.pose.get().pos());

          if (unitCell.size() == M + 1) {
            Vec3f origin = unitCell[0];
            std::array<Vec3f, M> unitBasis;
            Vec3f endCorner = origin;
            for (int i = 0; i < M; ++i) {
              unitBasis[i] = unitCell[i + 1] - origin;
              endCorner += unitBasis[i];
            }

            unitCellMesh.reset();
            unitCellMesh.primitive(Mesh::LINES);

            for (int i = 0; i < M; ++i) {
              unitCellMesh.vertex(origin);
              unitCellMesh.vertex(unitCell[i + 1]);
              if (M == 2) {
                unitCellMesh.vertex(unitCell[i + 1]);
                unitCellMesh.vertex(endCorner);
              } else if (M == 3) {
                for (int k = 0; k < M; ++k) {
                  if (k != i) {
                    unitCellMesh.vertex(unitCell[i + 1]);
                    unitCellMesh.vertex(unitCell[i + 1] + unitBasis[k]);

                    unitCellMesh.vertex(unitCell[i + 1] + unitBasis[k]);
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

  virtual void drawPickable(Graphics &g) {
    for (auto &cn : nodes) {
      auto &pickable = cn.pickable;
      g.color(1, 1, 1);
      pickable.drawBB(g);
    }

    g.color(1, 1, 0);
    for (auto &u : unitCell) {
      g.pushMatrix();
      g.translate(u);
      g.draw(box);
      g.popMatrix();
    }

    g.draw(unitCellMesh);
  }

  // virtual void drawPlane(Graphics &g) {
  //   g.pushMatrix();
  //   // TODO: calculate bivector
  //   Quatf rot = Quatf::getRotationTo(
  //       Vec3f(0.f, 0.f, 1.f), Vec3f(normal[0][0], normal[0][1],
  //       normal[0][2]));
  //   g.rotate(rot);
  //   g.draw(slicePlane);
  //   g.popMatrix();
  // }

  // virtual void drawBox(Graphics &g) {
  //   g.pushMatrix();
  //   // TODO: calculate bivector
  //   Quatf rot = Quatf::getRotationTo(
  //       Vec3f(0.f, 0.f, 1.f), Vec3f(normal[0][0], normal[0][1],
  //       normal[0][2]));
  //   g.rotate(rot);
  //   g.draw(sliceBox);
  //   g.popMatrix();
  // }

  // virtual void drawBoxNodes(Graphics &g, VAOMesh &mesh) {
  //   for (auto &bv : boxVertices) {
  //     g.pushMatrix();
  //     // TODO: consider projection
  //     // g.translate(lattice->vertices[bv.first][0],
  //     //            lattice->vertices[bv.first][1],
  //     //            lattice->vertices[bv.first][2]);
  //     g.scale(0.04f);
  //     g.draw(mesh);
  //     g.popMatrix();
  //   }

  //  g.draw(boxEdges);
  //}

  virtual void setMiller(float value, int millerNum, unsigned int index) {
    if (millerNum >= N - M || index >= N) {
      std::cerr << "Error: Miller write index out of bounds" << std::endl;
      return;
    }

    millerIdx[millerNum][index] = value;

    update();
  }

  virtual void roundMiller() {
    for (auto &miller : millerIdx) {
      for (auto &v : miller) {
        v = std::round(v);
      }
    }

    update();
  }

  virtual float getMiller(int millerNum, unsigned int index) {
    return millerIdx[millerNum][index];
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

  virtual int getVertexNum() { return subspaceVertices.size(); }
  virtual int getEdgeNum() { return edgeStarts.size(); }

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