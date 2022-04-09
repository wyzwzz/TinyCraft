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
#include <thread>
#include <mutex>
class Game{
    public:
        static constexpr int MaxChunkSize = 8192;
        static constexpr int MaxPlayerNum = 8;

        Game(int argc,char** argv);

        void run();

        void shutdown();

        inline static std::function<void(GLFWwindow*,int,int,int,int)> KeyCallback;
        inline static std::function<void(GLFWwindow*,unsigned int)> CharCallback;
        inline static std::function<void(GLFWwindow*,double,double)> ScrollCallback;
        inline static std::function<void(GLFWwindow*,int,int,int)>  MouseButtonCallback;

    private:
        void handleMouseInput();
        void handleMovement(double);
        void initGLContext();
        void initEventHandle();
        void mainLoop();
        std::queue<Chunk*> getVisibleChunks();
        void generateInitialWorld();
        void createChunk(int p,int q);//生成一个新的随机的chunk
        void createChunkAsync(int p,int q);
        void loadChunk(int p,int q,bool sync = false);//加载一个chunk 保证生成 从数据库中加载或者新生成
        void loadBlockTexture();
        void createTextureSampler();
        bool isChunkLoaded(int p,int q) const;
        void computeVisibleChunks();
        void clearChunkTask();

        void renderHitBlock();
        void generateHitBlockBuffer(const Chunk::Index&,const Chunk::Block&);

        bool getHitBlock(Chunk::Index& chunk_index,Chunk::Block& block_index);
        Chunk& getChunk(int p,int q);
        void updateDirtyChunks();
        void updateNeighborChunk(const Chunk::Index& chunk_index,const Chunk::Block& block_index);
        int getHitBlockFace(Chunk::Index& chunk_index,Chunk::Block& block_index);
        void computeBlockAccordingToFace(const Chunk::Index&,const Chunk::Block&,int,Chunk::Index&,Chunk::Block&);
        int getCurrentItemIndex();
        void computeChunkBlock(float world_x,float world_y,float world_z,Chunk::Index&,Chunk::Block&);

        void createItemBuffer();
        void deleteItemBuffer();
        void drawItem();
        void getItemMatrix(mat4& model,mat4& view,mat4& proj,bool isPlant);

        void createCrossChairBuffer();
        void deleteCrossChairBuffer();
        void drawCrossChair();

        void createSkyBox();
        void drawSkyBox();
        float getDayTime();
        float getDayLight();

        bool collide(int h,float3& pos);
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
        std::mutex chunks_mtx;
        std::list<std::pair<Chunk::Index,std::thread>> chunk_create_tasks;
        std::mutex mtx;
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

        uint32_t skybox_tex{0};
        uint32_t skybox_vao{0},skybox_vbo{0},skybox_ebo{0};
        Shader skybox_shader;

        double day_time;

        bool flying{false};
};