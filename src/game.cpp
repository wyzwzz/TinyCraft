#include "game.hpp"
#include "ray.hpp"
#include "cube.hpp"
#include <cmath>
extern "C"{
#include <noise.h>
}
# define M_PI           3.14159265358979323846f

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
    wireframe_shader = Shader((ShaderPath+"wireframe_v.glsl").c_str(),(ShaderPath+"wireframe_f.glsl").c_str());
    GL_CHECK
}

void Game::run(){
    generateInitialWorld();
    loadBlockTexture();
    createTextureSampler();
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

    //todo select nvidia gpu
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
                onLeftClick();
            }
            else{
                glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
            }
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT){
            if(exclusive){
                onRightClick();
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
    double last_t = glfwGetTime();
    while(!glfwWindowShouldClose(window)){
        auto cur_t = glfwGetTime();
        auto delta_t = cur_t - last_t;
        last_t = cur_t;
        std::cout<<"fps "<<int(1.0/delta_t)<<std::endl;

        glClearColor(0.f,0.f,0.f,0.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        handleMouseInput();

        //re-generate chunk draw buffer after input event
        updateDirtyChunks();

        auto view_matrix = camera.getViewMatrix();
        auto proj_matrix = camera.getProjMatrix();
        auto model_matrix = mat4(1.f);

        //render selected cube wireframe by camera ray
        renderHitBlock();



        shader.use();
        shader.setMat4("model",model_matrix);
        shader.setMat4("view",view_matrix);
        shader.setMat4("proj",proj_matrix);
        shader.setInt("BlockTexture",0);
        auto visible_chunks = getVisibleChunks();
        while(!visible_chunks.empty()){
            auto chunk = visible_chunks.front();
            visible_chunks.pop();
            //draw chunk
            auto buffer_handle = chunk->getDrawBuffer();
            auto draw_num = chunk->getDrawFaceNum() * 2 * 3;
            glBindVertexArray(buffer_handle);
//            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
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
        if(camera.pitch > 89.f){
            camera.pitch = 89.f;
        }
        else if(camera.pitch < -89.f){
            camera.pitch = -89.f;
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
            int w = 1;
            if (h <= t) {
                h = t;
                w = 2;
            }
            for(int y = 0;y<h;y++){
                chunk.setBlock({dx+pad,y,dz+pad,w});
            }
        }
    }
    chunk.generateVisibleFaces();
    this->chunks.push_back(chunk);
}

void Game::loadBlockTexture() {
    const std::string texture_path = AssetsPath + "textures/texture.png";

    stbi_set_flip_vertically_on_load(true);

    int width = 0,height = 0,channels = 0;

    auto data = stbi_load(texture_path.c_str(),&width,&height,&channels,0);
    assert(channels == 4);
    assert(data);
    glCreateTextures(GL_TEXTURE_2D,1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glBindTextureUnit(0,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    stbi_image_free(data);
    GL_CHECK
}

void Game::createTextureSampler() {
    glCreateSamplers(1,&sampler);
    glSamplerParameterf(sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glSamplerParameterf(sampler,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glSamplerParameterf(sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    glSamplerParameterf(sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glSamplerParameterf(sampler,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindSampler(0,sampler);
}

void Game::renderHitBlock() {
    auto view_matrix = camera.getViewMatrix();
    auto proj_matrix = camera.getProjMatrix();
    auto model_matrix = mat4(1.f);

    //render selected cube wireframe by camera ray
    Chunk::Index chunk_index{};
    Chunk::Block block_index{};
    if(!getHitBlock(chunk_index,block_index)) return;

    generateHitBlockBuffer(chunk_index,block_index);

    wireframe_shader.use();
    wireframe_shader.setMat4("model",model_matrix);
    wireframe_shader.setMat4("view",view_matrix);
    wireframe_shader.setMat4("proj",proj_matrix);
    glBindVertexArray(block_wireframe_vao);
    glLineWidth(1);
    glDrawArrays(GL_LINES,0,24);
}

void Game::generateHitBlockBuffer(const Chunk::Index& chunk_index,const Chunk::Block& block_index) {
    static Chunk::Index last_hit_chunk;
    static Chunk::Block last_hit_block;


    if(chunk_index == last_hit_chunk && block_index == last_hit_block){
        //nothing to do
    }
    else{
        //delete old and generate new
        deleteBlockWireframeBuffer();
        generateBlockWireframeBuffer(MakeCubeWireframe(chunk_index,block_index));
    }

    last_hit_chunk = chunk_index;
    last_hit_block = block_index;
}

bool Game::getHitBlock(Chunk::Index &chunk_index, Chunk::Block &block_index) {
    Ray ray(camera);
    int steps = ray.radius / ray.step;
    for(int i = 0;i<steps;i++){
        auto pos = ray.origin + ray.direction * ray.step * static_cast<float>(i);
        int chunk_index_x = pos.x / Chunk::ChunkBlockSizeX;
        int chunk_index_z = pos.z / Chunk::ChunkBlockSizeZ;
        int block_index_x = pos.x - chunk_index_x * Chunk::ChunkBlockSizeX + Chunk::ChunkPadding;
        int block_index_z = pos.z - chunk_index_z * Chunk::ChunkBlockSizeZ + Chunk::ChunkPadding;
        int block_index_y = pos.y;
        if(block_index_y<0 || block_index_y >= Chunk::ChunkSizeY){
            continue;
        }
        int w = getChunk(chunk_index_x,chunk_index_z).queryBlockW(block_index_x,block_index_y,block_index_z);
        if(w!=BLOCK_STATUS_EMPTY){
            chunk_index = {chunk_index_x,chunk_index_z};
            block_index = {block_index_x,block_index_y,block_index_z,w};
            return true;
        }
    }
    return false;
}

Chunk &Game::getChunk(int p,int q) {
    for(auto& chunk:chunks){
        if(chunk.getIndex() == Chunk::Index{p,q}){
            return chunk;
        }
    }
    loadChunk(p,q);
    return getChunk(p,q);
}

void Game::loadChunk(int p, int q) {
    //if chunk has store in the db

    //create the new chunk
    createChunk(p,q);
}

void Game::generateBlockWireframeBuffer(const std::vector<float3> &pts) {
    if(block_wireframe_vao || block_wireframe_vbo){
        deleteBlockWireframeBuffer();
    }
    glCreateVertexArrays(1,&block_wireframe_vao);
    glBindVertexArray(block_wireframe_vao);
    glCreateBuffers(1,&block_wireframe_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,block_wireframe_vbo);
    glBufferData(GL_ARRAY_BUFFER,pts.size()*sizeof(float3),pts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    GL_CHECK
}

void Game::deleteBlockWireframeBuffer() {
    if(block_wireframe_vao){
        glDeleteVertexArrays(1,&block_wireframe_vao);
        block_wireframe_vao = 0;
    }
    if(block_wireframe_vbo){
        glDeleteBuffers(1,&block_wireframe_vbo);
        block_wireframe_vbo = 0;
    }
}

void Game::onLeftClick() {
    Chunk::Index chunk_index{};
    Chunk::Block block_index{};
    if(!getHitBlock(chunk_index,block_index)) return;
    //update this chunk's block status
    block_index.w = BLOCK_STATUS_EMPTY;
    getChunk(chunk_index.p,chunk_index.q).setBlock(block_index);
    //if this block is boundary then update the neighbor chunk's block
    updateNeighborChunk(chunk_index,block_index);
}

void Game::updateDirtyChunks() {
    for(auto& chunk:chunks){
        if(chunk.isDirty()){
            chunk.generateVisibleFaces();
        }
    }
}

void Game::updateNeighborChunk(const Chunk::Index &chunk_index, const Chunk::Block &block_index) {
    if(block_index.x == 1){
        getChunk(chunk_index.p-1,chunk_index.q).setBlock({Chunk::ChunkSizeX-1,block_index.y,block_index.z,block_index.w});
    }
    if(block_index.x == Chunk::ChunkSizeX - 2){
        getChunk(chunk_index.p+1,chunk_index.q).setBlock({0,block_index.y,block_index.z,block_index.w});
    }
    if(block_index.z == 1){
        getChunk(chunk_index.p,chunk_index.q-1).setBlock({block_index.x,block_index.y,Chunk::ChunkSizeZ-1,block_index.w});
    }
    if(block_index.z == Chunk::ChunkSizeZ - 2){
        getChunk(chunk_index.p,chunk_index.q+1).setBlock({block_index.x,block_index.y,0,block_index.w});
    }
    if(block_index.x == 1 && block_index.z == 1){
        getChunk(chunk_index.p-1,chunk_index.q-1).setBlock({Chunk::ChunkSizeX-1,block_index.y,Chunk::ChunkSizeZ-1,block_index.w});
    }
    if(block_index.x == 1 && block_index.z == Chunk::ChunkSizeZ - 2){
        getChunk(chunk_index.p - 1,chunk_index.q + 1).setBlock({Chunk::ChunkSizeX-1,block_index.y,0,block_index.w});
    }
    if(block_index.x == Chunk::ChunkSizeX-2 && block_index.z == 1){
        getChunk(chunk_index.p + 1, chunk_index.q -1).setBlock({0,block_index.y,Chunk::ChunkSizeZ-1,block_index.w});
    }
    if(block_index.x == Chunk::ChunkSizeX - 2 && block_index.z == Chunk::ChunkSizeZ - 2){
        getChunk(chunk_index.p + 1,chunk_index.q + 1).setBlock({0,block_index.y,0,block_index.w});
    }
}

void Game::onRightClick() {
    Chunk::Index chunk_index{};
    Chunk::Block block_index{};
    int face = getHitBlockFace(chunk_index,block_index);
    if(face == -1) return;
    Chunk::Index index{};
    Chunk::Block block{};
    computeBlockAccordingToFace(chunk_index,block_index,face,index,block);
    block.w = getCurrentItemIndex();
    getChunk(index.p,index.q).setBlock(block);
    updateNeighborChunk(index,block);
}

int Game::getHitBlockFace(Chunk::Index& chunk_index,Chunk::Block& block_index) {
    Ray ray(camera);
    int steps = ray.radius / ray.step;

    for(int i = 0;i<steps;i++){
        auto pos = ray.origin + ray.direction * ray.step * static_cast<float>(i);
        int chunk_index_x = pos.x / Chunk::ChunkBlockSizeX;
        int chunk_index_z = pos.z / Chunk::ChunkBlockSizeZ;
        int block_index_x = pos.x - chunk_index_x * Chunk::ChunkBlockSizeX + Chunk::ChunkPadding;
        int block_index_z = pos.z - chunk_index_z * Chunk::ChunkBlockSizeZ + Chunk::ChunkPadding;
        int block_index_y = pos.y;
        if(block_index_y<0 || block_index_y >= Chunk::ChunkSizeY){
            continue;
        }
        int w = getChunk(chunk_index_x,chunk_index_z).queryBlockW(block_index_x,block_index_y,block_index_z);
        if(w!=BLOCK_STATUS_EMPTY){
            if(i==0){
                assert(false);
            }
            chunk_index = {chunk_index_x,chunk_index_z};
            block_index = {block_index_x,block_index_y,block_index_z,w};
            int3 cur_world_pos = {(int)pos.x,(int)pos.y,(int)pos.z};
            pos = ray.origin + ray.direction * ray.step *static_cast<float>(i-1);
            int3 last_world_pos = {(int)pos.x,(int)pos.y,(int)pos.z};
            int3 offset = last_world_pos - cur_world_pos;
            if(offset.y == -1){
                return 0;
            }
            else if(offset.z == 1){
                return 1;
            }
            else if(offset.x == 1){
                return 2;
            }
            else if(offset.z == -1){
                return 3;
            }
            else if(offset.x == -1){
                return 4;
            }
            else if(offset.y == 1){
                return 5;
            }
            else{
                throw std::runtime_error("error offset");
            }
        }
    }
    return -1;
}

void Game::computeBlockAccordingToFace(const Chunk::Index &hit_index, const Chunk::Block &hit_block, int face, Chunk::Index& index,Chunk::Block& block) {

    int world_x = hit_index.p * Chunk::ChunkBlockSizeX + hit_block.x - Chunk::ChunkPadding;
    int world_y = hit_block.y;
    int world_z = hit_index.q * Chunk::ChunkBlockSizeZ + hit_block.z - Chunk::ChunkPadding;
    assert(world_y > 0);
    int3 world_coord{world_x,world_y,world_z};
    assert(face >=0 && face <6);
    world_coord += CubeFaceOffset[face];

    computeChunkBlock(world_coord.x,world_coord.y,world_coord.z,index,block);
}

int Game::getCurrentItemIndex() {
    return 1;
}

//todo 考虑负的坐标
void Game::computeChunkBlock(int world_x, int world_y, int world_z,Chunk::Index& index,Chunk::Block& block) {
    assert(world_y > 0);
    int p = world_x / Chunk::ChunkBlockSizeX;
    int q = world_z / Chunk::ChunkBlockSizeZ;
    int x = std::abs(world_x) % Chunk::ChunkBlockSizeX + Chunk::ChunkPadding;
    int y = world_y;
    int z = std::abs(world_z) % Chunk::ChunkBlockSizeZ + Chunk::ChunkPadding;
    index.p = p;
    index.q = q;
    block.x = x;
    block.y = y;
    block.z = z;
}


