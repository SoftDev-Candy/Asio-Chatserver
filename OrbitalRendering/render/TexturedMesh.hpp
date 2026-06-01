//
// Created by Candy on 5/10/2026.
//

#ifndef SOUL_TEXTUREDMESH_HPP
#define SOUL_TEXTUREDMESH_HPP

#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QString>
#include <string>
#include "MeshData.hpp"

class QOpenGLShaderProgram;

class TexturedMesh
{
public:
    // Loads an OBJ, uploads its mesh data, and creates the texture used when this mesh is drawn.
    bool Initialize(const std::string& objPath, const QString& texturePath, QOpenGLShaderProgram* program);

    // Binds the mesh texture and draws the mesh with the shader program that is already active.
    void Draw(QOpenGLShaderProgram* program, int textureUnit, const char* uniformName);

    // Releases the buffers, texture, and stored mesh data owned by this mesh.
    void Destroy();

private:
    // Raw packed mesh data from the OBJ loader. It hangs around mostly so reload/debug stuff is easier later.
    MeshData mesh;

    // VAO remembers the layout, VBO stores the packed vertices, and EBO stores the triangle indices.
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ebo{QOpenGLBuffer::IndexBuffer};

    // One texture for this mesh. Keeping it simple for this project.
    QOpenGLTexture* texture = nullptr;

    // Draw count for glDrawElements so we do not have to do weird math every frame.
    int indexCount = 0;
};

#endif //SOUL_TEXTUREDMESH_HPP
