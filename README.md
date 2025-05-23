# Ray Tracer

Real-time 3D ray tracing engine with reflections, refractions, and realistic lighting.

## Features
- Mirror reflections on metallic surfaces
- Glass/water transparency with light bending
- Anti-aliasing for smooth edges
- Ray-traced shadows
- Texture mapping
- Global illumination

## Install
**Mac:** `brew install glfw glew`  
**Linux:** `sudo apt install libglfw3-dev libglew-dev libgl1-mesa-dev`  
**Windows:** Use MSYS2 + `pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew`

## Run
```bash
make && make run
```

## Controls
- Mouse: Rotate camera
- Scroll: Zoom
- WASD: Move camera
- Q/E: Adjust quality
- ESC: Exit

Scene has 3 spheres (metal, glass, blue) on checkered floor with moving light.
