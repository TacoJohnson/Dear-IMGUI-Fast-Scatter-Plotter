# Dear ImGui Example

A simple example application demonstrating Dear ImGui with GLFW and OpenGL3 backend.

## Features

- ImGui demo window with comprehensive widget showcase
- Custom window with various widgets:
  - Checkboxes
  - Sliders
  - Color picker
  - Text input
  - Buttons
- FPS counter
- Clean shutdown and resource management

## Prerequisites

### Windows
- Visual Studio 2019 or later (with C++ development tools)
- CMake 3.10 or later
- GLFW (can be installed via vcpkg)

To install GLFW with vcpkg:
```bash
vcpkg install glfw3:x64-windows
```

### Linux
```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential libglfw3-dev

# Fedora
sudo dnf install cmake gcc-c++ glfw-devel

# Arch
sudo pacman -S cmake gcc glfw-wayland
```

### macOS
```bash
brew install cmake glfw
```

## Setup

1. **Clone Dear ImGui**
   
   Download Dear ImGui into the project directory:
   ```bash
   cd imgui_example
   git clone https://github.com/ocornut/imgui.git
   ```

2. **Build the project**

   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

   On Windows with Visual Studio:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

3. **Run the application**

   ```bash
   # Linux/macOS
   ./imgui_example

   # Windows
   .\Release\imgui_example.exe
   ```

## Project Structure

```
imgui_example/
├── main.cpp           # Main application code
├── CMakeLists.txt     # CMake build configuration
├── README.md          # This file
└── imgui/             # Dear ImGui library (clone from GitHub)
    ├── imgui.cpp
    ├── imgui.h
    ├── backends/
    │   ├── imgui_impl_glfw.cpp
    │   ├── imgui_impl_glfw.h
    │   ├── imgui_impl_opengl3.cpp
    │   └── imgui_impl_opengl3.h
    └── ...
```

## Code Overview

The example demonstrates:
- **Initialization**: Setting up GLFW, OpenGL, and ImGui contexts
- **Main Loop**: Handling events, updating UI, and rendering
- **Dear ImGui Widgets**: Various UI elements like buttons, sliders, and text inputs
- **Cleanup**: Proper resource deallocation

## Customization

- Change the **style** by modifying `ImGui::StyleColorsDark()` to `ImGui::StyleColorsLight()` or create your own
- Add **more widgets** by exploring the demo window code
- Modify the **window size** in `glfwCreateWindow(1280, 720, ...)`
- Adjust the **background color** using the color picker in the UI

## Resources

- [Dear ImGui GitHub](https://github.com/ocornut/imgui)
- [Dear ImGui Documentation](https://github.com/ocornut/imgui/wiki)
- [GLFW Documentation](https://www.glfw.org/documentation.html)

## License

This example code is in the public domain. Dear ImGui itself is licensed under the MIT License.
