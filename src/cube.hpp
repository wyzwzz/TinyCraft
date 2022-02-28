#pragma once
#include "geometry.hpp"
#include <vector>
#include "chunk.hpp"
void MakeCube();

std::vector<Triangle> MakeCube(const Chunk::Index&,const Chunk::Block&,int* expose);




