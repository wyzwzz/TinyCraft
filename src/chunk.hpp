#pragma once
#include "map.hpp"
#include "geometry.hpp"
/**
 * 整个世界由一组二维的Chunk组成 所以Chunk的索引只需要两个数值 
 * 只有与相机视锥体相交的Chunk会被渲染
 * 遍历Chunk里的每一个Block 只有没有被遮挡的面会生成三角形被绘制
 * 一个Chunk会整个更新它的可见面buffer只要有一个它的Block发生了变化
 * 当一个Chunk与玩家的距离太远时 需要被删除或者保存到DB中
 */
class Chunk{
    public:
        struct Index{
            int p;
            int q;
        };
        struct Block{
            int x;
            int y;
            int z;
            int w;//value represent block's status(empty or textured)
        };
        static constexpr int ChunkSizeX = 32;
        static constexpr int ChunkSizeY = 256;
        static constexpr int ChunkSizeZ = 32;
        static constexpr int ChunkSize = ChunkSizeX * ChunkSizeY * ChunkSizeZ;
        static constexpr int ChunkPadding = 1;

        Chunk();

        void setBlock(const Block& block);

        //if need to re-generate mesh buffer
        bool isDirty() const;

        //if is modified since create, also meanings if need to store it into db
        bool isModified() const;

        //2d distance from point to the chunk center
        float distance2(float x,float z);

        const BoundBox3D& getBoundBox() const;

        void generateVisiableFaces();

        uint32_t getDrawBuffer();

        size_t getDrawFaceNum() const;

        Map& getMap();

    private:
        BoundBox3D bbox;
        Index chunk_index;
        Map block_map;
};
