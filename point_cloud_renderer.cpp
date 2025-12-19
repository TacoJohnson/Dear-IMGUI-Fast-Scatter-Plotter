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
    char label[32];
    
    // Calculate label skip
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    int numLinesX = (int)(sizeX / gridSpacing) + 1;
    int numLinesY = (int)(sizeY / gridSpacing) + 1;
    int numLinesZ = (int)(sizeZ / gridSpacing) + 1;
    
    int skipX = (numLinesX > 15) ? 2 : 1;
    int skipY = (numLinesY > 15) ? 2 : 1;
    int skipZ = (numLinesZ > 15) ? 2 : 1;
    if (numLinesX > 30) skipX = 4;
    if (numLinesY > 30) skipY = 4;
    if (numLinesZ > 30) skipZ = 4;
    
    // Use fixed 2D positions at viewport edges (like matplotlib)
    float padding = 10.0f;
    float labelHeight = 20.0f;
    float leftMargin = 60.0f;
    float bottomMargin = 40.0f;
    
    // Color scheme
    ImU32 xColor = IM_COL32(255, 100, 100, 255);
    ImU32 yColor = IM_COL32(100, 255, 100, 255);
    ImU32 zColor = IM_COL32(100, 100, 255, 255);
    ImU32 bgColor = IM_COL32(0, 0, 0, 200);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);
    
    // X-axis labels (bottom of screen)
    float xLabelY = screenHeight - bottomMargin / 2;
    float xAxisWidth = screenWidth - leftMargin - padding * 2;
    
    for (int i = 0; i <= numLinesX; i += skipX) {
        float x = minX + i * gridSpacing;
        if (x > maxX) x = maxX;
        
        float t = (float)i / numLinesX;  // 0 to 1
        float screenX = leftMargin + t * xAxisWidth;
        
        snprintf(label, sizeof(label), "%.1f", x);
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos(screenX - textSize.x / 2, xLabelY - textSize.y / 2);
        
        drawList->AddRectFilled(
            ImVec2(textPos.x - 3, textPos.y - 2),
            ImVec2(textPos.x + textSize.x + 3, textPos.y + textSize.y + 2),
            bgColor
        );
        drawList->AddText(textPos, xColor, label);
        
        // Small tick mark
        drawList->AddLine(
            ImVec2(screenX, xLabelY - 15),
            ImVec2(screenX, xLabelY - 10),
            xColor, 2.0f
        );
    }
    
    // X-axis label
    ImVec2 xAxisLabel = ImVec2(screenWidth / 2, screenHeight - 5);
    drawList->AddText(xAxisLabel, xColor, "X axis");
    
    // Y-axis labels (left side of screen)
    float yLabelX = leftMargin / 2;
    float yAxisHeight = screenHeight - bottomMargin - padding * 2;
    float yStart = padding;
    
    for (int i = 0; i <= numLinesY; i += skipY) {
        float y = minY + i * gridSpacing;
        if (y > maxY) y = maxY;
        
        float t = (float)i / numLinesY;  // 0 to 1
        float screenY = yStart + yAxisHeight - t * yAxisHeight;  // Inverted (0 at bottom)
        
        snprintf(label, sizeof(label), "%.1f", y);
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos(yLabelX - textSize.x / 2, screenY - textSize.y / 2);
        
        drawList->AddRectFilled(
            ImVec2(textPos.x - 3, textPos.y - 2),
            ImVec2(textPos.x + textSize.x + 3, textPos.y + textSize.y + 2),
            bgColor
        );
        drawList->AddText(textPos, yColor, label);
        
        // Small tick mark
        drawList->AddLine(
            ImVec2(leftMargin - 15, screenY),
            ImVec2(leftMargin - 10, screenY),
            yColor, 2.0f
        );
    }
    
    // Y-axis label
    drawList->AddText(ImVec2(5, screenHeight / 2), yColor, "Y axis");
    
    // Z-axis labels (right side - optional, or could be shown in legend)
    // For now, show Z range in top-right corner
    snprintf(label, sizeof(label), "Z: %.1f to %.1f", minZ, maxZ);
    ImVec2 zLabelPos(screenWidth - 150, 10);
    ImVec2 zTextSize = ImGui::CalcTextSize(label);
    drawList->AddRectFilled(
        ImVec2(zLabelPos.x - 3, zLabelPos.y - 2),
        ImVec2(zLabelPos.x + zTextSize.x + 3, zLabelPos.y + zTextSize.y + 2),
        bgColor
    );
    drawList->AddText(zLabelPos, zColor, label);
}
