#include "stdafx.h"
#include "CustomCamera3D.h"
#include "ZzzOpenglUtil.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Initialize static members
bool CCustomCamera3D::m_bEnabled = false;
float CCustomCamera3D::m_fZoomDistance = 100.0f;     // Default zoom (100 = 1.0x scale)
float CCustomCamera3D::m_fRotationAngle = 0.0f;      // Default rotation (0 degrees)
float CCustomCamera3D::m_fMinZoom = 50.0f;           // Minimum zoom (0.5x - closest)
float CCustomCamera3D::m_fMaxZoom = 200.0f;          // Maximum zoom (2.0x - farthest)
bool CCustomCamera3D::m_bRotating = false;
int CCustomCamera3D::m_iLastMouseX = 0;
bool CCustomCamera3D::m_bF9KeyPressed = false;
float CCustomCamera3D::m_fInitialCameraOffset[3] = { 0.0f, 0.0f, 0.0f };

void CCustomCamera3D::Initialize()
{
    m_bEnabled = false;
    m_fZoomDistance = 100.0f;  // Start at 100 (scale 1.0)
    m_fRotationAngle = 0.0f;
    m_fMinZoom = 50.0f;        // Min 0.5x zoom
    m_fMaxZoom = 200.0f;       // Max 2.0x zoom
    m_bRotating = false;
    m_iLastMouseX = 0;
    m_bF9KeyPressed = false;
}

void CCustomCamera3D::Toggle()
{
    m_bEnabled = !m_bEnabled;

    if (m_bEnabled)
    {
        // Start with 1.0x zoom (100 units) and zero rotation
        m_fZoomDistance = 100.0f;
        m_fRotationAngle = 0.0f;

        // Clear initial offset (will be set on first frame)
        m_fInitialCameraOffset[0] = 0.0f;
        m_fInitialCameraOffset[1] = 0.0f;
        m_fInitialCameraOffset[2] = 0.0f;
    }
    else
    {
        StopRotation();
    }
}

void CCustomCamera3D::Update()
{
    // Currently unused - reserved for future smooth transitions if needed
}

void CCustomCamera3D::ProcessMouseWheel(int delta)
{
    if (!m_bEnabled)
        return;

    // Input validation: clamp extreme delta values to prevent unexpected behavior
    const int maxDelta = 10;  // Reasonable limit for wheel delta per event
    delta = std::max(-maxDelta, std::min(maxDelta, delta));

    // Zoom speed: 10.0 units per wheel notch
    const float zoomSpeed = 10.0f;

    // delta > 0 = scroll up = zoom in (decrease distance)
    // delta < 0 = scroll down = zoom out (increase distance)
    m_fZoomDistance -= (float)delta * zoomSpeed;

    // Clamp zoom distance to min/max bounds
    m_fZoomDistance = std::max(m_fMinZoom, std::min(m_fMaxZoom, m_fZoomDistance));
}

void CCustomCamera3D::ProcessMouseRotation(int currentMouseX)
{
    if (!m_bEnabled || !m_bRotating)
        return;

    // Input validation: ensure mouse position is within reasonable screen bounds
    const int maxScreenWidth = 4096;  // Reasonable maximum for modern displays
    if (currentMouseX < -maxScreenWidth || currentMouseX > maxScreenWidth)
        return;

    // Calculate mouse movement delta
    int deltaX = currentMouseX - m_iLastMouseX;

    // Validate delta to prevent extreme jumps (e.g., from teleporting cursor)
    const int maxDeltaX = 500;  // Maximum reasonable pixel movement per frame
    if (abs(deltaX) > maxDeltaX)
    {
        // Likely a cursor jump - just update position without rotating
        m_iLastMouseX = currentMouseX;
        return;
    }

    // Only process if there's actual movement
    if (deltaX == 0)
        return;

    // Rotation speed: 1.0 degrees per pixel
    const float rotationSpeed = 1.0f;

    // Update rotation angle
    m_fRotationAngle += (float)deltaX * rotationSpeed;

    // Normalize angle to 0-360 range
    while (m_fRotationAngle >= 360.0f)
        m_fRotationAngle -= 360.0f;
    while (m_fRotationAngle < 0.0f)
        m_fRotationAngle += 360.0f;

    // Update last mouse position
    m_iLastMouseX = currentMouseX;
}

void CCustomCamera3D::StartRotation(int mouseX)
{
    if (!m_bEnabled)
        return;

    // Input validation: ensure mouse position is within reasonable bounds
    const int maxScreenWidth = 4096;
    if (mouseX < -maxScreenWidth || mouseX > maxScreenWidth)
        return;

    // Only initialize if we're not already rotating
    if (!m_bRotating)
    {
        m_bRotating = true;
        m_iLastMouseX = mouseX;
    }
}

void CCustomCamera3D::StopRotation()
{
    m_bRotating = false;
}

void CCustomCamera3D::GetModifiedCameraPosition(float inPos[3], float charPos[3], float outPos[3])
{
    if (!m_bEnabled)
    {
        outPos[0] = inPos[0];
        outPos[1] = inPos[1];
        outPos[2] = inPos[2];
        return;
    }

    // Calculate camera offset relative to character position
    float relativeX = inPos[0] - charPos[0];
    float relativeY = inPos[1] - charPos[1];
    float relativeZ = inPos[2] - charPos[2];

    // If initial offset not set yet (first frame or not rotating), save it
    if (m_fInitialCameraOffset[0] == 0.0f && m_fInitialCameraOffset[1] == 0.0f && m_fInitialCameraOffset[2] == 0.0f)
    {
        m_fInitialCameraOffset[0] = relativeX;
        m_fInitialCameraOffset[1] = relativeY;
        m_fInitialCameraOffset[2] = relativeZ;
    }

    // Calculate zoom scale from reference distance (100 units = 1.0x)
    // 50 = 0.5x (zoom in closer), 100 = 1.0x (normal), 200 = 2.0x (zoom out further)
    float zoomScale = m_fZoomDistance / 100.0f;

    // Apply zoom to the initial offset (not the current offset)
    float scaledX = m_fInitialCameraOffset[0] * zoomScale;
    float scaledY = m_fInitialCameraOffset[1] * zoomScale;
    float scaledZ = m_fInitialCameraOffset[2] * zoomScale;

    // Apply rotation around Z axis (vertical axis) to orbit around character
    // Rotate from the initial camera offset direction
    float angleRad = m_fRotationAngle * (float)M_PI / 180.0f;

    float rotatedX = scaledX * cosf(angleRad) - scaledY * sinf(angleRad);
    float rotatedY = scaledX * sinf(angleRad) + scaledY * cosf(angleRad);
    float rotatedZ = scaledZ; // Z stays the same for horizontal rotation

    // Convert back to world coordinates
    outPos[0] = charPos[0] + rotatedX;
    outPos[1] = charPos[1] + rotatedY;
    outPos[2] = charPos[2] + rotatedZ;
}

void CCustomCamera3D::GetModifiedCameraAngle(float inAngle[3], float outAngle[3])
{
    if (!m_bEnabled)
    {
        outAngle[0] = inAngle[0];
        outAngle[1] = inAngle[1];
        outAngle[2] = inAngle[2];
        return;
    }

    // Keep pitch and yaw the same
    outAngle[0] = inAngle[0];
    outAngle[1] = inAngle[1];

    // Adjust the horizontal rotation (CameraAngle[2]) by the rotation amount
    // This makes the camera look at the character while orbiting
    // Note: Inverted rotation (-) because camera view rotates opposite to position
    outAngle[2] = inAngle[2] - m_fRotationAngle;
}

