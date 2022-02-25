#pragma once
#include <unordered_map>

class Map{
    public:

        struct MapEntry{
            int x;
            int y;
            int z;
        };

        using Iterator = std::unordered_map<MapEntry,int>::iterator;
        Iterator begin();
        bool find(MapEntry);
        void set(MapEntry,int);
    private:
        std::unordered_map<MapEntry,int> m;

};