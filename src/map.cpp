#include "map.hpp"

Map::Iterator Map::begin() {
    return m.begin();
}

int Map::find(const MapEntry& e) {
    if(m.find(e)==m.end()){
        return BLOCK_STATUS_EMPTY;
    }
    else{
        return m.at(e);
    }
}

void Map::set(const MapEntry& e, ValueType v) {
    m[e] = v;
}

Map::Iterator Map::end() {
    return m.end();
}
