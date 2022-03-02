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
        void loadChunk(int p,int q);//从数据库中加载
        void loadBlockTexture();
        void createTextureSampler();
    private:
        void testGenChunk();
    private:
        //use list because of frequent operation for append and delete
        //there is no random access but access by iterate
        std::list<Chunk> chunks;
        std::vector<Player> players;
        GLFWwindow* window;
        Camera camera;
        Shader shader;
        bool exclusive{true};
        uint32_t texture{0};
        uint32_t sampler{0};
};