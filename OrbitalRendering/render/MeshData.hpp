//
// Created by Candy on 4/30/2026.
//

#ifndef SOUL_MESHDATA_HPP
#define SOUL_MESHDATA_HPP

#include<vector>

// Small packed mesh container used by the OBJ loader and the GL upload helper.
// vertices = interleaved float buffer, indices = triangle index list.
struct MeshData
{
    // Layout is: position xyz, uv, normal xyz.
    std::vector<float>vertices;

    // Triangle indices that point into the packed vertex buffer.
    std::vector<unsigned int>indices;

};

#endif //SOUL_MESHDATA_HPP
