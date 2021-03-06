#pragma once
#include "util.hpp"
#include "map.hpp"
#include "geometry.hpp"
#include <type_traits>
#include <unordered_set>
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
            bool operator==(const Index& index) const{
                return p== index.p && q == index.q;
            }
        };
        struct Block{
            bool operator==(const Block& block) const{
                return x==block.x && y==block.y && z==block.z && w == block.w;
            }
            Block()
            :w(0)
            {}
            Block(int x,int y,int z,int w):
            x(x),y(y),z(z),w(w){}
            Block(const Map::MapEntry& entry,int w)
            :x(entry.x),y(entry.y),z(entry.z),w(w)
            {}
//            Block(const Block&) = default;
//            Block& operator=(const Block&) = default;
            int x;
            int y;
            int z;
            int w;//value represent block's status(empty or textured)
        };
        struct BlockHash{
            size_t operator()(const Block& block) const{
                return 0;
            }
        };

        static constexpr int ChunkSizeX = 32;
        static constexpr int ChunkSizeY = 256;
        static constexpr int ChunkSizeZ = 32;
        static constexpr int ChunkSize = ChunkSizeX * ChunkSizeY * ChunkSizeZ;
        static constexpr int ChunkPadding = 1;// for x z direction
        static constexpr int ChunkBlockSizeX = ChunkSizeX - 2*ChunkPadding;
        static constexpr int ChunkBlockSizeZ = ChunkSizeZ - 2*ChunkPadding;
        using MapEntry = Map::MapEntry;
        bool isBoundary(const MapEntry&);
        bool isBase(const Block&);
        bool isValidBlock(const MapEntry&);

        Chunk(Index);

        const Index& getIndex() const;

        void setBlock(const Block& block);

        int queryBlockW(int x,int y,int z);

        bool isBlockOpaque(const Map::MapEntry&);

        //if need to re-generate mesh buffer
        bool isDirty() const;

        bool isUpdate() const;

        void setDirty(bool dirty);

        //if is modified since create, also meanings if need to store it into db
        bool isModified() const;

        //2d distance from point to the chunk center
        float distance2(float x,float z);

        const BoundBox3D& getBoundBox() const;

        //must call after call setBlock or isDirty return true
        void generateVisibleFaces();

        void generateVisibleTriangles(std::vector<Triangle>& triangles);

        void generateVisibleTriangles();

        void genVisibleFaceBuffer(const std::vector<Triangle>&);

        void genVisibleFaceBuffer();

        int getHighest(int x,int z);

        //考虑负的情况
        static Chunk::Index computeChunkIndex(float x,float z){
            return {computeChunIndexP(x), computeChunIndexQ(z)};
        }
        static int computeChunIndexP(float x){
            int p = x < 0.f ? -1 - (-x)/ChunkBlockSizeX : x / ChunkBlockSizeX;
            return p;
        }
        static int computeChunIndexQ(float z){
            int q = z < 0.f ? -1 - (-z)/ChunkBlockSizeZ : z / ChunkBlockSizeZ;
            return q;
        }

        uint32_t getDrawBuffer();

        size_t getDrawFaceNum() const;

        Map& getMap();

        void deleteDrawBuffer();

    private:

    

    private:
        BoundBox3D bbox;
        Index chunk_index;
        bool dirty{false};
        int dirty_count{0};
        bool update = false;
        Map block_map;
        int visible_face_num{0};
        uint32_t draw_buffer{0};
        uint32_t draw_vao{0};
        std::vector<Triangle> triangles;
};


