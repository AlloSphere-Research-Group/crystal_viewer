#ifndef CRYSTAL_HPP
#define CRYSTAL_HPP

#include <array>
#include <cmath>
#include <vector>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "al/graphics/al_BufferObject.hpp"
#include "al/math/al_Vec.hpp"
#include "al/types/al_Color.hpp"

using namespace al;

struct AbstractLattice {
  virtual void update() = 0;
  virtual void pollUpdate() = 0;

  virtual void generateLattice(int size = 1) = 0;

  virtual void setBasis(Vec5f &value, unsigned int basisNum) = 0;
  virtual void resetBasis() = 0;
  virtual Vec5f getBasis(unsigned int basisNum) = 0;

  virtual int getVertexNum() = 0;
  virtual int getEdgeNum() = 0;

  virtual void uploadVertices(BufferObject &vertexBuffer,
                              BufferObject &colorBuffer) = 0;
  virtual void uploadEdges(BufferObject &startBuffer,
                           BufferObject &endBuffer) = 0;

  int latticeDim;
  int latticeSize{1};

  bool needsUpdate{true};
  std::atomic<bool> dirty{false};
  std::atomic<bool> valid{false};

  bool shouldUploadVertices{true};
  bool shouldUploadEdges{true};
};

template <int N> struct Lattice : AbstractLattice {
  std::array<Vec<N, float>, N> basis;
  std::vector<Vec<N, float>> additionalPoints;

  std::vector<Vec<N, float>> unitCell;
  std::vector<Vec3f> projectedVertices;
  std::vector<Color> colors;
  std::vector<Vec3f> edgeStarts;
  std::vector<Vec3f> edgeEnds;

  std::vector<Vec<N, float>> vertices;
  std::vector<std::atomic<bool>> busy;
  std::mutex latticeLock;

  Lattice() {
    latticeDim = N;

    for (int i = 0; i < N; ++i) {
      basis[i] = 0.f;
      basis[i][i] = 1.f;
    }

    update();
  }

  Lattice(std::shared_ptr<AbstractLattice> oldLattice) {
    latticeDim = N;

    if (oldLattice == nullptr) {
      for (int i = 0; i < N; ++i) {
        basis[i] = 0.f;
        basis[i][i] = 1.f;
      }
    } else {
      // latticeSize = oldLattice->latticeSize; //reset latticeSize to 1
      for (int i = 0; i < latticeDim; ++i) {
        if (i < oldLattice->latticeDim) {
          basis[i] = oldLattice->getBasis(i);
        } else {
          basis[i] = 0.f;
          basis[i][i] = 1.f;
        }
      }
    }

    update();
  }

  virtual void pollUpdate() {
    if (needsUpdate) {
      update();
      needsUpdate = false;
    }
  }

  virtual void update() {
    unitCell.resize((1 << latticeDim) + additionalPoints.size());
    projectedVertices.resize(unitCell.size());
    colors.resize(unitCell.size());

    for (int i = 0; i < (1 << latticeDim); ++i) {
      Vec<N, float> newVec(0);
      for (int j = 0; j < latticeDim; ++j) {
        int hasBasis = (i % (1 << (j + 1))) / (1 << j);
        if (hasBasis > 0)
          newVec += basis[j];
      }
      unitCell[i] = newVec;
      projectedVertices[i] = project(newVec);
      colors[i] = Color(1.f);
    }

    if (additionalPoints.size() > 0) {
      std::cerr << "Error: Non-cubic cells not supported yet" << std::endl;
    }

    edgeStarts.resize(latticeDim * (1 << (latticeDim - 1)));
    edgeEnds.resize(latticeDim * (1 << (latticeDim - 1)));

    if (additionalPoints.size() > 0) {
      std::cerr << "Error: Non-cubic cells not supported yet" << std::endl;
    }

    int index = 0;
    for (int i = 0; i < (1 << latticeDim); ++i) {
      for (int j = 0; j < latticeDim; ++j) {
        int hasBasis = (i % (1 << (j + 1))) / (1 << j);
        if (hasBasis == 0) {
          int k = 1 << j;
          if (i + k < (1 << latticeDim)) {
            edgeStarts[index] = projectedVertices[i];
            edgeEnds[index] = projectedVertices[i + k];
            index++;
          }
        }
      }
    }

    shouldUploadVertices = true;
    shouldUploadEdges = true;

    generateLattice(latticeSize);
  }

  virtual void generateLattice(int size = 1) {
    latticeSize = size;

    dirty = true;

    // TODO: spread into threads

    generateLatticeFunc(size);

    dirty = false;
  }

  void generateLatticeFunc(int size) {
    valid = false;
    int maxSize = int(std::pow(size + 1, latticeDim));
    vertices.resize(maxSize);

    int halfSizeNegative = std::ceil(-size / 2.f);
    int halfSizePositive = std::ceil(size / 2.f);

    Vec<N, int> newVertex(halfSizeNegative);

    for (int i = 0; i < maxSize; ++i) {
      vertices[i] = newVertex;

      newVertex[0] += 1;
      for (int j = 0; j < latticeDim - 1; ++j) {
        if (newVertex[j] > halfSizePositive) {
          newVertex[j] = halfSizeNegative;
          newVertex[j + 1] += 1;
        }
      }
    }
    valid = true;
  }

  virtual void setBasis(Vec5f &value, unsigned int basisNum) {
    if (basisNum >= basis.size()) {
      std::cerr << "Error: Basis vector write index out of bounds" << std::endl;
      return;
    }
    basis[basisNum] = value;

    needsUpdate = true;
  }

  virtual void resetBasis() {
    for (int i = 0; i < basis.size(); ++i) {
      basis[i] = 0.f;
      basis[i][i] = 1.f;
    }

    needsUpdate = true;
  }

  virtual Vec5f getBasis(unsigned int basisNum) {
    if (basisNum >= basis.size()) {
      std::cerr << "Error: Basis vector read index out of bounds" << std::endl;
      return Vec5f();
    }
    return Vec5f(basis[basisNum]);
  }

  virtual int getVertexNum() { return projectedVertices.size(); }
  virtual int getEdgeNum() { return edgeStarts.size(); }

  // template <int T>
  // Vec<T - 1, float> stereographicProjection(Vec<T, float> &point) {
  //   Vec<T - 1, float> newPoint;
  //   for (int i = 0; i < T - 1; ++i) {
  //     newPoint[i] = point[i] / (2.f - point[T - 1]);
  //   }
  //   return newPoint;
  // }

  // Vec3f stereographic3D(Vec<N, float> &point) {
  //   if (N < 3 || N > 5) {
  //     std::cerr << "Error: Unsupported latticeDimension " << N << std::endl;
  //     return Vec3f(0);
  //   }

  //  Vec<5, float> point5D = point;
  //  return stereographicProjection<4>(stereographicProjection<5>(point5D));
  //}

  inline Vec3f project(Vec<N, float> &point) {
    return Vec3f{point[0], point[1], point[2]};
    // return stereographic3D(point);
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
};

#endif // CRYSTAL_HPP