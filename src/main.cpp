#include "game.hpp"
#include <iostream>
int main(int argc,char** argv){
    std::cout<<"Welcome to TinyCraft."<<std::endl;
    try{
        Game g(argc,argv);
        g.run();
        g.shundown();
    }
    catch(const std::exception& err){
        std::cout<<err.what()<<std::endl;
    }

    return 0;
}