#include "point_cloud_renderer.h"
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== Camera Implementation ==========

Camera::Camera() {
    reset();
}

void Camera::reset() {
    distance = 10.0f;
    yaw = 45.0f;
    pitch = 30.0f;
    targetX = targetY = targetZ = 0.0f;
    fov = 45.0f;
}

void Camera::orbit(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    pitch += deltaPitch;
    
    // Clamp pitch to avoid gimbal lock
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void Camera::pan(float deltaX, float deltaY) {
    // Convert screen space to world space based on camera orientation
    float yawRad = yaw * M_PI / 180.0f;
    
    float right_x = cos(yawRad);
    float right_z = -sin(yawRad);
    float up_y = 1.0f;
    
    float scale = distance * 0.001f; // Scale pan speed with distance
    
    targetX += (right_x * deltaX - right_x * deltaY) * scale;
    targetY += up_y * deltaY * scale;
    targetZ += (right_z * deltaX - right_z * deltaY) * scale;
}

void Camera::zoom(float delta) {
    distance *= (1.0f - delta * 0.1f);
    if (distance < 0.1f) distance = 0.1f;
    if (distance > 1000.0f) distance = 1000.0f;
}

void Camera::applyTransform(int width, int height) {
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float aspect = (float)width / (float)height;
    float fH = tan(fov * M_PI / 360.0f) * 0.1f;
    float fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, 0.1f, 10000.0f);
    
    // Set up view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Calculate camera position from spherical coordinates
    float yawRad = yaw * M_PI / 180.0f;
    float pitchRad = pitch * M_PI / 180.0f;
    
    float camX = targetX + distance * cos(pitchRad) * sin(yawRad);
    float camY = targetY + distance * sin(pitchRad);
    float camZ = targetZ + distance * cos(pitchRad) * cos(yawRad);
    
    // Look at the target
    gluLookAt(camX, camY, camZ,      // Camera position
              targetX, targetY, targetZ,  // Look at point
              0.0f, 1.0f, 0.0f);         // Up vector
}

// CAD-style view presets
void Camera::setTopView() {
    yaw = 0.0f;
    pitch = 89.0f;  // Looking straight down
}

void Camera::setFrontView() {
    yaw = 0.0f;
    pitch = 0.0f;
}

void Camera::setSideView() {
    yaw = 90.0f;
    pitch = 0.0f;
}

void Camera::setIsometricView() {
    yaw = 45.0f;
    pitch = 35.26f;  // Classic isometric angle
}

void Camera::getProjectionMatrices(float modelview[16], float projection[16], int viewport[4], int width, int height) {
    // Apply the same transformations as in applyTransform
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    float aspect = (float)width / (float)height;
    float fH = tan(fov * M_PI / 360.0f) * 0.1f;
    float fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, 0.1f, 10000.0f);
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    float yawRad = yaw * M_PI / 180.0f;
    float pitchRad = pitch * M_PI / 180.0f;
    
    float camX = targetX + distance * cos(pitchRad) * sin(yawRad);
    float camY = targetY + distance * sin(pitchRad);
    float camZ = targetZ + distance * cos(pitchRad) * cos(yawRad);
    
    gluLookAt(camX, camY, camZ,
              targetX, targetY, targetZ,
              0.0f, 1.0f, 0.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    glPopMatrix();
    
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = width;
    viewport[3] = height;
}

// ========== PointCloudRenderer Implementation ==========

PointCloudRenderer::PointCloudRenderer() 
    : pointCount(0), pointSize(2.0f), colorMode(COLOR_RGB),
      showGrid(true), gridSpacing(1.0f), gridSize(10), showAxisLabels(true) {
    minX = minY = minZ = 0;
    maxX = maxY = maxZ = 0;
    setupOpenGL();
}

PointCloudRenderer::~PointCloudRenderer() {
    cleanupOpenGL();
}

void PointCloudRenderer::setupOpenGL() {
    // No special setup needed for immediate mode rendering
}

void PointCloudRenderer::cleanupOpenGL() {
    // No cleanup needed for immediate mode rendering
}

void PointCloudRenderer::calculateBounds() {
    if (points.empty()) {
        minX = minY = minZ = maxX = maxY = maxZ = 0;
        return;
    }
    
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = std::numeric_limits<float>::lowest();
    
    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        minZ = std::min(minZ, p.z);
        maxZ = std::max(maxZ, p.z);
    }
}

void PointCloudRenderer::setPointCloud(const std::vector<Point3D>& newPoints) {
    points = newPoints;
    pointCount = points.size();
    calculateBounds();
    
    // Auto-center camera on point cloud
    camera.targetX = (minX + maxX) * 0.5f;
    camera.targetY = (minY + maxY) * 0.5f;
    camera.targetZ = (minZ + maxZ) * 0.5f;
    
    // Set camera distance based on bounding box
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float maxSize = std::max({sizeX, sizeY, sizeZ});
    camera.distance = maxSize * 2.0f;
}

void PointCloudRenderer::clearPointCloud() {
    points.clear();
    pointCount = 0;
}

void PointCloudRenderer::renderGrid() {
    if (!showGrid) return;
    
    // Use actual point cloud bounds for the grid planes
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    
    if (sizeX == 0 || sizeY == 0 || sizeZ == 0) return; // No valid bounds
    
    // Calculate number of grid lines based on spacing
    int numLinesX = (int)(sizeX / gridSpacing) + 1;
    int numLinesY = (int)(sizeY / gridSpacing) + 1;
    int numLinesZ = (int)(sizeZ / gridSpacing) + 1;
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
    
    // ===== BOTTOM PLANE (X-Z at minY) =====
    glColor4f(0.6f, 0.6f, 0.6f, 0.7f);
    
    // Lines parallel to X axis
    for (int i = 0; i <= numLinesZ; ++i) {
        float z = minZ + i * gridSpacing;
        if (z > maxZ) z = maxZ;
        
        glVertex3f(minX, minY, z);
        glVertex3f(maxX, minY, z);
    }
    
    // Lines parallel to Z axis
    for (int i = 0; i <= numLinesX; ++i) {
        float x = minX + i * gridSpacing;
        if (x > maxX) x = maxX;
        
        glVertex3f(x, minY, minZ);
        glVertex3f(x, minY, maxZ);
    }
    
    // ===== BACK PLANE (X-Y at minZ) =====
    glColor4f(0.5f, 0.5f, 0.6f, 0.5f);
    
    // Lines parallel to X axis (horizontal)
    for (int i = 0; i <= numLinesY; ++i) {
        float y = minY + i * gridSpacing;
        if (y > maxY) y = maxY;
        
        glVertex3f(minX, y, minZ);
        glVertex3f(maxX, y, minZ);
    }
    
    // Lines parallel to Y axis (vertical)
    for (int i = 0; i <= numLinesX; ++i) {
        float x = minX + i * gridSpacing;
        if (x > maxX) x = maxX;
        
        glVertex3f(x, minY, minZ);
        glVertex3f(x, maxY, minZ);
    }
    
    // ===== LEFT PLANE (Y-Z at minX) =====
    glColor4f(0.6f, 0.5f, 0.5f, 0.5f);
    
    // Lines parallel to Z axis (horizontal)
    for (int i = 0; i <= numLinesY; ++i) {
        float y = minY + i * gridSpacing;
        if (y > maxY) y = maxY;
        
        glVertex3f(minX, y, minZ);
        glVertex3f(minX, y, maxZ);
    }
    
    // Lines parallel to Y axis (vertical)
    for (int i = 0; i <= numLinesZ; ++i) {
        float z = minZ + i * gridSpacing;
        if (z > maxZ) z = maxZ;
        
        glVertex3f(minX, minY, z);
        glVertex3f(minX, maxY, z);
    }
    
    glEnd();
    
    // Draw bounding box edges (thicker lines)
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor4f(0.3f, 0.3f, 0.3f, 0.9f);
    
    // Bottom rectangle
    glVertex3f(minX, minY, minZ); glVertex3f(maxX, minY, minZ);
    glVertex3f(maxX, minY, minZ); glVertex3f(maxX, minY, maxZ);
    glVertex3f(maxX, minY, maxZ); glVertex3f(minX, minY, maxZ);
    glVertex3f(minX, minY, maxZ); glVertex3f(minX, minY, minZ);
    
    // Vertical edges at corners
    glVertex3f(minX, minY, minZ); glVertex3f(minX, maxY, minZ);
    glVertex3f(maxX, minY, minZ); glVertex3f(maxX, maxY, minZ);
    glVertex3f(minX, minY, maxZ); glVertex3f(minX, maxY, maxZ);
    
    // Top edges (partial box)
    glVertex3f(minX, maxY, minZ); glVertex3f(maxX, maxY, minZ);
    glVertex3f(minX, maxY, minZ); glVertex3f(minX, maxY, maxZ);
    
    glEnd();
    
    // Draw colored coordinate axes at origin corner
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    
    float axisLength = std::min({sizeX, sizeY, sizeZ}) * 0.15f;
    
    // X axis (red) - along bottom edge
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f(minX, minY, minZ);
    glVertex3f(minX + axisLength, minY, minZ);
    
    // Y axis (green) - vertical
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glVertex3f(minX, minY, minZ);
    glVertex3f(minX, minY + axisLength, minZ);
    
    // Z axis (blue) - along back edge
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glVertex3f(minX, minY, minZ);
    glVertex3f(minX, minY, minZ + axisLength);
    
    glEnd();
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
}

void PointCloudRenderer::render(int width, int height) {
    // Apply camera transformation first
    camera.applyTransform(width, height);
    
    // Render grid first (underneath points)
    renderGrid();
    
    // Then render points
    if (pointCount == 0) return;
    
    // Enable point rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(pointSize);
    
    // Render points using immediate mode (compatible with all OpenGL versions)
    glBegin(GL_POINTS);
    
    for (const auto& p : points) {
        // Set color based on mode
        switch (colorMode) {
            case COLOR_RGB:
                glColor3f(p.r, p.g, p.b);
                break;
            case COLOR_HEIGHT: {
                // Color by height (Y axis)
                float t = (p.y - minY) / (maxY - minY);
                // Gradient: blue (low) -> green -> red (high)
                glColor3f(t, 1.0f - fabs(t - 0.5f) * 2.0f, 1.0f - t);
                break;
            }
            case COLOR_INTENSITY:
                glColor3f(p.intensity, p.intensity, p.intensity);
                break;
            case COLOR_UNIFORM:
                glColor3f(1.0f, 1.0f, 1.0f);
                break;
        }
        
        // Set vertex position
        glVertex3f(p.x, p.y, p.z);
    }
    
    glEnd();
    
    glDisable(GL_POINT_SMOOTH);
}

// Axis label rendering implementation (using ImGui overlays)
#include "imgui.h"
#include <GL/glu.h>

// Helper function to project 3D world coordinates to 2D screen coordinates
static bool project3Dto2D(float objX, float objY, float objZ,
                   const float modelview[16], const float projection[16], const int viewport[4],
                   float& winX, float& winY) {
    GLdouble wx, wy, wz;
    GLint result = gluProject(objX, objY, objZ,
                              (const GLdouble*)modelview,
                              (const GLdouble*)projection,
                              viewport,
                              &wx, &wy, &wz);
    
    if (result == GL_TRUE && wz >= 0.0 && wz <= 1.0) {
        winX = (float)wx;
        winY = viewport[3] - (float)wy; // Flip Y for screen coordinates
        return true;
    }
    return false;
}

void PointCloudRenderer::renderAxisLabels(int screenWidth, int screenHeight) {
    if (!showAxisLabels || !showGrid) return;
    if (maxX == minX || maxY == minY || maxZ == minZ) return;
    
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Get projection matrices
    float modelview[16], projection[16];
    int viewport[4];
    camera.getProjectionMatrices(modelview, projection, viewport, screenWidth, screenHeight);
    
    char label[32];
    
    // Calculate grid parameters
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    int numLinesX = (int)(sizeX / gridSpacing) + 1;
    int numLinesY = (int)(sizeY / gridSpacing) + 1;
    int numLinesZ = (int)(sizeZ / gridSpacing) + 1;
    
    // Adaptive skip
    int skipX = (numLinesX > 10) ? 2 : 1;
    int skipY = (numLinesY > 10) ? 2 : 1;
    int skipZ = (numLinesZ > 10) ? 2 : 1;
    
    // Helper to draw label at 3D position
    auto drawAt = [&](float wx, float wy, float wz, const char* text, ImU32 color) {
        GLdouble sx, sy, sz;
        if (gluProject(wx, wy, wz,
                      (const GLdouble*)modelview,
                      (const GLdouble*)projection,
                      viewport,
                      &sx, &sy, &sz) == GL_TRUE && sz >= 0.0 && sz <= 1.0) {
            
            float fx = (float)sx;
            float fy = viewport[3] - (float)sy;
            
            ImVec2 tsize = ImGui::CalcTextSize(text);
            ImVec2 tpos(fx - tsize.x/2, fy - tsize.y/2);
            
            drawList->AddRectFilled(
                ImVec2(tpos.x-2, tpos.y-2),
                ImVec2(tpos.x+tsize.x+2, tpos.y+tsize.y+2),
                IM_COL32(0,0,0,220)
            );
            drawList->AddText(tpos, color, text);
        }
    };
    
    // X-axis labels along bottom edge
    for (int i = 0; i <= numLinesX; i += skipX) {
        float x = minX + i * gridSpacing;
        if (x > maxX) x = maxX;
        snprintf(label, sizeof(label), "%.1f", x);
        drawAt(x, minY, minZ, label, IM_COL32(255,100,100,255));
    }
    
    // Y-axis labels along left edge
    for (int i = 0; i <= numLinesY; i += skipY) {
        float y = minY + i * gridSpacing;
        if (y > maxY) y = maxY;
        snprintf(label, sizeof(label), "%.1f", y);
        drawAt(minX, y, minZ, label, IM_COL32(100,255,100,255));
    }
    
    // Z-axis labels along back edge
    for (int i = 0; i <= numLinesZ; i += skipZ) {
        float z = minZ + i * gridSpacing;
        if (z > maxZ) z = maxZ;
        snprintf(label, sizeof(label), "%.1f", z);
        drawAt(minX, minY, z, label, IM_COL32(100,100,255,255));
    }
}
