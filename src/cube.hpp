#pragma once
#include "geometry.hpp"
#include <vector>
#include "chunk.hpp"
void MakeCube();

std::vector<Triangle> MakeCube(const Chunk::Index&,const Chunk::Block&,bool expose[6],float ao[6][4]);

std::vector<float3> MakeCubeWireframe(const Chunk::Index &chunk_index,const Chunk::Block& block_index);

std::vector<Triangle> MakePlant(const Chunk::Index&,const Chunk::Block&,bool r = false);

extern int3 CubeFaceOffset[6];

const float3* GetCubeVertices();

const int* GetCubeIndices();

