#pragma once

#include <GL/gl.h>
#include <vector>

// Modern OpenGL helper for migrating from immediate mode to VBO/VAO
// Provides utilities for rendering with vertex buffers in OpenGL 4.x

struct Vertex2D {
    float x, y;
};

struct Vertex3D {
    float x, y, z;
};

struct VertexColor {
    float r, g, b;
};

struct VertexTexCoord {
    float u, v;
};

// Complete vertex with position, color, and texcoord
struct VertexPCT {
    float x, y, z;
    float r, g, b;
    float u, v;
};

// Dynamic batch renderer for effects that render many primitives
class CDynamicBatchRenderer {
private:
    GLuint m_VAO;
    GLuint m_VBO;
    std::vector<VertexPCT> m_Vertices;
    GLenum m_PrimitiveType;
    size_t m_MaxVertices;
    bool m_IsBuilding;

public:
    CDynamicBatchRenderer();
    ~CDynamicBatchRenderer();

    // Initialize with max vertices capacity
    void Initialize(size_t maxVertices);
    void Cleanup();

    // Start building a batch of primitives
    void Begin(GLenum primitiveType); // GL_TRIANGLES, GL_TRIANGLE_FAN, GL_QUADS, etc.

    // Add vertex with color and texcoord
    void AddVertex(float x, float y, float z, float r, float g, float b, float u, float v);
    void AddVertex(const float* pos, const float* color, const float* texCoord);

    // Finish and render the batch
    void End();
    void Flush(); // Render accumulated vertices

    // Get current vertex count
    size_t GetVertexCount() const { return m_Vertices.size(); }
};

// Simple VBO wrapper for static geometry
class CStaticVBO {
private:
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_VertexCount;
    GLenum m_PrimitiveType;

public:
    CStaticVBO();
    ~CStaticVBO();

    // Upload vertex data
    void Upload(const VertexPCT* vertices, size_t count, GLenum primitiveType, GLenum usage = GL_STATIC_DRAW);
    void Upload(const std::vector<VertexPCT>& vertices, GLenum primitiveType, GLenum usage = GL_STATIC_DRAW);

    // Render the VBO
    void Render();

    // Cleanup
    void Cleanup();
};

// Helper to convert immediate mode calls to batched rendering
// Usage: Replace glBegin/glEnd blocks with this
class CImmediateModeEmulator {
private:
    std::vector<VertexPCT> m_Vertices;
    GLenum m_PrimitiveType;
    bool m_IsBuilding;
    VertexPCT m_CurrentVertex;

public:
    CImmediateModeEmulator();

    void Begin(GLenum mode);
    void End();

    void Color3f(float r, float g, float b);
    void Color3fv(const float* c);
    void TexCoord2f(float u, float v);
    void TexCoord2fv(const float* t);
    void Vertex2f(float x, float y);
    void Vertex2fv(const float* v);
    void Vertex3f(float x, float y, float z);
    void Vertex3fv(const float* v);

private:
    void RenderBatch();
    void ConvertQuadsToTriangles(std::vector<VertexPCT>& outVertices);
};

// Matrix stack for replacing glPushMatrix/glPopMatrix in core profile
// Note: Currently using compatibility profile, so legacy matrix stack still works
// This class is prepared for future migration to core profile
class CMatrixStack {
private:
    struct Matrix4x4 {
        float m[16];
        Matrix4x4();
        void LoadIdentity();
        void Multiply(const Matrix4x4& other);
    };

    std::vector<Matrix4x4> m_Stack;
    Matrix4x4 m_Current;

public:
    CMatrixStack();

    void Push();
    void Pop();
    void LoadIdentity();
    void Translate(float x, float y, float z);
    void Rotate(float angle, float x, float y, float z);
    void Scale(float x, float y, float z);
    void MultMatrix(const float* m);
    void LoadMatrix(const float* m);
    const float* GetMatrix() const { return m_Current.m; }

    // Helper to sync with legacy OpenGL matrix (for compatibility mode)
    void SyncFromGL();
    void SyncToGL();
};

// Global matrix stacks (for future core profile migration)
extern CMatrixStack g_ModelViewStack;
extern CMatrixStack g_ProjectionStack;

// Global immediate mode emulator instance
extern CImmediateModeEmulator g_ImmediateModeEmulator;

// Global dynamic batch renderer for effects
extern CDynamicBatchRenderer g_EffectBatchRenderer;

// Initialize modern OpenGL systems
void InitModernGL();
void CleanupModernGL();
