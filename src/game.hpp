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
};