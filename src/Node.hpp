#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "al/math/al_Vec.hpp"
#include "al/ui/al_Pickable.hpp"

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

#endif // NODE_HPP