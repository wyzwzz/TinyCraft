#include "game.hpp"

#include <cmath>
extern "C"{
#include <noise.h>
}
# define M_PI           3.14159265358979323846f
void glfwKeyCallback(GLFWwindow* window,int key,int scancode,int action,int mods){
    Game::KeyCallback(window,key,scancode,action,mods);
}
void glfwCharCallback(GLFWwindow* window,unsigned int u){
    Game::CharCallback(window,u);
}

void glfwScrollCallback(GLFWwindow* window,double xdalta,double ydelta){
    Game::ScrollCallback(window,xdalta,ydelta);
}
void glfwMouseButtonCallback(GLFWwindow* window,int button,int action,int mods){
    Game::MouseButtonCallback(window,button,action,mods);
}

Game::Game(int argc,char** argv){

    initGLContext();
    initEventHandle();
    GL_CHECK
    shader = Shader((ShaderPath+"block_v.glsl").c_str(),(ShaderPath+"block_f.glsl").c_str());
    GL_CHECK
}

void Game::run(){
    generateInitialWorld();

//    testGenChunk();
    GL_CHECK

    mainLoop();
    GL_CHECK
}

void Game::shundown(){

}

void Game::initGLContext(){
    if (glfwInit() == GLFW_FALSE)
    {
        throw std::runtime_error("Failed to init GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, true);

    this->window = glfwCreateWindow(ScreenWidth, ScreenHeight, "GLContext", nullptr, nullptr);
    if (this->window == nullptr)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(VSYNC);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        throw std::runtime_error("GLAD failed to load opengl api");
    }
    glEnable(GL_DEPTH_TEST);
    GL_CHECK
}

void Game::initEventHandle(){
    KeyCallback = [&](GLFWwindow* window,int key,int scancode,int action,int mods){
        if(key == GLFW_KEY_ESCAPE){
            if(exclusive){
                glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                exclusive = false;
            }
        }
    };
    CharCallback = [&](GLFWwindow* window,unsigned int u){
        switch (u) {
            case 'a':{
                camera.position -= camera.right * camera.move_speed;
            }break;
            case 'd':{
                camera.position += camera.right * camera.move_speed;
            }break;
            case 'w':{
                camera.position += camera.front * camera.move_speed;
            }break;
            case 's':{
                camera.position -= camera.front * camera.move_speed;
            }break;
            case ' ':{

            }break;

        }

    };
    ScrollCallback = [&](GLFWwindow* window,double xdelta,double ydelta){

    };
    MouseButtonCallback = [&](GLFWwindow* window,int button,int action,int mods){
        this->exclusive = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
        if(action !=  GLFW_PRESS) return;
        if(button == GLFW_MOUSE_BUTTON_LEFT){
            if(exclusive){

            }
            else{
                glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
            }
        }
    };
    glfwSetKeyCallback(window,glfwKeyCallback);
    glfwSetCharCallback(window,glfwCharCallback);
    glfwSetScrollCallback(window,glfwScrollCallback);
    glfwSetMouseButtonCallback(window,glfwMouseButtonCallback);
    glfwSetInputMode(window,GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::mainLoop(){

    while(!glfwWindowShouldClose(window)){

        glClearColor(0.f,0.f,0.f,0.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        handleMouseInput();

        auto view_matrix = camera.getViewMatrix();
        auto proj_matrix = camera.getProjMatrix();
        auto model_matrix = mat4(1.f);

        shader.use();
        shader.setMat4("model",model_matrix);
        shader.setMat4("view",view_matrix);
        shader.setMat4("proj",proj_matrix);
        auto visible_chunks = getVisibleChunks();
        while(!visible_chunks.empty()){
            auto chunk = visible_chunks.front();
            visible_chunks.pop();
            //draw chunk
            auto buffer_handle = chunk->getDrawBuffer();
            auto draw_num = chunk->getDrawFaceNum() * 2 * 3;
            glBindVertexArray(buffer_handle);
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            glDrawArrays(GL_TRIANGLES,0,draw_num);
        }
        GL_CHECK
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Game::testGenChunk() {
    chunks.emplace_back(Chunk::Index{0,0});
    auto& chunk = chunks.back();
    chunk.setBlock({1,2,1,1});

    chunk.setBlock({2,2,1,1});

    chunk.setBlock({1,2,2,1});

    chunk.setBlock({2,2,2,1});

    chunk.setBlock({1,2,3,1});

    chunk.setBlock({2,2,3,1});

    chunk.setBlock({3,2,1,1});

    chunk.setBlock({3,2,2,1});

    chunk.setBlock({1,3,1,1});

    chunk.setBlock({2,3,1,1});

    chunk.setBlock({1,3,2,1});

    chunk.generateVisibleFaces();
}

std::queue<Chunk *> Game::getVisibleChunks() {
    std::queue<Chunk*> q;
    for(auto& chunk:chunks){
        q.push(&chunk);
    }
    return q;
}

void Game::handleMouseInput() {
    static double px = 0.0;
    static double py = 0.0;
    static bool first = true;
    if(first){
        glfwGetCursorPos(window,&px,&py);
        first = false;
    }
    else if(exclusive){
        double mx,my;
        glfwGetCursorPos(window,&mx,&my);
        double xdelta = mx - px;
        double ydelta = my - py;
        ydelta = -ydelta;
        px = mx;
        py = my;
        float xoffset = xdelta * camera.move_sense;
        float yoffset = ydelta * camera.move_sense;
        camera.yaw += xoffset;
        camera.pitch += yoffset;
        if(camera.pitch > 60.f){
            camera.pitch = 60.f;
        }
        else if(camera.pitch < -60.f){
            camera.pitch = -60.f;
        }
        camera.front.x = std::cos(camera.pitch * M_PI / 180.f) * std::cos(camera.yaw * M_PI / 180.f);
        camera.front.y = std::sin(camera.pitch * M_PI / 180.f);
        camera.front.z = std::cos(camera.pitch * M_PI / 180.f) * std::sin(camera.yaw * M_PI / 180.f);
        camera.front = normalize(camera.front);
        camera.right = normalize(cross(camera.front,camera.world_up));
        camera.up = normalize(cross(camera.right,camera.front));
        camera.target = camera.position + camera.front;
    }
    else{
        glfwGetCursorPos(window,&px,&py);
    }

}

void Game::generateInitialWorld() {
    createChunk(0,0);
    createChunk(0,1);
    createChunk(1,0);
    createChunk(1,1);
}
/**
 * chunk里的每一个block都有全局唯一的坐标 所以padding的部分也可以得到 保证是正确的
 */
void Game::createChunk(int p, int q) {
    Chunk chunk({p,q});
    int chunk_origin_x = p * Chunk::ChunkBlockSizeX;
    int chunk_origin_z = q * Chunk::ChunkBlockSizeZ;
    int pad =Chunk::ChunkPadding;
    for(int dx = -pad;dx < Chunk::ChunkBlockSizeX+pad; dx++){
        for(int dz = -pad;dz < Chunk::ChunkBlockSizeZ+pad;dz++){
            int x = chunk_origin_x + dx;
            int z = chunk_origin_z + dz;
            float f = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
            float g = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
            int mh = g * 32 + 16;
            int h = f * mh;
            int t = 12;
            if (h <= t) {
                h = t;
            }
            for(int y = 0;y<h;y++){
                chunk.setBlock({dx+pad,y,dz+pad,1});
            }
        }
    }
    chunk.generateVisibleFaces();
    this->chunks.push_back(chunk);
}
