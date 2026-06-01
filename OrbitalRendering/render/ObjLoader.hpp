//
// Created by Candy on 4/30/2026.
//

#ifndef SOUL_OBJLOADER_HPP
#define SOUL_OBJLOADER_HPP
#include<string>
#include"MeshData.hpp"

// Reads an OBJ file and packs it into the MeshData layout this project expects.
// The output layout is 8 floats per vertex: position xyz, uv, normal xyz.
bool LoadObjToMeshData(const std::string& path , MeshData& meshout);

class ObjLoader {

};



#endif //SOUL_OBJLOADER_HPP
