#include "stdafx.h"
#include "ModernGL.h"
#include <GL/gl.h>

// OpenGL function pointers with unique names to avoid GLEW conflicts
static PFNGLGENVERTEXARRAYSPROC pglGenVertexArrays = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC pglDeleteVertexArrays = nullptr;
static PFNGLBINDVERTEXARRAYPROC pglBindVertexArray = nullptr;
static PFNGLGENBUFFERSPROC pglGenBuffers = nullptr;
static PFNGLDELETEBUFFERSPROC pglDeleteBuffers = nullptr;
static PFNGLBINDBUFFERPROC pglBindBuffer = nullptr;
static PFNGLBUFFERDATAPROC pglBufferData = nullptr;
static PFNGLBUFFERSUBDATAPROC pglBufferSubData = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC pglVertexAttribPointer = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray = nullptr;

// Global instances
CImmediateModeEmulator g_ImmediateModeEmulator;
CDynamicBatchRenderer g_EffectBatchRenderer;

// Load OpenGL extension functions
static bool LoadGLExtensions()
{
    pglGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    pglDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
    pglBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    pglGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    pglDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
    pglBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    pglBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    pglBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
    pglVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    pglEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");

    return pglGenVertexArrays && pglDeleteVertexArrays && pglBindVertexArray &&
           pglGenBuffers && pglDeleteBuffers && pglBindBuffer &&
           pglBufferData && pglBufferSubData &&
           pglVertexAttribPointer && pglEnableVertexAttribArray;
}

// ============================================================================
// CDynamicBatchRenderer Implementation
// ============================================================================

CDynamicBatchRenderer::CDynamicBatchRenderer()
    : m_VAO(0), m_VBO(0), m_MaxVertices(0), m_IsBuilding(false), m_PrimitiveType(GL_TRIANGLES)
{
}

CDynamicBatchRenderer::~CDynamicBatchRenderer()
{
    Cleanup();
}

void CDynamicBatchRenderer::Initialize(size_t maxVertices)
{
    m_MaxVertices = maxVertices;
    m_Vertices.reserve(maxVertices);

    // Generate VAO and VBO
    pglGenVertexArrays(1, &m_VAO);
    pglGenBuffers(1, &m_VBO);

    pglBindVertexArray(m_VAO);
    pglBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Allocate buffer
    pglBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(VertexPCT), nullptr, GL_DYNAMIC_DRAW);

    // Setup vertex attributes
    // Position
    pglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)0);
    pglEnableVertexAttribArray(0);

    // Color
    pglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)(3 * sizeof(float)));
    pglEnableVertexAttribArray(1);

    // TexCoord
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)(6 * sizeof(float)));
    pglEnableVertexAttribArray(2);

    pglBindVertexArray(0);
}

void CDynamicBatchRenderer::Cleanup()
{
    if (m_VBO) {
        pglDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_VAO) {
        pglDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

void CDynamicBatchRenderer::Begin(GLenum primitiveType)
{
    m_PrimitiveType = primitiveType;
    m_IsBuilding = true;
    m_Vertices.clear();
}

void CDynamicBatchRenderer::AddVertex(float x, float y, float z, float r, float g, float b, float u, float v)
{
    if (!m_IsBuilding) return;

    VertexPCT vertex;
    vertex.x = x; vertex.y = y; vertex.z = z;
    vertex.r = r; vertex.g = g; vertex.b = b;
    vertex.u = u; vertex.v = v;

    m_Vertices.push_back(vertex);

    // Auto-flush if we hit capacity
    if (m_Vertices.size() >= m_MaxVertices) {
        Flush();
    }
}

void CDynamicBatchRenderer::AddVertex(const float* pos, const float* color, const float* texCoord)
{
    AddVertex(pos[0], pos[1], pos[2], color[0], color[1], color[2], texCoord[0], texCoord[1]);
}

void CDynamicBatchRenderer::End()
{
    if (m_IsBuilding) {
        Flush();
        m_IsBuilding = false;
    }
}

void CDynamicBatchRenderer::Flush()
{
    if (m_Vertices.empty()) return;

    pglBindVertexArray(m_VAO);
    pglBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Upload vertex data
    pglBufferSubData(GL_ARRAY_BUFFER, 0, m_Vertices.size() * sizeof(VertexPCT), m_Vertices.data());

    // Handle GL_QUADS conversion (deprecated in OpenGL 4 core)
    if (m_PrimitiveType == GL_QUADS) {
        // Convert quads to triangles
        size_t quadCount = m_Vertices.size() / 4;
        for (size_t i = 0; i < quadCount; i++) {
            // Draw two triangles per quad (0,1,2) and (0,2,3)
            GLuint indices[6] = {
                (GLuint)(i * 4 + 0), (GLuint)(i * 4 + 1), (GLuint)(i * 4 + 2),
                (GLuint)(i * 4 + 0), (GLuint)(i * 4 + 2), (GLuint)(i * 4 + 3)
            };
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
        }
    }
    else {
        // Draw primitives
        glDrawArrays(m_PrimitiveType, 0, (GLsizei)m_Vertices.size());
    }

    pglBindVertexArray(0);
    m_Vertices.clear();
}

// ============================================================================
// CStaticVBO Implementation
// ============================================================================

CStaticVBO::CStaticVBO()
    : m_VAO(0), m_VBO(0), m_VertexCount(0), m_PrimitiveType(GL_TRIANGLES)
{
}

CStaticVBO::~CStaticVBO()
{
    Cleanup();
}

void CStaticVBO::Upload(const VertexPCT* vertices, size_t count, GLenum primitiveType, GLenum usage)
{
    Cleanup();

    m_VertexCount = count;
    m_PrimitiveType = primitiveType;

    pglGenVertexArrays(1, &m_VAO);
    pglGenBuffers(1, &m_VBO);

    pglBindVertexArray(m_VAO);
    pglBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    pglBufferData(GL_ARRAY_BUFFER, count * sizeof(VertexPCT), vertices, usage);

    // Position
    pglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)0);
    pglEnableVertexAttribArray(0);

    // Color
    pglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)(3 * sizeof(float)));
    pglEnableVertexAttribArray(1);

    // TexCoord
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)(6 * sizeof(float)));
    pglEnableVertexAttribArray(2);

    pglBindVertexArray(0);
}

void CStaticVBO::Upload(const std::vector<VertexPCT>& vertices, GLenum primitiveType, GLenum usage)
{
    Upload(vertices.data(), vertices.size(), primitiveType, usage);
}

void CStaticVBO::Render()
{
    if (m_VAO == 0 || m_VertexCount == 0) return;

    pglBindVertexArray(m_VAO);

    if (m_PrimitiveType == GL_QUADS) {
        // Convert quads to triangles
        size_t quadCount = m_VertexCount / 4;
        for (size_t i = 0; i < quadCount; i++) {
            GLuint indices[6] = {
                (GLuint)(i * 4 + 0), (GLuint)(i * 4 + 1), (GLuint)(i * 4 + 2),
                (GLuint)(i * 4 + 0), (GLuint)(i * 4 + 2), (GLuint)(i * 4 + 3)
            };
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
        }
    }
    else {
        glDrawArrays(m_PrimitiveType, 0, (GLsizei)m_VertexCount);
    }

    pglBindVertexArray(0);
}

void CStaticVBO::Cleanup()
{
    if (m_VBO) {
        pglDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_VAO) {
        pglDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    m_VertexCount = 0;
}

// ============================================================================
// CImmediateModeEmulator Implementation
// ============================================================================

CImmediateModeEmulator::CImmediateModeEmulator()
    : m_IsBuilding(false), m_PrimitiveType(GL_TRIANGLES)
{
    m_CurrentVertex = {};
    m_CurrentVertex.r = 1.0f;
    m_CurrentVertex.g = 1.0f;
    m_CurrentVertex.b = 1.0f;
}

void CImmediateModeEmulator::Begin(GLenum mode)
{
    m_IsBuilding = true;
    m_PrimitiveType = mode;
    m_Vertices.clear();
}

void CImmediateModeEmulator::End()
{
    if (!m_IsBuilding) return;

    RenderBatch();

    m_IsBuilding = false;
    m_Vertices.clear();
}

void CImmediateModeEmulator::Color3f(float r, float g, float b)
{
    m_CurrentVertex.r = r;
    m_CurrentVertex.g = g;
    m_CurrentVertex.b = b;
}

void CImmediateModeEmulator::Color3fv(const float* c)
{
    m_CurrentVertex.r = c[0];
    m_CurrentVertex.g = c[1];
    m_CurrentVertex.b = c[2];
}

void CImmediateModeEmulator::TexCoord2f(float u, float v)
{
    m_CurrentVertex.u = u;
    m_CurrentVertex.v = v;
}

void CImmediateModeEmulator::TexCoord2fv(const float* t)
{
    m_CurrentVertex.u = t[0];
    m_CurrentVertex.v = t[1];
}

void CImmediateModeEmulator::Vertex2f(float x, float y)
{
    m_CurrentVertex.x = x;
    m_CurrentVertex.y = y;
    m_CurrentVertex.z = 0.0f;
    m_Vertices.push_back(m_CurrentVertex);
}

void CImmediateModeEmulator::Vertex2fv(const float* v)
{
    Vertex2f(v[0], v[1]);
}

void CImmediateModeEmulator::Vertex3f(float x, float y, float z)
{
    m_CurrentVertex.x = x;
    m_CurrentVertex.y = y;
    m_CurrentVertex.z = z;
    m_Vertices.push_back(m_CurrentVertex);
}

void CImmediateModeEmulator::Vertex3fv(const float* v)
{
    Vertex3f(v[0], v[1], v[2]);
}

void CImmediateModeEmulator::RenderBatch()
{
    if (m_Vertices.empty()) return;

    std::vector<VertexPCT> renderVertices;

    // Convert GL_QUADS to triangles if needed
    if (m_PrimitiveType == GL_QUADS) {
        ConvertQuadsToTriangles(renderVertices);
    }
    else {
        renderVertices = m_Vertices;
    }

    // Create temporary VBO for this batch
    GLuint vao, vbo;
    pglGenVertexArrays(1, &vao);
    pglGenBuffers(1, &vbo);

    pglBindVertexArray(vao);
    pglBindBuffer(GL_ARRAY_BUFFER, vbo);
    pglBufferData(GL_ARRAY_BUFFER, renderVertices.size() * sizeof(VertexPCT), renderVertices.data(), GL_STREAM_DRAW);

    pglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)0);
    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)(3 * sizeof(float)));
    pglEnableVertexAttribArray(1);
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPCT), (void*)(6 * sizeof(float)));
    pglEnableVertexAttribArray(2);

    GLenum drawMode = (m_PrimitiveType == GL_QUADS) ? GL_TRIANGLES : m_PrimitiveType;
    glDrawArrays(drawMode, 0, (GLsizei)renderVertices.size());

    pglBindVertexArray(0);
    pglDeleteBuffers(1, &vbo);
    pglDeleteVertexArrays(1, &vao);
}

void CImmediateModeEmulator::ConvertQuadsToTriangles(std::vector<VertexPCT>& outVertices)
{
    outVertices.clear();
    size_t quadCount = m_Vertices.size() / 4;

    for (size_t i = 0; i < quadCount; i++) {
        size_t base = i * 4;
        // Triangle 1: 0, 1, 2
        outVertices.push_back(m_Vertices[base + 0]);
        outVertices.push_back(m_Vertices[base + 1]);
        outVertices.push_back(m_Vertices[base + 2]);
        // Triangle 2: 0, 2, 3
        outVertices.push_back(m_Vertices[base + 0]);
        outVertices.push_back(m_Vertices[base + 2]);
        outVertices.push_back(m_Vertices[base + 3]);
    }
}

// ============================================================================
// Initialization
// ============================================================================

void InitModernGL()
{
    // Load OpenGL extensions
    if (!LoadGLExtensions()) {
        MessageBox(nullptr, L"Failed to load OpenGL extensions for modern rendering!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Initialize global batch renderer with reasonable capacity
    g_EffectBatchRenderer.Initialize(65536); // 64K vertices
}

void CleanupModernGL()
{
    g_EffectBatchRenderer.Cleanup();
}
