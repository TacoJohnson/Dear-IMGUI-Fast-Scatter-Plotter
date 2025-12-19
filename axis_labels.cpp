#include "point_cloud_renderer.h"
#include "imgui.h"
#include <GL/glu.h>
#include <stdio.h>
#include <vector>

// Helper function to project 3D world coordinates to 2D screen coordinates
bool project3Dto2D(float objX, float objY, float objZ,
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
    
    // Get projection matrices
    float modelview[16], projection[16];
    int viewport[4];
    camera.getProjectionMatrices(modelview, projection, viewport, screenWidth, screenHeight);
    
    char label[32];
    float screenX, screenY;
    
    // Calculate how many labels to skip for clarity (show fewer labels if spacing is small)
    int labelSkip = 1;
    float sizeX = maxX - minX;
    float sizeZ = maxZ - minZ;
    int numLinesX = (int)(sizeX / gridSpacing) + 1;
    int numLinesZ = (int)(sizeZ / gridSpacing) + 1;
    
    // Skip labels if too many grid lines
    if (numLinesX > 15) labelSkip = 2;
    if (numLinesX > 30) labelSkip = 4;
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // X-axis labels (along bottom front edge)
    for (int i = 0; i <= numLinesX; i += labelSkip) {
        float x = minX + i * gridSpacing;
        if (x > maxX) x = maxX;
        
        if (project3Dto2D(x, minY, minZ, modelview, projection, viewport, screenX, screenY)) {
            snprintf(label, sizeof(label), "%.1f", x);
            drawList->AddText(ImVec2(screenX - 15, screenY + 5), IM_COL32(200, 200, 200, 255), label);
        }
    }
    
    // Y-axis labels (along left front edge)
    int numLinesY = (int)((maxY - minY) / gridSpacing) + 1;
    int yLabelSkip = labelSkip;
    if (numLinesY > 15) yLabelSkip = 2;
    if (numLinesY > 30) yLabelSkip = 4;
    
    for (int i = 0; i <= numLinesY; i += yLabelSkip) {
        float y = minY + i * gridSpacing;
        if (y > maxY) y = maxY;
        
        if (project3Dto2D(minX, y, minZ, modelview, projection, viewport, screenX, screenY)) {
            snprintf(label, sizeof(label), "%.1f", y);
            drawList->AddText(ImVec2(screenX - 35, screenY - 7), IM_COL32(180, 255, 180, 255), label);
        }
    }
    
    // Z-axis labels (along left bottom edge)
    for (int i = 0; i <= numLinesZ; i += labelSkip) {
        float z = minZ + i * gridSpacing;
        if (z > maxZ) z = maxZ;
        
        if (project3Dto2D(minX, minY, z, modelview, projection, viewport, screenX, screenY)) {
            snprintf(label, sizeof(label), "%.1f", z);
            drawList->AddText(ImVec2(screenX - 35, screenY + 5), IM_COL32(180, 180, 255, 255), label);
        }
    }
}
