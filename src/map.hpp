#pragma once
#include <unordered_map>
#include "define.hpp"
#include "util.hpp"
namespace mdetail {
    struct MapEntry {
        int x;
        int y;
        int z;

        bool operator==(const MapEntry &e) const {
            return x == e.x && y == e.y && z == e.z;
        }
    };
}
namespace std{
    template<>
    struct hash<mdetail::MapEntry>{
        size_t operator()(const mdetail::MapEntry& e) const{
            return ::hash(e.x,e.y,e.z);
        }
    };
}
class Map{
    public:

        using MapEntry = mdetail::MapEntry;
        using ValueType = int;
        using Iterator = std::unordered_map<MapEntry,ValueType>::iterator;
        Iterator begin();
        Iterator end();
        int find(const MapEntry&);
        void set(const MapEntry&,ValueType);
    private:
        std::unordered_map<MapEntry,ValueType> m;

};