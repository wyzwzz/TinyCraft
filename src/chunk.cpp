#include <glad/glad.h>
#include "chunk.hpp"
#include "cube.hpp"
#include "item.hpp"
Chunk::Chunk(Index index): chunk_index(index) {

}

void Chunk::setBlock(const Chunk::Block &block) {
    this->block_map.set({block.x,block.y,block.z},block.w); //todo 检查是否已经存在
    this->dirty = true;
    this->dirty_count++;
}

bool Chunk::isDirty() const {
    return dirty;
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
 * A:0  B:1  C:2  D:3
 * E:4  F:5  G:6  H:7
 *
 * face 0: (0,1,2,3) bottom (A B C D)
 * face 1: (3,2,6,7) front (D C G H)
 * face 2: (2,1,5,6) right (C B F G)
 * face 3: (1,0,4,5) back (B A E F)
 * face 4: (0,3,7,4) left (A D H E)
 * face 5: (4,7,6,5) top (E H G F)
 *
 * bottom
 *        0   1   2
 *          A   B
 *        9   10  11
 *          D   C
 *        18  19  20
 * front  24  25  26
 *          H   G
 *        21  22  23
 *          D   C
 *        18  19  20
 * right  26  17  8
 *          G   F
 *        23  14  5
 *          C   B
 *        20  11  2
 * back   6   7   8
 *          E   F
 *        3   4   5
 *          A   B
 *        0   1   2
 * left   6  15  24
 *         E    H
 *        3  12  21
 *         A   D
 *        0   9  18
 * top    6   7   8
 *          E   F
 *        15  16  17
 *          H   G
 *        24  25  26
 */
void occlusion(bool neighbor[27],float shades[27],float ao[6][4]){
    static constexpr int lookup3[6][4][3]={
        {{0,1,9},{2,1,11},{20,11,19},{18,9,19}},
        {{18, 19, 21},{20,19,23},{26,23,25},{24,21,25}},
        {{20,11,23},{2,5,11},{8,17,5},{26,17,23}},
        {{2,1,5},{0,1,3},{6,3,7},{8,5,7}},
        {{0,3,9},{18,9,21},{24,15,21},{6,3,15}},
        {{6,7,15},{24,15,25},{26,17,25},{8,7,17}}
    };
    static constexpr int lookup4[6][4][4]={
        {{0,1,9,10},{1,2,10,11},{10,11,19,20},{9,10,18,19}},
        {{18,19,21,22},{19,20,22,23},{22,23,25,26},{21,22,24,25}},
        {{11,14,20,23},{2,5,11,14},{5,8,14,17},{14,17,23,26}},
        {{1,2,4,5},{0,1,3,4},{3,4,6,7},{4,5,7,8}},
        {{0,3,9,12},{9,18,12,21},{12,15,21,24},{3,6,12,15}},
        {{6,7,15,16},{15,16,24,25},{16,17,25,26},{7,8,16,17}}
    };
    static constexpr float curve[4] = {0.f,0.25f,0.5f,0.75f};
    for(int i = 0;i<6;i++){
        for(int j = 0;j<4;j++){
            int corner = neighbor[lookup3[i][j][0]];
            int side1 = neighbor[lookup3[i][j][1]];
            int side2 = neighbor[lookup3[i][j][2]];
            int value = side1 && side2 ? 3 : corner + side1 + side2;
            float shade_sum = 0.f;
            for(int k = 0;k<4;k++){
                shade_sum += shades[lookup4[i][j][k]];
            }
            float total = curve[value] + shade_sum / 4.f;
            ao[i][j] = std::min(total,1.f);
        }
    }
}
#define XYZ(x,y,z) ((y) * ChunkSizeX * ChunkSizeZ + (z) * ChunkSizeX + (x))
#define XZ(x,z) ((z) * ChunkSizeX + (x))
void Chunk::generateVisibleFaces() {
    this->visible_face_num = 0;
    std::vector<Triangle> visible_triangles;
    std::vector<bool> opaque(ChunkSize,false);
    std::vector<uint8_t> highest(ChunkSizeX*ChunkSizeZ,0);

    for(auto it = block_map.begin();it!=block_map.end();it++){
        const auto& entry = it->first;
        bool is_opaque = !isTransparent(it->second);
        opaque[XYZ(entry.x,entry.y,entry.z)] = is_opaque;
        if(is_opaque){
            highest[XZ(entry.x,entry.z)] = std::max(highest[XZ(entry.x,entry.z)],(uint8_t)entry.y);
        }
    }
    for(auto& block_item:block_map){
        bool expose[6] = {false};

        auto& block_index = block_item.first;
        auto block_value = block_item.second;
        if(block_value == BLOCK_STATUS_EMPTY) continue;

        if(isBoundary(block_index)) continue;
        if(isPlant(block_value)){
            auto tris = MakePlant(chunk_index,Block{block_index,block_value},true);
            visible_triangles.insert(visible_triangles.end(),tris.begin(),tris.end());
            this->visible_face_num+=4;
            continue;
        }
        expose[0] = !opaque[XYZ(block_index.x,block_index.y-1,block_index.z)];
        expose[1] = !opaque[XYZ(block_index.x,block_index.y,block_index.z+1)];
        expose[2] = !opaque[XYZ(block_index.x+1,block_index.y,block_index.z)];
        expose[3] = !opaque[XYZ(block_index.x,block_index.y,block_index.z-1)];
        expose[4] = !opaque[XYZ(block_index.x-1,block_index.y,block_index.z)];
        expose[5] = !opaque[XYZ(block_index.x,block_index.y+1,block_index.z)];

        int exposed_count = 0;
        for(int i =0;i<6;i++){
            if(expose[i]==1){
                this->visible_face_num++;
                exposed_count++;
            }
        }
        if(exposed_count==0) continue;
        bool neighbor[27] = {false};
        float shades[27] = {0.f};
        float ao[6][4] = {0.f};
        int index = 0;
        for(int dz = -1;dz<=1;dz++){
            for(int dy = -1;dy<=1;dy++){
                for(int dx = -1;dx<=1;dx++){
                    neighbor[index] = opaque[XYZ(block_index.x+dx,block_index.y+dy,block_index.z+dz)];
                    if(block_index.y + dy <= highest[XZ(block_index.x+dx,block_index.z+dz)]){
                        for(int oy = 0;oy<8;oy++){
                            if(opaque[XYZ(block_index.x+dx,block_index.y+dy+oy,block_index.z+dz)]){
                                shades[index] = 1.0 - oy * 0.125;
                                break;
                            }
                        }
                    }
                    index++;
                }
            }
        }
        occlusion(neighbor,shades,ao);

        auto gen_triangles  = MakeCube(chunk_index,Block{block_index,block_value},expose,ao);

        visible_triangles.insert(visible_triangles.end(),gen_triangles.begin(),gen_triangles.end());
    }
    genVisibleFaceBuffer(visible_triangles);

    //reset dirty to false
    this->dirty = false;
    this->dirty_count = 0;
}

bool Chunk::isBoundary(const Map::MapEntry& block_index) {
    return block_index.x==0 || block_index.z==0
    || block_index.x==ChunkSizeX-1 || block_index.z==ChunkSizeZ-1
        || block_index.y == 0;
}

bool Chunk::isBase(const Chunk::Block &block) {
    return block.y == 0;
}

void Chunk::genVisibleFaceBuffer(const std::vector<Triangle>& triangles) {
    if(draw_vao!=0 || draw_buffer!=0){
        deleteDrawBuffer();
    }
    GL_EXPR(glCreateVertexArrays(1,&draw_vao));
    glBindVertexArray(draw_vao);
    glCreateBuffers(1,&draw_buffer);
    glBindBuffer(GL_ARRAY_BUFFER,draw_buffer);
    glBufferData(GL_ARRAY_BUFFER,triangles.size()*sizeof(Triangle),triangles.data(),GL_STATIC_DRAW);
    GL_CHECK
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*9,(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(float)*9,(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(float)*9,(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    GL_CHECK
}

void Chunk::deleteDrawBuffer() {
    std::cout<<"call Chunk::deleteDrawBuffer()"<<std::endl;
    if(draw_buffer) glDeleteBuffers(1,&draw_buffer);
    draw_buffer = 0;
    if(draw_vao) glDeleteVertexArrays(1,&draw_vao);
    draw_vao = 0;
}

uint32_t Chunk::getDrawBuffer() {
    return draw_vao;
}

size_t Chunk::getDrawFaceNum() const {
    return visible_face_num;
}

bool Chunk::isBlockOpaque(const Map::MapEntry& block_index) {
//    assert(isValidBlock(block_index));
    if(!isValidBlock(block_index)) return false;
    auto find_value = block_map.find(block_index);

    return !isTransparent(find_value);
    //0 represent not exits
//    if(find_value == BLOCK_STATUS_EMPTY){
//        return false;
//    }
//    else{
//        return true;
//    }

}

bool Chunk::isValidBlock(const MapEntry &e) {
    return e.x>=0 && e.y>=0 && e.z>=0 && e.x<ChunkSizeX && e.y<ChunkSizeY && e.z<ChunkSizeZ;
}

int Chunk::queryBlockW(int x, int y, int z) {
    return block_map.find({x,y,z});
}

const Chunk::Index& Chunk::getIndex() const{
    return chunk_index;
}

int Chunk::getHighest(int x,int z) {
    int max_y = 0;
    for(int y = 0;y<ChunkSizeY;y++){
        int w = block_map.find({x,y,z});
        if(isObstacle(w)){
            max_y = std::max(max_y,y);
        }
    }
    return max_y+1;
}
