#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "al/graphics/al_VAOMesh.hpp"
#include "al/math/al_Vec.hpp"
#include "al/ui/al_Pickable.hpp"

using namespace al;

static const float compareThreshold = 1E-4;

struct CrystalNode {
  unsigned int id;
  Vec3f pos;
  unsigned int overlap{0};
  unsigned int environment;
  PickableBB pickable;
  std::vector<std::pair<int, Vec3f>> neighbours;

  Vec3f unitCellCoord;
  bool insideUnitCell;
  bool isInteriorNode;

  CrystalNode(std::string name) : pickable(name) {}

  void addNeighbour(CrystalNode &neighbourNode) {
    Vec3f vecToNeighbour = neighbourNode.pos - pos;
    neighbours.push_back({neighbourNode.id, vecToNeighbour});
  }

  void sortNeighbours() {
    for (int i = 0; i < neighbours.size(); ++i) {
      for (int j = i + 1; j < neighbours.size(); ++j) {
        Vec3f diff = neighbours[i].second - neighbours[j].second;

        if (diff[0] > compareThreshold) {
          neighbours[i].swap(neighbours[j]);
        } else if (abs(diff[0]) <= compareThreshold) {
          if (diff[1] > compareThreshold) {
            neighbours[i].swap(neighbours[j]);
          } else if (abs(diff[1]) <= compareThreshold) {
            if (diff[2] > compareThreshold) {
              neighbours[i].swap(neighbours[j]);
            } else if (abs(diff[2]) <= compareThreshold) {
              std::cerr << "Error: overlapping nodes. check overlap detection"
                        << std::endl;
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

      if (diff.sumAbs() > compareThreshold) {
        return false;
      }
    }

    return true;
  }
};

struct UnitCell {
  std::vector<Vec3f> unitBasis;
  std::vector<CrystalNode *> cornerNodes;
  std::vector<CrystalNode *> unitCellNodes;
  VAOMesh unitCellMesh;

  void clear(bool clearAll = true) {
    if (clearAll) {
      cornerNodes.clear();
    }
    unitBasis.clear();
    unitCellNodes.clear();
    unitCellMesh.reset();
    unitCellMesh.update();
  }

  bool hasPoint(CrystalNode *node) {
    for (std::vector<CrystalNode *>::iterator it = cornerNodes.begin();
         it != cornerNodes.end();) {
      if (*it == node) {
        it = cornerNodes.erase(it);

        clear(false); // maintain corner nodes

        return true;
      }
      it++;
    }
    return false;
  }

  bool hasMesh() { return unitCellMesh.valid(); }

  // returns true when nodes are added to the unit cell
  bool addNode(CrystalNode *node, int &sliceDim) {
    if (cornerNodes.size() > sliceDim) {
      return false;
    }

    cornerNodes.push_back(node);

    if (cornerNodes.size() == sliceDim + 1) {
      buildMesh(sliceDim);
    }

    return true;
  }

  void buildMesh(int &sliceDim) {
    Vec3f &origin = cornerNodes[0]->pos;
    Vec3f endCorner = origin;

    unitBasis.resize(sliceDim);

    for (int i = 0; i < sliceDim; ++i) {
      unitBasis[i] = cornerNodes[i + 1]->pos - origin;
      endCorner += unitBasis[i];
    }

    unitCellMesh.primitive(Mesh::LINES);

    for (int i = 0; i < sliceDim; ++i) {
      Vec3f &newPoint = cornerNodes[i + 1]->pos;
      unitCellMesh.vertex(origin);
      unitCellMesh.vertex(newPoint);

      if (sliceDim == 2) {
        unitCellMesh.vertex(newPoint);
        unitCellMesh.vertex(endCorner);
      } else if (sliceDim == 3) {
        for (int j = 0; j < sliceDim; ++j) {
          if (j != i) {
            unitCellMesh.vertex(newPoint);
            unitCellMesh.vertex(newPoint + unitBasis[j]);

            unitCellMesh.vertex(newPoint + unitBasis[j]);
            unitCellMesh.vertex(endCorner);
          }
        }
      }
    }

    unitCellMesh.update();
  }
};

#endif // NODE_HPP