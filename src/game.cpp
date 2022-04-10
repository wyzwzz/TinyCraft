#include "game.hpp"
#include "ray.hpp"
#include "cube.hpp"
#include <cmath>
#include "item.hpp"
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
    skybox_shader = Shader((ShaderPath+"background_v.glsl").c_str(),(ShaderPath+"background_f.glsl").c_str());
    GL_CHECK
}

void Game::run(){
    generateInitialWorld();
    loadBlockTexture();
    createTextureSampler();
    createItemBuffer();
    createCrossChairBuffer();
    createSkyBox();
//    testGenChunk();
    GL_CHECK

    mainLoop();
    GL_CHECK
}

void Game::shutdown(){
    {
        std::lock_guard<std::mutex> lk(chunks_mtx);
        for(auto& pa:chunk_create_tasks){
            if(pa.second.joinable())
                pa.second.join();
        }
    }
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

    this->window = glfwCreateWindow(ScreenWidth, ScreenHeight, "TinyCraft", nullptr, nullptr);
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
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);//defualt is GL_LESS, this is for skybox trick
    //todo select nvidia gpu
}

void Game::initEventHandle(){
    KeyCallback = [&](GLFWwindow* window,int key,int scancode,int action,int mods){
        if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
            if(exclusive){
                glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                exclusive = false;
            }
        }
        else if(key == GLFW_KEY_TAB && action == GLFW_PRESS){
            flying = !flying;
        }
    };
    CharCallback = [&](GLFWwindow* window,unsigned int u){

    };
    ScrollCallback = [&](GLFWwindow* window,double xdelta,double ydelta){
        static double ypos = 0;
        ypos += ydelta;
        bool changed = false;
        if (ypos < -SCROLL_THRESHOLD) {
            current_item_index = (current_item_index + 1) % item_count;
            ypos = 0;
            changed = true;
        }
        if (ypos > SCROLL_THRESHOLD) {
            if(--current_item_index<0){
                current_item_index = item_count - 1;
            }
            ypos = 0;
            changed = true;
        }
        if(changed){
            createItemBuffer();
        }
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
        day_time = cur_t;
//        std::cout<<"fps "<<int(1.0/delta_t)<<std::endl;
        glClearColor(0.f,0.f,0.f,0.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        {
            // START_TIMER
            handleMouseInput();
            handleMovement(delta_t);
            // STOP_TIMER("process input")
        }
        {
            // START_TIMER
            computeVisibleChunks();
            // STOP_TIMER("compute visible chunks")
        }
        //re-generate chunk draw buffer after input event

        {
            // START_TIMER
            updateDirtyChunks();
            // STOP_TIMER("update dirty chunks")
        }

        {
        //render selected cube wireframe by camera ray
            // START_TIMER
            renderHitBlock();
            // STOP_TIMER("render hit block")
        }

        auto view_matrix = camera.getViewMatrix();
        auto proj_matrix = camera.getProjMatrix();
        auto model_matrix = mat4(1.f);

        shader.use();
        shader.setMat4("model",model_matrix);
        shader.setMat4("view",view_matrix);
        shader.setMat4("proj",proj_matrix);
        shader.setInt("BlockTexture",0);
        shader.setInt("equirectangularMap",1);
        shader.setVec3("view_pos",camera.position);
        shader.setFloat("fog_distance",camera.z_far*0.9);
        shader.setFloat("day_light",getDayLight());
        shader.setFloat("day_time",getDayTime());
        glBindTextureUnit(0,texture);
        glBindTextureUnit(1,skybox_tex);
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

        drawSkyBox();

        glClear(GL_DEPTH_BUFFER_BIT);



        drawItem();

        drawCrossChair();



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
    for(auto chunk:visible_chunks){
        q.push(chunk);
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
    auto pos = camera.position;
    auto r = camera.z_far*1.7f;
    BoundBox3D box = {
            pos-float3(r),
            pos+float3(r)
    };
    //计算与包围盒相交的数据块 不存储数据块的包围盒 直接根据大的包围盒的范围快速计算得到
    int min_chunk_p = Chunk::computeChunIndexP(box.min_p.x);//box.min_p.x / Chunk::ChunkBlockSizeX;
    int min_chunk_q = Chunk::computeChunIndexQ(box.min_p.z);//box.min_p.z / Chunk::ChunkBlockSizeZ;
    int max_chunk_p = Chunk::computeChunIndexP(box.max_p.x);//box.max_p.x / Chunk::ChunkBlockSizeX;
    int max_chunk_q = Chunk::computeChunIndexQ(box.max_p.z);//box.max_p.z / Chunk::ChunkBlockSizeZ;
    for(int p = min_chunk_p;p<=max_chunk_p;p++){
        for(int q = min_chunk_q;q<=max_chunk_q;q++){
            createChunkAsync(p,q);
        }
    }
    for(auto& task:chunk_create_tasks){
        if(task.second.joinable()){
            task.second.join();
        }
    }
    for(auto & chunk:chunks){
        chunk.genVisibleFaceBuffer();
    }
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
            if(w==1){
                if(simplex2(-x*0.1,z*0.1,4,0.8,2)>0.6){
                    chunk.setBlock({dx+pad,h,dz+pad,TALL_GRASS});
                }
                if(simplex2(0.05*x,-z*0.05,4,0.8,2)>0.7){
                    int w = YELLOW_FLOWER + simplex2(x*0.1,z*0.1,4,0.8,2)*7;
                    chunk.setBlock({dx+pad,h,dz+pad,w});
                }
                bool ok = true;
                if(dx - 4 < 0 || dx + 4 >= Chunk::ChunkBlockSizeX
                || dz - 4 < 0 || dz + 4 >= Chunk::ChunkBlockSizeZ){
                    ok = false;
                }
                if(ok && simplex2(x,z,6,0.5,2) > 0.87){
                    for(int y = h+3;y<h+8;y++){
                        for(int ox = -3;ox<=3;ox++){
                            for(int oz = -3;oz <=3;oz++){
                                int d = (ox*ox)+(oz*oz)+(y-(h+4))*(y-(h+4));
                                if(d<11){
                                    chunk.setBlock({dx+pad+ox,y,dz+pad+oz,LEAVES});
                                }
                            }
                        }
                    }
                    for(int y= h;y<h+7;y++){
                        chunk.setBlock({dx+pad,y,dz+pad,WOOD});
                    }
                }
                for(int y = 64;y<72;y++){
                    if(simplex3(x*0.01,y*0.1,z*0.01,8,0.5,2)>0.72){
                        chunk.setBlock({dx+pad,y,dz+pad,CLOUD});
                    }
                }
            }
        }
    }

    chunk.generateVisibleTriangles();
    std::lock_guard<std::mutex> lk(chunks_mtx);
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
    stbi_set_flip_vertically_on_load(false);
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
    Chunk::Index _chunk_index{};
    Chunk::Block _block_index{};
    computeChunkBlock(camera.position.x,camera.position.y,camera.position.z,_chunk_index,_block_index);
    if(chunk_index == _chunk_index && block_index == _block_index) return;

    if(isPlant(block_index.w)) return;

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
        int chunk_index_x = Chunk::computeChunIndexP(pos.x);
        int chunk_index_z = Chunk::computeChunIndexP(pos.z);
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
    loadChunk(p,q,true);
    return getChunk(p,q);
}

void Game::loadChunk(int p, int q,bool sync) {
    if(isChunkLoaded(p,q)) return;
    //if chunk has store in the db

    //create the new chunk
    if(sync){
        createChunk(p,q);
    }
    else
        createChunkAsync(p,q);
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
    auto& chunk = getChunk(chunk_index.p,chunk_index.q);
    chunk.setBlock(block_index);

    //if this block is boundary then update the neighbor chunk's block
    updateNeighborChunk(chunk_index,block_index);
    assert(!chunk.isBoundary({block_index.x,block_index.y+1,block_index.z}));
    if(isPlant(chunk.queryBlockW(block_index.x,block_index.y+1,block_index.z))){
        chunk.setBlock({block_index.x,block_index.y+1,block_index.z,BLOCK_STATUS_EMPTY});
        updateNeighborChunk(chunk_index,{block_index.x,block_index.y+1,block_index.z,BLOCK_STATUS_EMPTY});
    }
}

void Game::updateDirtyChunks() {
    
    for(auto& chunk:chunks){
        if(chunk.isDirty()){
            chunk.generateVisibleFaces();
        }
        else if(chunk.isUpdate()){
            START_TIMER
            chunk.genVisibleFaceBuffer();
            STOP_TIMER("gen buffer")
        }
    }
    GL_EXPR(glFinish());
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
    // std::cout<<"face "<<face<<std::endl;
    if(face == -1) return;
    if(isPlant(block_index.w)) return;//can not select plant

    Chunk::Index index{};
    Chunk::Block block{};
    computeBlockAccordingToFace(chunk_index,block_index,face,index,block);
    block.w = getCurrentItemIndex();

    // std::cout<<"index: "<<index.p<<" "<<index.q<<std::endl;
    // std::cout<<"block: "<<block.x<<" "<<block.y<<" "<<block.z<<std::endl; 

    getChunk(index.p,index.q).setBlock(block);
    updateNeighborChunk(index,block);
    Chunk::Index _index{};
    Chunk::Block _block{};
    
    auto pos = camera.position;
    computeChunkBlock(pos.x,pos.y,pos.z,_index,_block);
    _block.w = block.w;
    // std::cout<<"_index: "<<_index.p<<" "<<_index.q<<std::endl;
    // std::cout<<"_block: "<<_block.x<<" "<<_block.y<<" "<<_block.z<<std::endl;

    bool ok = true;
    if((_index == index && _block == block) || (_index == index && _block.y == block.y + 1 && block.x == _block.x && block.z == _block.z)){
        ok = false;
    }

    collide(2,pos);

    if(!ok){
        // std::cout<<"add will collide"<<std::endl;
        block.w = BLOCK_STATUS_EMPTY;
        getChunk(index.p,index.q).setBlock(block);
        updateNeighborChunk(index,block);
    }

}

int Game::getHitBlockFace(Chunk::Index& chunk_index,Chunk::Block& block_index) {
    Ray ray(camera);
    int steps = ray.radius / ray.step;

    for(int i = 0;i<steps;i++){
        auto pos = ray.origin + ray.direction * ray.step * static_cast<float>(i);
        int chunk_index_x = Chunk::computeChunIndexP(pos.x);
        int chunk_index_z = Chunk::computeChunIndexP(pos.z);
        int block_index_x = pos.x - chunk_index_x * Chunk::ChunkBlockSizeX + Chunk::ChunkPadding;
        int block_index_z = pos.z - chunk_index_z * Chunk::ChunkBlockSizeZ + Chunk::ChunkPadding;
        int block_index_y = pos.y;
        if(block_index_y<0 || block_index_y >= Chunk::ChunkSizeY){
            continue;
        }
        int w = getChunk(chunk_index_x,chunk_index_z).queryBlockW(block_index_x,block_index_y,block_index_z);
        if(w!=BLOCK_STATUS_EMPTY){
            if(i==0){
                throw std::runtime_error("inside no empty block");
            }
            chunk_index = {chunk_index_x,chunk_index_z};
            block_index = {block_index_x,block_index_y,block_index_z,w};
            int3 cur_world_pos = {std::floor(pos.x),std::floor(pos.y),std::floor(pos.z)};
            pos = ray.origin + ray.direction * ray.step *static_cast<float>(i-1);
            int3 last_world_pos = {std::floor(pos.x),std::floor(pos.y),std::floor(pos.z)};
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
                std::cout<<"error face"<<std::endl;
                return -1;
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
    return items[current_item_index];
}

//todo 考虑负的坐标
void Game::computeChunkBlock(float world_x, float world_y, float world_z,Chunk::Index& index,Chunk::Block& block) {
//    assert(world_y > 0);
    int p = Chunk::computeChunIndexP(world_x);
    int q = Chunk::computeChunIndexQ(world_z);
    int x = world_x - p * Chunk::ChunkBlockSizeX + Chunk::ChunkPadding;
    int y = world_y;
    int z = world_z - q * Chunk::ChunkBlockSizeZ + Chunk::ChunkPadding;
    index.p = p;
    index.q = q;
    block.x = x;
    block.y = y;
    block.z = z;
}

void Game::drawItem() {
    mat4 model_matrix,view_matrix,proj_matrix;
    getItemMatrix(model_matrix,view_matrix,proj_matrix, isPlant(getCurrentItemIndex()));
    shader.use();   
    shader.setVec3("view_pos",float3{0.f,0.f,3.f});
    shader.setMat4("model",model_matrix);
    shader.setMat4("view",view_matrix);
    shader.setMat4("proj",proj_matrix);
    shader.setFloat("day_light",1.f);
    assert(item_vao);
    glBindVertexArray(item_vao);
    glDrawArrays(GL_TRIANGLES,0,36);
    glBindVertexArray(0);
}

void Game::getItemMatrix(mat4& model,mat4& view,mat4& proj,bool isPlant) {
    auto t1 = translate(glm::mat4(1.f),{-0.5f,-0.5f,-0.5f});
    auto r1 = rotate(glm::mat4(1.f),radians(45.f),{0.f,1.f,0.f});
    auto r2 = rotate(glm::mat4(1.f), radians(15.f),{1.f,0.f,0.f});
    auto s1 = scale(glm::mat4(1.f),{0.1f,0.1f,0.1f});
    auto t2 = translate(glm::mat4(1.f),{-1.1f,-0.6f,0.f});
    if(isPlant)
        model = t2 * s1 * t1;
    else
        model = t2 * s1 * r2 * r1 * t1;
    view = lookAt(float3{0.f,0.f,3.f},{0.f,0.f,0.f},{0.f,1.f,0.f});
    proj = ortho(-0.7f*ScreenAspect,0.7f*ScreenAspect,-0.7f,0.7f,0.1f,5.f);
}

void Game::createItemBuffer() {
    static bool expose[6] = {1,1,1,1,1,1};
    static float ao[6][4]={0.f};
    if(item_vao || item_vbo){
        deleteItemBuffer();
    }
    int w = getCurrentItemIndex();
    auto getTriangles = [=](int w){
        if(isPlant(w)){
            return MakePlant({0,0},{1,0,1,w});
        }
        else{
            return MakeCube({0,0},{1,0,1,w},expose,ao);
        }
    };
    auto triangles = getTriangles(w);
    glCreateVertexArrays(1,&item_vao);
    glBindVertexArray(item_vao);
    glCreateBuffers(1,&item_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,item_vbo);

    assert(sizeof(Triangle)==sizeof(float)*9*3);
    glBufferData(GL_ARRAY_BUFFER,triangles.size()*sizeof(Triangle),triangles.data(),GL_STATIC_DRAW);
    glFinish();
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

void Game::deleteItemBuffer() {
    if(item_vao){
        GL_EXPR(glDeleteVertexArrays(1,&item_vao));
    }
    if(item_vbo){
        GL_EXPR(glDeleteBuffers(1,&item_vbo))
    }
}

void Game::createCrossChairBuffer() {
    static float3 CrossChairVertices[4] = {
            {-0.03f,0.f,0.f},{0.03f,0.f,0.f},
            {0.f,-0.03f,0.f},{0.f,0.03f,0.f}
    };
    if(cross_chair_vao || cross_chair_vbo){
        deleteCrossChairBuffer();
    }
    glCreateVertexArrays(1,&cross_chair_vao);
    glBindVertexArray(cross_chair_vao);
    glCreateBuffers(1,&cross_chair_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,cross_chair_vbo);
    glBufferData(GL_ARRAY_BUFFER,4*sizeof(float3),CrossChairVertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(cross_chair_vao);
    GL_CHECK
}

void Game::drawCrossChair() {
    static mat4 model = mat4(1.f);
    static mat4 view = lookAt(float3{0.f,0.f,3.f},{0.f,0.f,0.f},{0.f,1.f,0.f});
    static mat4 proj = ortho(-0.7f*ScreenAspect,0.7f*ScreenAspect,-0.7f,0.7f,0.1f,5.f);

    if(cross_chair_vao == 0) return;


    wireframe_shader.use();
    wireframe_shader.setMat4("model",model);
    wireframe_shader.setMat4("view",view);
    wireframe_shader.setMat4("proj",proj);
    glBindVertexArray(cross_chair_vao);
    glLineWidth(3.f);
    glDrawArrays(GL_LINES,0,4);
    glBindVertexArray(0);

}

void Game::deleteCrossChairBuffer() {
    if(cross_chair_vao){
        glDeleteVertexArrays(1,&cross_chair_vao);
    }
    if(cross_chair_vbo){
        glDeleteBuffers(1,&cross_chair_vbo);
    }
}

bool Game::isChunkLoaded(int p,int q) const{
    for(auto& chunk:chunks){
        if(chunk.getIndex() == Chunk::Index{p,q}){
            return true;
        }
    }
    return false;
}

void Game::computeVisibleChunks() {
    visible_chunks.clear();
    clearChunkTask();
    //get view frustum of camera
    auto vp = camera.getProjMatrix() * camera.getViewMatrix();
    Frustum frustum;
    ExtractFrustumFromProjViewMatrix(vp,frustum);
    //get bound box of frustum
    auto box = GetBoundBoxFromCamera(camera);
    //计算与包围盒相交的数据块 不存储数据块的包围盒 直接根据大的包围盒的范围快速计算得到
    int min_chunk_p = Chunk::computeChunIndexP(box.min_p.x);//box.min_p.x / Chunk::ChunkBlockSizeX;
    int min_chunk_q = Chunk::computeChunIndexQ(box.min_p.z);//box.min_p.z / Chunk::ChunkBlockSizeZ;
    int max_chunk_p = Chunk::computeChunIndexP(box.max_p.x);//box.max_p.x / Chunk::ChunkBlockSizeX;
    int max_chunk_q = Chunk::computeChunIndexQ(box.max_p.z);//box.max_p.z / Chunk::ChunkBlockSizeZ;
    auto make_boundbox = [](float x,float y,float z,float len_x,float len_y,float len_z){
        return BoundBox3D{
            float3{x*len_x,y*len_y,z*len_z},
            float3{(x+1)*len_x,(y+1)*len_y,(z+1)*len_z}
        };
    };
    std::vector<Chunk::Index> vis_chunks;
    for(int p = min_chunk_p;p<=max_chunk_p;p++){
        for(int q = min_chunk_q;q<=max_chunk_q;q++){
            assert(Chunk::ChunkBlockSizeX == Chunk::ChunkBlockSizeZ);
            auto bbox = make_boundbox(p,0,q,Chunk::ChunkBlockSizeX,Chunk::ChunkSizeY,Chunk::ChunkBlockSizeZ);
            if(!FrustumIntersectWithBoundBox(frustum,bbox)) continue;
            if(!isChunkLoaded(p,q)){
                loadChunk(p,q);
            }
            vis_chunks.push_back({p,q});
        }
    }
    // std::cout<<"visiable chunk count "<<vis_chunks.size()<<std::endl;
    std::lock_guard<std::mutex> lk(chunks_mtx);
    for(auto& idx:vis_chunks)
        if(isChunkLoaded(idx.p,idx.q))
            visible_chunks.push_back(&getChunk(idx.p,idx.q));
}
void Game::handleMovement(double dt)
{
    dt = 1.0 / 60.0;
    static float dy = 0.f;
    static float a = 0.f;
    auto op = camera.position;
    auto np = camera.position;
    float3 front = camera.front;
    if(!flying) front.y = 0.f;
    front = normalize(front);
    float3 right = camera.right;
    if(!flying) right.y = 0.f;
    right = normalize(right);
    if(glfwGetKey(window,'W')){
        np += front  ;
    }
    if(glfwGetKey(window,'S')){
        np -= front  ;
    }
    if(glfwGetKey(window,'A')) {
        np -= right;
    }
    if(glfwGetKey(window,'D')){
        np += right  ;
    }
    auto d = np - op;
    if(!flying)
        d.y = 0.f;

    if(glfwGetKey(window,' ')){
        if(flying){
           dy = 0.f;
        }
        else{
            if(dy == 0.f && a == 0.f){
                dy = 5.f;
            }
        }

    }
    float speed = flying ? camera.move_speed * 3 : camera.move_speed;
    int steps = 8;
    float ut = dt / steps;


    for(int i = 0;i<steps;i++){
        if(flying){
            dy = 0;
        }
        else{
            dy -= ut * 25;
            dy = (std::max)(dy, -250.f);
        }
        
        op +=  d * ut * speed;
        op.y += dy * ut * speed;
        bool col = collide(2,op);
        if(dy > 0.f){
            a = 1.f;
        }
        else{
            a = -1.f;
        }
        if(col){
            dy = 0.f;
            a = 0.f;
        }
    }
    camera.position = op;
    if(camera.position.y < 0){
        Chunk::Index chunk_index;
        Chunk::Block block_index;
        computeChunkBlock(camera.position.x,camera.position.y,camera.position.z,
                          chunk_index,block_index);
        int y = getChunk(chunk_index.p,chunk_index.q).getHighest(block_index.x,block_index.z);
        camera.position.y = y+1.5f;
    }

    camera.target = camera.position + camera.front;
}

void Game::createSkyBox() {
    std::string path = AssetsPath + "textures/sky.png";
    int width = 0,height = 0,channels = 0;
    stbi_set_flip_vertically_on_load(true);
    auto data = stbi_load(path.c_str(),&width,&height,&channels,0);
    assert(channels == 4);
    assert(data);
    glCreateTextures(GL_TEXTURE_2D,1,&skybox_tex);
    glBindTexture(GL_TEXTURE_2D,skybox_tex);
    glBindTextureUnit(1,skybox_tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE ,data);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    stbi_set_flip_vertically_on_load(false);
    stbi_image_free(data);

    glCreateVertexArrays(1,&skybox_vao);
    glBindVertexArray(skybox_vao);
    glCreateBuffers(1,&skybox_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,skybox_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float3)*8,GetCubeVertices(),GL_STATIC_DRAW);
    glCreateBuffers(1,&skybox_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,skybox_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(int)*36,GetCubeIndices(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    GL_CHECK



}

void Game::drawSkyBox() {
    skybox_shader.use();
    skybox_shader.setMat4("view",camera.getViewMatrix());
    skybox_shader.setMat4("proj",camera.getProjMatrix());
    skybox_shader.setInt("equirectangularMap",1);
    skybox_shader.setFloat("exposure",1.f);

    skybox_shader.setFloat("day_time",getDayTime());
    glFrontFace(GL_CW);
    glBindVertexArray(skybox_vao);
    glBindTextureUnit(1,skybox_tex);
    glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,nullptr);
    glFrontFace(GL_CCW);
}

float Game::getDayLight() {
    float day_t = getDayTime();
    if (day_t < 0.5) {
        float t = (day_t - 0.25) * 100;
        return 1 / (1 + powf(2, -t));
    }
    else {
        float t = (day_t - 0.85) * 100;
        return 1 - 1 / (1 + powf(2, -t));
    }
}

float Game::getDayTime() {
    float t = day_time * 0.003;
    t = t - (int)t;
    return t;
}

void Game::createChunkAsync(int p, int q) {
    std::lock_guard<std::mutex> lk(mtx);
    for(auto it=chunk_create_tasks.begin();it!=chunk_create_tasks.end();it++){
        if(it->first.p == p && it->first.q == q){
            return;
        }
    }
    Chunk::Index index{p,q};
    chunk_create_tasks.emplace_back(index,std::thread([this,p,q](){
        createChunk(p,q);
    }));
}

void Game::clearChunkTask() {
    std::lock_guard<std::mutex> lk(mtx);
    for(auto it = chunk_create_tasks.begin();it!=chunk_create_tasks.end();it++){
        if(!it->second.joinable()){
            chunk_create_tasks.erase(it);
        }
    }
}

bool Game::collide(int h,float3& pos) {
    Chunk::Index chunk_index;
    Chunk::Block block_index;
    computeChunkBlock(pos.x,pos.y,pos.z,
                      chunk_index,block_index);
    if(isObstacle(block_index.w)){
        throw std::runtime_error("error: inside block");
    }

    auto& chunk = getChunk(chunk_index.p,chunk_index.q);


    int nx = block_index.x + chunk_index.p * Chunk::ChunkBlockSizeX;
    int ny = block_index.y;
    int nz = block_index.z + chunk_index.q * Chunk::ChunkBlockSizeZ;

    float px = pos.x + Chunk::ChunkPadding - nx;
    float py = pos.y - ny;
    float pz = pos.z + Chunk::ChunkPadding - nz;
    float pad = 0.25f;
    bool res = false;
    for(int dy = 0;dy < h;dy ++){
        if(px < pad && isObstacle(chunk.queryBlockW(block_index.x-1,block_index.y-dy,block_index.z))){
            pos.x = nx + pad - Chunk::ChunkPadding;
        }
        if(px > 1.f - pad && isObstacle(chunk.queryBlockW(block_index.x+1,block_index.y-dy,block_index.z))){
            pos.x = nx + 1.f - pad - Chunk::ChunkPadding;
        }
        if(pz < pad && isObstacle(chunk.queryBlockW(block_index.x,block_index.y-dy,block_index.z-1))){
            pos.z = nz + pad - Chunk::ChunkPadding;
        }
        if(pz > 1.f - pad && isObstacle(chunk.queryBlockW(block_index.x,block_index.y-dy,block_index.z+1))){
            pos.z = nz + 1.f - pad - Chunk::ChunkPadding;
        }
        if(py < pad && isObstacle(chunk.queryBlockW(block_index.x,block_index.y-dy-1,block_index.z))){
            pos.y = ny + pad;
            res = true;
        }
        if(py > 1.f - pad && isObstacle(chunk.queryBlockW(block_index.x,block_index.y-dy+1,block_index.z))){
            pos.y = ny + 1.f - pad;
            res = true;
        }
    }

    return res;
}


