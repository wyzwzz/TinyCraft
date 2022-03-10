#pragma once
#include <vector>
#include <string>
#include <list>
#include "chunk.hpp"
#include "camera.hpp"
#include "player.hpp"
#include <functional>
#include "shader_program.hpp"
#include <GLFW/glfw3.h>
#include <queue>
class Game{
    public:
        static constexpr int MaxChunkSize = 8192;
        static constexpr int MaxPlayerNum = 8;

        Game(int argc,char** argv);

        void run();

        void shundown();

        inline static std::function<void(GLFWwindow*,int,int,int,int)> KeyCallback;
        inline static std::function<void(GLFWwindow*,unsigned int)> CharCallback;
        inline static std::function<void(GLFWwindow*,double,double)> ScrollCallback;
        inline static std::function<void(GLFWwindow*,int,int,int)>  MouseButtonCallback;

    private:
        void handleMouseInput();
        void initGLContext();
        void initEventHandle();
        void mainLoop();
        std::queue<Chunk*> getVisibleChunks();
        void generateInitialWorld();
        void createChunk(int p,int q);//生成一个新的随机的chunk
        void loadChunk(int p,int q);//加载一个chunk 保证生成 从数据库中加载或者新生成
        void loadBlockTexture();
        void createTextureSampler();
        bool isChunkLoaded(int p,int q) const;
        void computeVisibleChunks();

        void renderHitBlock();
        void generateHitBlockBuffer(const Chunk::Index&,const Chunk::Block&);

        bool getHitBlock(Chunk::Index& chunk_index,Chunk::Block& block_index);
        Chunk& getChunk(int p,int q);
        void updateDirtyChunks();
        void updateNeighborChunk(const Chunk::Index& chunk_index,const Chunk::Block& block_index);
        int getHitBlockFace(Chunk::Index& chunk_index,Chunk::Block& block_index);
        void computeBlockAccordingToFace(const Chunk::Index&,const Chunk::Block&,int,Chunk::Index&,Chunk::Block&);
        int getCurrentItemIndex();
        void computeChunkBlock(int world_x,int world_y,int world_z,Chunk::Index&,Chunk::Block&);

        void createItemBuffer();
        void deleteItemBuffer();
        void drawItem();
        void getItemMatrix(mat4& model,mat4& view,mat4& proj);

        void createCrossChairBuffer();
        void deleteCrossChairBuffer();
        void drawCrossChair();
private:
        void onLeftClick();
        void onRightClick();
        void testGenChunk();
        void deleteBlockWireframeBuffer();
        void generateBlockWireframeBuffer(const std::vector<float3>& pts);
    private:
        //use list because of frequent operation for append and delete
        //there is no random access but access by iterate
        std::list<Chunk> chunks;//todo replace with lru
        std::list<Chunk*> visible_chunks;
        std::vector<Player> players;
        GLFWwindow* window;
        Camera camera;
        Shader shader;
        bool exclusive{true};
        uint32_t texture{0};
        uint32_t sampler{0};
        uint32_t block_wireframe_vao{0},block_wireframe_vbo{0};

        uint32_t item_vao{0},item_vbo{0};
        int current_item_index{0};

        uint32_t cross_chair_vao{0},cross_chair_vbo{0};

        Shader wireframe_shader;
};