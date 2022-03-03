#pragma once
#include "geometry.hpp"
#include <vector>
#include "chunk.hpp"
void MakeCube();

std::vector<Triangle> MakeCube(const Chunk::Index&,const Chunk::Block&,int* expose);

std::vector<float3> MakeCubeWireframe(const Chunk::Index &chunk_index,const Chunk::Block& block_index);

