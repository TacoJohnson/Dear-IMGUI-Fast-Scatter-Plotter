#pragma once

#include <vector>
#include <string>
#include <GL/gl.h>
#include <GL/glu.h>

// Structure to represent a single point in 3D space
struct Point3D {
    float x, y, z;        // Position
    float r, g, b;        // Color (0-1 range)
    float intensity;      // Optional intensity value
    
    Point3D(float x = 0, float y = 0, float z = 0, 
            float r = 1, float g = 1, float b = 1, float intensity = 1.0f)
        : x(x), y(y), z(z), r(r), g(g), b(b), intensity(intensity) {}
};

// Camera class for 3D navigation
class Camera {
public:
    float distance;       // Distance from target
    float yaw;           // Rotation around Y axis
    float pitch;         // Rotation around X axis
    float targetX, targetY, targetZ;  // Look-at target
    float fov;           // Field of view
    
    Camera();
    void reset();
    void orbit(float deltaYaw, float deltaPitch);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);
    void applyTransform(int width, int height);
    
    // CAD-style view presets
    void setTopView();
    void setFrontView();
    void setSideView();
    void setIsometricView();
    
    // Get current projection info for 3D->2D transforms
    void getProjectionMatrices(float modelview[16], float projection[16], int viewport[4], int width, int height);
};

// High-performance point cloud renderer using OpenGL VBOs
class PointCloudRenderer {
public:
    PointCloudRenderer();
    ~PointCloudRenderer();
    
    // Load point cloud data
    void setPointCloud(const std::vector<Point3D>& points);
    void clearPointCloud();
    
    // Rendering
    void render(int width, int height);
    
    // Configuration
    void setPointSize(float size) { pointSize = size; }
    float getPointSize() const { return pointSize; }
    
    void setColorMode(int mode) { colorMode = mode; }
    int getColorMode() const { return colorMode; }
    
    // Camera access
    Camera& getCamera() { return camera; }
    
    // Statistics
    size_t getPointCount() const { return pointCount; }
    
    // Grid settings
    void setShowGrid(bool show) { showGrid = show; }
    bool getShowGrid() const { return showGrid; }
    
    void setGridSpacing(float spacing) { gridSpacing = spacing; }
    float getGridSpacing() const { return gridSpacing; }
    
    void setGridSize(int size) { gridSize = size; }
    int getGridSize() const { return gridSize; }
    
    void setShowAxisLabels(bool show) { showAxisLabels = show; }
    bool getShowAxisLabels() const { return showAxisLabels; }
    
    void renderGrid();
    void renderAxisLabels(int screenWidth, int screenHeight);
    
    // Color modes
    enum ColorMode {
        COLOR_RGB = 0,
        COLOR_HEIGHT = 1,
        COLOR_INTENSITY = 2,
        COLOR_UNIFORM = 3
    };
    
private:
    void setupOpenGL();
    void cleanupOpenGL();
    
    size_t pointCount;
    float pointSize;
    int colorMode;
    
    // Grid settings
    bool showGrid;
    float gridSpacing;
    int gridSize;
    bool showAxisLabels;
    
    Camera camera;
    
    std::vector<Point3D> points;
    
    // Bounding box for auto-scaling
    float minX, maxX, minY, maxY, minZ, maxZ;
    void calculateBounds();
};
