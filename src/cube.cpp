#include "cube.hpp"
#include "item.hpp"
extern "C"{
#include <noise.h>
}
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
int3 CubeFaceOffset[6] = {
        {0,-1,0},
        {0,0,1},
        {1,0,0},
        {0,0,-1},
        {-1,0,0},
        {0,1,0}
};
static constexpr const float3 CubeVertices[8] = {
        {-0.5f,-0.5f,-0.5f},//A
        {0.5f,-0.5f,-0.5f},//B
        {0.5f,-0.5f,0.5f},//C
        {-0.5f,-0.5f,0.5f},//D
        {-0.5f,0.5f,-0.5f},//E
        {0.5f,0.5f,-0.5f},//F
        {0.5f,0.5f,0.5f},//G
        {-0.5f,0.5f,0.5f}//H
};
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
static constexpr float2 UVs[6][4] = {
        {{0,0},{1,0},{1,1},{0,1}},
        {{0,0},{1,0},{1,1},{0,1}},
        {{0,0},{1,0},{1,1},{0,1}},
        {{0,0},{1,0},{1,1},{0,1}},
        {{0,0},{1,0},{1,1},{0,1}},
        {{0,0},{1,0},{1,1},{0,1}}
};
void MakeCube(){

}

std::vector<Triangle> MakeCube(const Chunk::Index &chunk_idx, const Chunk::Block &block_idx, int *expose) {
    std::vector<Triangle> triangles;
    float chunk_origin_x = chunk_idx.p * Chunk::ChunkBlockSizeX;
    float chunk_origin_z = chunk_idx.q * Chunk::ChunkBlockSizeZ;
    //传进来的Block确保不会是边界块
    float block_pos_x = chunk_origin_x + static_cast<float>(block_idx.x) - Chunk::ChunkPadding+0.5f;
    float block_pos_z = chunk_origin_z + static_cast<float>(block_idx.z) - Chunk::ChunkPadding+0.5f;
    float block_pos_y = static_cast<float>(block_idx.y) + 0.5f;//y has no padding
    float3 block_center_pos = {block_pos_x,block_pos_y,block_pos_z};
    int w = block_idx.w;
    //texture uv
    static float s = 0.0625f;
    static float a = 0.f + 1.f / 2048.f;
    static float b = s - 1.f / 2048.f;

    for(int i =0;i<6;i++){
        if(expose[i]==1){
            float du = static_cast<float>(blocks[w][i] % 16) * s;
            float dv = static_cast<float>(blocks[w][i] / 16) * s;
            triangles.emplace_back(Triangle{});
            auto& tri1 = triangles.back();
            tri1.vertices[0].pos = CubeOffsets[i][0] + block_center_pos;
            tri1.vertices[1].pos = CubeOffsets[i][1] + block_center_pos;
            tri1.vertices[2].pos = CubeOffsets[i][2] + block_center_pos;
            tri1.vertices[0].normal = CubeNormals[i];
            tri1.vertices[1].normal = CubeNormals[i];
            tri1.vertices[2].normal = CubeNormals[i];
            tri1.vertices[0].uv = float2{du + (UVs[i][0].x ? b : a),
                                         dv + (UVs[i][0].y ? b : a)};
            tri1.vertices[1].uv = float2{du + (UVs[i][1].x ? b : a),
                                         dv + (UVs[i][1].y ? b : a)};
            tri1.vertices[2].uv = float2{du + (UVs[i][2].x ? b : a),
                                         dv + (UVs[i][2].y ? b : a)};

            triangles.emplace_back(Triangle{});
            auto& tri2 = triangles.back();
            tri2.vertices[0].pos = CubeOffsets[i][2] + block_center_pos;
            tri2.vertices[1].pos = CubeOffsets[i][3] + block_center_pos;
            tri2.vertices[2].pos = CubeOffsets[i][0] + block_center_pos;
            tri2.vertices[0].normal = CubeNormals[i];
            tri2.vertices[1].normal = CubeNormals[i];
            tri2.vertices[2].normal = CubeNormals[i];
            tri2.vertices[0].uv = float2{du + (UVs[i][2].x ? b : a),
                                         dv + (UVs[i][2].y ? b : a)};
            tri2.vertices[1].uv = float2{du + (UVs[i][3].x ? b : a),
                                         dv + (UVs[i][3].y ? b : a)};
            tri2.vertices[2].uv = float2{du + (UVs[i][0].x ? b : a),
                                         dv + (UVs[i][0].y ? b : a)};
        }
    }
    return triangles;
}

//24 point float3
std::vector<float3> MakeCubeWireframe(const Chunk::Index &chunk_index,const Chunk::Block& block_index){
    static const int indices[24]={
        0,1,1,2,2,3,3,0,
        4,5,5,6,6,7,7,4,
        0,4,1,5,2,6,3,7
    };
    float chunk_origin_x = chunk_index.p * Chunk::ChunkBlockSizeX;
    float chunk_origin_z = chunk_index.q * Chunk::ChunkBlockSizeZ;
    //传进来的Block确保不会是边界块
    float block_pos_x = chunk_origin_x + static_cast<float>(block_index.x) - Chunk::ChunkPadding+0.5f;
    float block_pos_z = chunk_origin_z + static_cast<float>(block_index.z) - Chunk::ChunkPadding+0.5f;
    float block_pos_y = static_cast<float>(block_index.y) + 0.5f;//y has no padding
    float3 block_center_pos = {block_pos_x,block_pos_y,block_pos_z};
    std::vector<float3> pts(24);
    for(int i = 0;i < 24;i++){
        pts[i] = block_center_pos + CubeVertices[indices[i]] * 1.1f;
    }
    return pts;
}

std::vector<Triangle> MakePlant(const Chunk::Index &chunk_index, const Chunk::Block &block_index,bool r) {
    static constexpr float3 positions[4][4] = {
            {{0.f,-0.5,-0.5f},{0.f,-0.5f,0.5f},{0.f,0.5f,-0.5f},{0.f,0.5f,0.5f}},
            {{0.f,-0.5,-0.5f},{0.f,-0.5f,0.5f},{0.f,0.5f,-0.5f},{0.f,0.5f,0.5f}},
            {{-0.5f,-0.5f,0.f},{-0.5f,0.5f,0.f},{0.5f,-0.5f,0.f},{0.5f,0.5f,0.f}},
            {{-0.5f,-0.5f,0.f},{-0.5f,0.5f,0.f},{0.5f,-0.5f,0.f},{0.5f,0.5f,0.f}}
    };
    static constexpr float3 normals[4]={
            {-1.f,0.f,0.f},
            {1.f,0.f,0.f},
            {0.f,0.f,-1.f},
            {0.f,0.f,1.f}
    };
    static constexpr float2 uvs[4][4] = {
            {{0.f,0.f},{1.f,0.f},{0.f,1.f},{1.f,1.f}},
            {{1.f,0.f},{0.f,0.f},{1.f,1.f},{0.f,1.f}},
            {{0.f,0.f},{0.f,1.f},{1.f,0.f},{1.f,1.f}},
            {{1.f,0.f},{1.f,1.f},{0.f,0.f},{0.f,1.f}}
    };
    static constexpr int indices[4][6] = {
            {0, 3, 2, 0, 1, 3},
            {0, 3, 1, 0, 2, 3},
            {0, 3, 2, 0, 1, 3},
            {0, 3, 1, 0, 2, 3}
    };

    std::vector<Triangle> triangles;
    float chunk_origin_x = chunk_index.p * Chunk::ChunkBlockSizeX;
    float chunk_origin_z = chunk_index.q * Chunk::ChunkBlockSizeZ;
    //传进来的Block确保不会是边界块
    float block_pos_x = chunk_origin_x + static_cast<float>(block_index.x) - Chunk::ChunkPadding+0.5f;
    float block_pos_z = chunk_origin_z + static_cast<float>(block_index.z) - Chunk::ChunkPadding+0.5f;
    float block_pos_y = static_cast<float>(block_index.y) + 0.5f;//y has no padding
    float3 block_center_pos = {block_pos_x,block_pos_y,block_pos_z};

    float s = 0.0625f;
    float a = 0.f;
    float b = s;
    int w = block_index.w;
    float du = (plants[w] % 16) * s;
    float dv = (plants[w] / 16) * s;
    for(int i = 0;i<4;i++){
        {
            triangles.emplace_back();
            auto &tri = triangles.back();
            for (int j = 0; j < 3; j++) {
                int idx = indices[i][j];
                tri.vertices[j].pos = block_center_pos + positions[i][idx];
                tri.vertices[j].normal = normals[i];
                tri.vertices[j].uv = {du + (uvs[i][idx][0] ? b : a),
                                      dv + (uvs[i][idx][1] ? b : a)};
            }
        }
        {
            triangles.emplace_back();
            auto &tri = triangles.back();
            for (int j = 3; j < 6; j++) {

                int idx = indices[i][j];
                tri.vertices[j - 3].pos = block_center_pos + positions[i][idx];
                tri.vertices[j - 3].normal = normals[i];
                tri.vertices[j - 3].uv = {du + (uvs[i][idx][0] ? b : a),
                                          dv + (uvs[i][idx][1] ? b : a)};
            }
        }
    }
    if(r){
        float d = simplex2(block_pos_x * 0.01, block_pos_z * 0.01, 4, 0.5, 2);
        auto t1 = translate(mat4(1.f),{-block_center_pos});
        auto rot = rotate(mat4(1.f), d, {0.f, 1.f, 0.f});
        auto t2 = translate(mat4(1.f),block_center_pos);
        rot = t2 * rot * t1;
        for (auto &tri: triangles) {
            for (auto &vert: tri.vertices) {
                vert.pos = rot * float4(vert.pos, 1.f);
                vert.normal = rot * float4(vert.normal, 0.f);
            }
        }
    }
    return triangles;
}
