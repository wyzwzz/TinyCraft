#include <glad/glad.h>
#include "chunk.hpp"
#include "cube.hpp"
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

void Chunk::generateVisibleFaces() {
    this->visible_face_num = 0;
    std::vector<Triangle> visible_triangles;
    for(auto& block_item:block_map){
        int expose[6] = {0,0,0,0,0,0};
        auto& block_index = block_item.first;
        auto block_value = block_item.second;
        if(isBoundary(block_index)) continue;
        if(!isBlockOpaque({block_index.x,block_index.y-1,block_index.z})){
            expose[0] = 1;
        }
        if(!isBlockOpaque({block_index.x,block_index.y,block_index.z+1})){
            expose[1] = 1;
        }
        if(!isBlockOpaque({block_index.x+1,block_index.y,block_index.z})){
            expose[2] = 1;
        }
        if(!isBlockOpaque({block_index.x,block_index.y,block_index.z-1})){
            expose[3] = 1;
        }
        if(!isBlockOpaque({block_index.x-1,block_index.y,block_index.z})){
            expose[4] = 1;
        }
        if(!isBlockOpaque({block_index.x,block_index.y+1,block_index.z})){
            expose[5] = 1;
        }
        for(int i =0;i<6;i++){
            if(expose[i]==1){
                this->visible_face_num++;
            }
        }
        auto gen_triangles  = MakeCube(chunk_index,Block{block_index,block_value},expose);
//        std::cout<<"block "<<block_index.x<<" "<<block_index.y<<" "<<block_index.z<<std::endl;
//        std::cout<<"gen triangle num "<<gen_triangles.size()<<std::endl;
        visible_triangles.insert(visible_triangles.end(),gen_triangles.begin(),gen_triangles.end());
    }
    genVisibleFaceBuffer(visible_triangles);
}

bool Chunk::isBoundary(const Map::MapEntry& block_index) {
    return block_index.x==0 || block_index.z==0
    || block_index.x==ChunkSizeX-1 || block_index.z==ChunkSizeZ-1;
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
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*8,(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(float)*8,(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(float)*8,(void*)(6*sizeof(float)));
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
    //0 represent not exits
    if(find_value == BLOCK_STATUS_EMPTY){
        return false;
    }
    else{
        return true;
    }
}

bool Chunk::isValidBlock(const MapEntry &e) {
    return e.x>=0 && e.y>=0 && e.z>=0 && e.x<ChunkSizeX && e.y<ChunkSizeY && e.z<ChunkSizeZ;
}

