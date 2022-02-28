#include "cube.hpp"

/*
 *                y
 *                | 
 *               E|________F
 *              / |      / |
 *             /  | O   /  |
 *           H/___|_*__/G  |
 *            |   |___ |___|______x
 *            |  / A   |  /B
 *            | /      | /
 *            |/_______|/
 *            /D        C
 *           /
 *          z
 * 
 * A:(-1,-1,-1) B:(1,-1,-1) C:(1,-1,1) D:(-1,-1,1)
 * E:(-1,1,-1)  F:(1,1,-1)  G:(1,1,1)  H:(-1,1,1)
 */

/**
 *@brief 立方体的顶点位置，每个面由两个三角形组成
 */
//static constexpr float CubePositions[6][4][3] = {
//    {{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}}, //bottom (A B C D)
//    {{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}}, //front (D C G H)
//    {{1,-1,1},{1,-1,-1},{1,1,-1},{1,1,1}}, //right (C B F G)
//    {{1,-1,-1},{-1,-1,-1},{-1,1,-1},{1,1,-1}}, //back (B A E F)
//    {{-1,-1,-1},{-1,-1,1},{-1,1,1},{-1,1,-1}}, //left (A D H E)
//    {{-1,1,-1},{-1,1,1},{1,1,1},{1,1,-1}}  // top (E H G F)
//};
static constexpr float3 CubeOffsets[6][4] = {
        {{-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,-0.5f,0.5f},{-0.5f,-0.5f,0.5f}}, //bottom (A B C D)
        {{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}}, //front (D C G H)
        {{0.5f,-0.5f,0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{0.5f,0.5f,0.5f}}, //right (C B F G)
        {{0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f}}, //back (B A E F)
        {{-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,0.5f},{-0.5f,0.5f,0.5f},{-0.5f,0.5f,-0.5f}}, //left (A D H E)
        {{-0.5f,0.5f,-0.5f},{-0.5f,0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,0.5f,-0.5f}}  // top (E H G F)
};
static constexpr float3 CubeNormals[6]={
        {0.f,-1.f,0.f},
        {0.f,0.f,1.f},
        {1.f,0.f,0.f},
        {0.f,0.f,-1.f},
        {-1.f,0.f,0.f},
        {0.f,1.f,0.f}
};

void MakeCube(){

}

std::vector<Triangle> MakeCube(const Chunk::Index &chunk_idx, const Chunk::Block &block_idx, int *expose) {
    std::vector<Triangle> triangles;
    float chunk_origin_x = chunk_idx.x * Chunk::ChunkBlockSizeX;
    float chunk_origin_z = chunk_idx.y * Chunk::ChunkBlockSizeZ;
    //传进来的Block确保不会是边界块
    float block_pos_x = chunk_origin_x + static_cast<float>(block_idx.x) - Chunk::ChunkPadding+0.5f;
    float block_pos_z = chunk_origin_z + static_cast<float>(block_idx.z) - Chunk::ChunkPadding+0.5f;
    float block_pos_y = static_cast<float>(block_idx.y) + 0.5f;//y has no padding
    float3 block_center_pos = {block_pos_x,block_pos_y,block_pos_z};
    for(int i =0;i<6;i++){
        if(expose[i]==1){
            triangles.emplace_back(Triangle{});
            auto& tri1 = triangles.back();
            tri1.vertices[0].pos = CubeOffsets[i][0] + block_center_pos;
            tri1.vertices[1].pos = CubeOffsets[i][1] + block_center_pos;
            tri1.vertices[2].pos = CubeOffsets[i][2] + block_center_pos;
            tri1.vertices[0].normal = CubeNormals[i];
            tri1.vertices[1].normal = CubeNormals[i];
            tri1.vertices[2].normal = CubeNormals[i];
            triangles.emplace_back(Triangle{});
            auto& tri2 = triangles.back();
            tri2.vertices[0].pos = CubeOffsets[i][2] + block_center_pos;
            tri2.vertices[1].pos = CubeOffsets[i][3] + block_center_pos;
            tri2.vertices[2].pos = CubeOffsets[i][0] + block_center_pos;
            tri2.vertices[0].normal = CubeNormals[i];
            tri2.vertices[1].normal = CubeNormals[i];
            tri2.vertices[2].normal = CubeNormals[i];
        }
    }
    return triangles;
}
