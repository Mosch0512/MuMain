#pragma once
// OpenGL 2 Immediate Mode Deprecation Header
// This header disables OpenGL 2 immediate mode functions to ensure all rendering uses OpenGL 4.6 compatible code
// Any remaining usage will cause compile-time errors

// Helper to generate compile errors
#define OPENGL2_DEPRECATED_ERROR(func, replacement) \
    _Pragma("error(\"" #func " is deprecated! Use " #replacement " instead\")")

// Disable immediate mode vertex specification
#define glBegin DO_NOT_USE_glBegin_USE_g_ImmediateModeEmulator_Begin_INSTEAD
#define glEnd DO_NOT_USE_glEnd_USE_g_ImmediateModeEmulator_End_INSTEAD
#define glVertex2f DO_NOT_USE_glVertex2f_USE_g_ImmediateModeEmulator_Vertex2f_INSTEAD
#define glVertex2fv DO_NOT_USE_glVertex2fv_USE_g_ImmediateModeEmulator_Vertex2fv_INSTEAD
#define glVertex3f DO_NOT_USE_glVertex3f_USE_g_ImmediateModeEmulator_Vertex3f_INSTEAD
#define glVertex3fv DO_NOT_USE_glVertex3fv_USE_g_ImmediateModeEmulator_Vertex3fv_INSTEAD

// Disable immediate mode color specification (per-vertex colors)
#define glColor3f DO_NOT_USE_glColor3f_USE_g_ImmediateModeEmulator_Color3f_OR_glColor4f_INSTEAD
#define glColor3fv DO_NOT_USE_glColor3fv_USE_g_ImmediateModeEmulator_Color3fv_OR_glColor4f_INSTEAD
#define glColor3ub DO_NOT_USE_glColor3ub_USE_glColor4f_INSTEAD
#define glColor4ub DO_NOT_USE_glColor4ub_USE_glColor4f_INSTEAD

// Disable immediate mode texture coordinate specification
#define glTexCoord2f DO_NOT_USE_glTexCoord2f_USE_g_ImmediateModeEmulator_TexCoord2f_INSTEAD
#define glTexCoord2fv DO_NOT_USE_glTexCoord2fv_USE_g_ImmediateModeEmulator_TexCoord2fv_INSTEAD

// Note: glColor4f is NOT disabled because it's used correctly for setting OpenGL state
// Note: glNormal3f is NOT disabled because it's used for lighting (not immediate mode rendering)
