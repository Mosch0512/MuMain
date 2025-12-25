#pragma once

// Custom 3D Camera System
// Toggle with F9 key
// - Mouse wheel: Zoom in/out
// - Middle mouse button + move: Rotate around character

// Shared camera height offset constant
// Used by both default camera and 3D camera to ensure consistent framing
// This value represents the vertical offset added to the camera target point
// to focus on the character's instead of their feet
constexpr float CAMERA_HEIGHT_OFFSET = 80.0f;

class CCustomCamera3D
{
private:
    static bool m_bEnabled;              // Is 3D camera mode active?
    static float m_fZoomDistance;        // Current zoom distance from character
    static float m_fRotationAngle;       // Camera rotation angle around character (in degrees)
    static float m_fMinZoom;             // Minimum zoom distance (closest)
    static float m_fMaxZoom;             // Maximum zoom distance (farthest)
    static bool m_bRotating;             // Is middle mouse button held for rotation?
    static int m_iLastMouseX;            // Last mouse X position for rotation calculation
    static bool m_bF9KeyPressed;         // To prevent toggle spam
    static float m_fInitialCameraOffset[3]; // Initial camera offset from character (saved on first frame)

public:
    // Initialize default values
    static void Initialize();

    // Toggle camera mode on/off (F9 key)
    static void Toggle();

    // Check if 3D camera is enabled
    static bool IsEnabled() { return m_bEnabled; }

    // Update camera state every frame
    static void Update();

    // Process mouse wheel for zooming
    static void ProcessMouseWheel(int delta);

    // Process middle mouse button for rotation
    static void ProcessMouseRotation(int currentMouseX);

    // Start rotation when middle mouse is pressed
    static void StartRotation(int mouseX);

    // Stop rotation when middle mouse is released
    static void StopRotation();

    // Get modified camera position for zoom/rotation
    static void GetModifiedCameraPosition(float inPos[3], float charPos[3], float outPos[3]);

    // Get modified camera angle for rotation
    static void GetModifiedCameraAngle(float inAngle[3], float outAngle[3]);

    // Get current zoom distance
    static float GetZoomDistance() { return m_fZoomDistance; }

    // Get current rotation angle
    static float GetRotationAngle() { return m_fRotationAngle; }
};
