// LIDAR Point Cloud Viewer with Dear ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "point_cloud_renderer.h"
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <random>
#include <cmath>

// Generate sample LIDAR point cloud data
std::vector<Point3D> generateSampleLidarData(int numPoints) {
    std::vector<Point3D> points;
    points.reserve(numPoints);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    // Generate a synthetic scene with multiple objects
    for (int i = 0; i < numPoints; ++i) {
        float t = (float)i / numPoints;
        float angle = t * 2.0f * M_PI * 10.0f; // Multiple spirals
        float radius = dis(gen) * 5.0f;
        float height = sin(angle) * 2.0f + dis(gen) * 0.5f;
        
        float x = cos(angle) * radius;
        float y = height;
        float z = sin(angle) * radius;
        
        // Color based on position
        float r = (x + 5.0f) / 10.0f;
        float g = (y + 3.0f) / 6.0f;
        float b = (z + 5.0f) / 10.0f;
        
        points.emplace_back(x, y, z, r, g, b, dis(gen));
    }
    
    return points;
}

// Mouse interaction state
struct MouseState {
    bool leftPressed = false;
    bool rightPressed = false;
    bool middlePressed = false;
    double lastX = 0, lastY = 0;
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window
    GLFWwindow* window = glfwCreateWindow(1600, 900, "LIDAR Point Cloud Viewer", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Create point cloud renderer
    PointCloudRenderer renderer;
    
    // Generate initial sample data
    int numPoints = 100000;
    auto points = generateSampleLidarData(numPoints);
    renderer.setPointCloud(points);

    // UI State
    float pointSize = renderer.getPointSize();
    int colorMode = renderer.getColorMode();
    const char* colorModeNames[] = { "RGB Colors", "Height Map", "Intensity", "Uniform White" };
    
    bool showDemoWindow = false;
    bool showControlPanel = true;
    bool showStats = true;
    
    MouseState mouse;
    ImVec4 clearColor = ImVec4(0.1f, 0.1f, 0.15f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle mouse input for 3D camera control (when not over ImGui)
        if (!io.WantCaptureMouse) {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                if (mouse.leftPressed) {
                    float dx = (float)(mouseX - mouse.lastX);
                    float dy = (float)(mouseY - mouse.lastY);
                    renderer.getCamera().orbit(dx * 0.5f, -dy * 0.5f);
                }
                mouse.leftPressed = true;
            } else {
                mouse.leftPressed = false;
            }
            
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                if (mouse.rightPressed) {
                    float dx = (float)(mouseX - mouse.lastX);
                    float dy = (float)(mouseY - mouse.lastY);
                    renderer.getCamera().pan(dx, dy);
                }
                mouse.rightPressed = true;
            } else {
                mouse.rightPressed = false;
            }
            
            mouse.lastX = mouseX;
            mouse.lastY = mouseY;
        }
        
        // Handle scroll for zoom
        if (!io.WantCaptureMouse && io.MouseWheel != 0) {
            renderer.getCamera().zoom(io.MouseWheel);
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Control Panel
        if (showControlPanel) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Control Panel", &showControlPanel);
            
            ImGui::Text("LIDAR Point Cloud Viewer");
            ImGui::Separator();
            
            // Point cloud info
            ImGui::Text("Points: %zu", renderer.getPointCount());
            ImGui::Spacing();
            
            // Point size control
            if (ImGui::SliderFloat("Point Size", &pointSize, 0.1f, 10.0f)) {
                renderer.setPointSize(pointSize);
            }
            
            // Color mode selection
            if (ImGui::Combo("Color Mode", &colorMode, colorModeNames, 4)) {
                renderer.setColorMode(colorMode);
                // Immediate mode renders colors on-the-fly, no need to regenerate
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Grid Settings");
            
            bool showGrid = renderer.getShowGrid();
            if (ImGui::Checkbox("Show Grid", &showGrid)) {
                renderer.setShowGrid(showGrid);
            }
            
            if (showGrid) {
                float gridSpacing = renderer.getGridSpacing();
                if (ImGui::SliderFloat("Grid Spacing", &gridSpacing, 0.1f, 10.0f, "%.1f units")) {
                    renderer.setGridSpacing(gridSpacing);
                }
                
                bool showAxisLabels = renderer.getShowAxisLabels();
                if (ImGui::Checkbox("Show Axis Labels", &showAxisLabels)) {
                    renderer.setShowAxisLabels(showAxisLabels);
                }
                
                ImGui::Text("Grid helps visualize scale");
                ImGui::Text("X-axis: Red, Y: Green, Z: Blue");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Point Cloud Generation");
            
            // Generate new data
            if (ImGui::SliderInt("Num Points", &numPoints, 1000, 1000000)) {
                // Update when slider is released
            }
            
            if (ImGui::Button("Generate New Cloud", ImVec2(-1, 0))) {
                points = generateSampleLidarData(numPoints);
                renderer.setPointCloud(points);
            }
            
            if (ImGui::Button("Clear Point Cloud", ImVec2(-1, 0))) {
                renderer.clearPointCloud();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Camera Controls");
            
            if (ImGui::Button("Reset Camera", ImVec2(-1, 0))) {
                renderer.getCamera().reset();
                renderer.setPointCloud(points); // Re-center
            }
            
            ImGui::Text("View Presets:");
            
            // CAD-style view buttons in a 2x2 grid
            if (ImGui::Button("Top", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0))) {
                renderer.getCamera().setTopView();
            }
            ImGui::SameLine();
            if (ImGui::Button("Front", ImVec2(-1, 0))) {
                renderer.getCamera().setFrontView();
            }
            
            if (ImGui::Button("Side", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0))) {
                renderer.getCamera().setSideView();
            }
            ImGui::SameLine();
            if (ImGui::Button("Isometric", ImVec2(-1, 0))) {
                renderer.getCamera().setIsometricView();
            }
            
            ImGui::Spacing();
            ImGui::Text("Left Mouse: Rotate");
            ImGui::Text("Right Mouse: Pan");
            ImGui::Text("Scroll Wheel: Zoom");
            
            ImGui::Spacing();
            ImGui::Separator();
            
            ImGui::Checkbox("Show Demo Window", &showDemoWindow);
            ImGui::Checkbox("Show Statistics", &showStats);
            
            ImGui::ColorEdit3("Background", (float*)&clearColor);
            
            ImGui::End();
        }

        // Statistics overlay
        if (showStats) {
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 250, 10), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::Begin("Statistics", &showStats, 
                        ImGuiWindowFlags_NoDecoration | 
                        ImGuiWindowFlags_AlwaysAutoResize | 
                        ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoFocusOnAppearing | 
                        ImGuiWindowFlags_NoNav);
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
            ImGui::Text("Points: %zu", renderer.getPointCount());
            ImGui::End();
        }

        // Demo window
        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        // Get display size for axis labels
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        // Render axis labels BEFORE ImGui::Render() (during frame building)
        renderer.renderAxisLabels(display_w, display_h);

        // Rendering
        ImGui::Render();
        
        // Clear and render 3D scene
        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render point cloud
        renderer.render(display_w, display_h);
        
        // Render ImGui on top
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
