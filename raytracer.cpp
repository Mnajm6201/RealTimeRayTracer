#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <random>
#include <memory>
#include <algorithm>

// Vector and math classes
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const { 
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); 
    }
    Vec3 normalize() const {
        float len = sqrt(x * x + y * y + z * z);
        return len > 0 ? *this * (1.0f / len) : Vec3();
    }
    Vec3 reflect(const Vec3& n) const { return *this - n * 2.0f * this->dot(n); }
    
    // Refraction using Snell's law
    Vec3 refract(const Vec3& n, float eta) const {
        float cos_i = -this->dot(n);
        float sin_t2 = eta * eta * (1.0f - cos_i * cos_i);
        if (sin_t2 >= 1.0f) return Vec3(); // Total internal reflection
        return *this * eta + n * (eta * cos_i - sqrt(1.0f - sin_t2));
    }
};

struct Color {
    float r, g, b;
    Color() : r(0), g(0), b(0) {}
    Color(float r, float g, float b) : r(r), g(g), b(b) {}
    Color operator+(const Color& c) const { return Color(r + c.r, g + c.g, b + c.b); }
    Color operator*(float s) const { return Color(r * s, g * s, b * s); }
    Color operator*(const Color& c) const { return Color(r * c.r, g * c.g, b * c.b); }
    Color clamp() const { return Color(std::min(1.0f, r), std::min(1.0f, g), std::min(1.0f, b)); }
};

// Texture class for texture mapping
struct Texture {
    std::vector<Color> data;
    int width, height;
    
    Texture(int w, int h) : width(w), height(h), data(w * h) {
        // Create checkerboard pattern
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                bool checker = ((x / 16) + (y / 16)) % 2;
                data[y * width + x] = checker ? Color(0.8f, 0.8f, 0.8f) : Color(0.2f, 0.2f, 0.2f);
            }
        }
    }
    
    Color sample(float u, float v) const {
        int x = (int)(u * width) % width;
        int y = (int)(v * height) % height;
        if (x < 0) x += width;
        if (y < 0) y += height;
        return data[y * width + x];
    }
};

struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalize()) {}
    Vec3 at(float t) const { return origin + direction * t; }
};

struct Sphere {
    Vec3 center;
    float radius;
    Color color;
    float metallic;
    float transparency;
    float refractive_index;
    Texture* texture;
    
    Sphere(const Vec3& c, float r, const Color& col, float met = 0.0f, float trans = 0.0f, 
           float ri = 1.0f, Texture* tex = nullptr)
        : center(c), radius(r), color(col), metallic(met), transparency(trans), 
          refractive_index(ri), texture(tex) {}
    
    float intersect(const Ray& ray) const {
        Vec3 oc = ray.origin - center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return -1;
        
        float t = (-b - sqrt(discriminant)) / (2.0f * a);
        return t > 0.001f ? t : -1;
    }
    
    Vec3 normal(const Vec3& point) const {
        return (point - center).normalize();
    }
    
    // Get UV coordinates for texture mapping
    void getUV(const Vec3& point, float& u, float& v) const {
        Vec3 n = normal(point);
        u = 0.5f + atan2(n.z, n.x) / (2 * M_PI);
        v = 0.5f - asin(n.y) / M_PI;
    }
    
    Color getColor(const Vec3& point) const {
        if (texture) {
            float u, v;
            getUV(point, u, v);
            return texture->sample(u, v) * color;
        }
        return color;
    }
};

class RealTimeRayTracer {
private:
    GLFWwindow* window;
    std::vector<Sphere> spheres;
    std::vector<unsigned char> frameBuffer;
    int width, height;
    
    // Camera parameters
    Vec3 camera_pos;
    Vec3 camera_target;
    float camera_angle_x, camera_angle_y;
    float camera_distance;
    
    // Animation and random sampling
    float time;
    mutable std::mt19937 rng;
    mutable std::uniform_real_distribution<float> dist;
    
    // Textures
    std::unique_ptr<Texture> checkerboard_texture;
    
    // Anti-aliasing samples per pixel
    int samples_per_pixel;
    
public:
    RealTimeRayTracer(int w, int h) : width(w), height(h), 
        camera_pos(0, 0, 5), camera_target(0, 0, 0), 
        camera_angle_x(0), camera_angle_y(0), camera_distance(5), time(0),
        rng(std::random_device{}()), dist(0.0f, 1.0f), samples_per_pixel(2) {
        
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            exit(-1);
        }
        
        // Create window
        window = glfwCreateWindow(width, height, "Real-Time Ray Tracer", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            exit(-1);
        }
        
        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        
        // Get actual framebuffer size (important for Retina displays)
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        width = fb_width;
        height = fb_height;
        
        frameBuffer.resize(width * height * 3);
        
        // Set callbacks
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
        
        // Initialize OpenGL
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            exit(-1);
        }
        
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);
        
        // Create textures
        checkerboard_texture = std::make_unique<Texture>(64, 64);
        
        // Create scene
        createScene();
    }
    
    void createScene() {
        spheres.clear();
        
        // Add spheres with different materials and textures
        spheres.push_back(Sphere(Vec3(-2, 0, -5), 1.0f, Color(0.8f, 0.2f, 0.2f), 0.9f, 0.0f, 1.0f)); // Red metallic
        spheres.push_back(Sphere(Vec3(0, 0, -5), 1.0f, Color(0.9f, 0.9f, 0.9f), 0.0f, 0.9f, 1.52f));   // Glass
        spheres.push_back(Sphere(Vec3(2, 0, -5), 1.0f, Color(0.2f, 0.2f, 0.8f), 0.0f, 0.0f, 1.0f));    // Blue diffuse
        spheres.push_back(Sphere(Vec3(0, -101, -5), 100.0f, Color(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, 1.0f, checkerboard_texture.get())); // Textured ground
    }
    
    // Sample hemisphere for global illumination
    Vec3 sampleHemisphere(const Vec3& normal) const {
        float r1 = dist(rng);
        float r2 = dist(rng);
        
        float cos_theta = sqrt(r1);
        float sin_theta = sqrt(1.0f - r1);
        float phi = 2 * M_PI * r2;
        
        Vec3 w = normal;
        Vec3 u = ((abs(w.x) > 0.1f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0)).cross(w).normalize();
        Vec3 v = w.cross(u);
        
        return u * cos(phi) * sin_theta + v * sin(phi) * sin_theta + w * cos_theta;
    }
    
    // Fresnel reflectance calculation
    float fresnel(float cos_i, float eta) const {
        float sin_t = eta * sqrt(std::max(0.0f, 1.0f - cos_i * cos_i));
        if (sin_t >= 1.0f) return 1.0f; // Total internal reflection
        
        float cos_t = sqrt(std::max(0.0f, 1.0f - sin_t * sin_t));
        float r_perp = (eta * cos_i - cos_t) / (eta * cos_i + cos_t);
        float r_parallel = (cos_i - eta * cos_t) / (cos_i + eta * cos_t);
        
        return (r_perp * r_perp + r_parallel * r_parallel) * 0.5f;
    }
    
    Color trace(const Ray& ray, int depth = 0) const {
        if (depth > 8) return Color(0.1f, 0.1f, 0.2f); // Sky color
        
        float closest_t = 1e30f;
        const Sphere* hit_sphere = nullptr;
        
        // Find closest intersection
        for (const auto& sphere : spheres) {
            float t = sphere.intersect(ray);
            if (t > 0 && t < closest_t) {
                closest_t = t;
                hit_sphere = &sphere;
            }
        }
        
        if (!hit_sphere) return Color(0.1f, 0.1f, 0.2f); // Sky
        
        Vec3 hit_point = ray.at(closest_t);
        Vec3 normal = hit_sphere->normal(hit_point);
        Color material_color = hit_sphere->getColor(hit_point);
        
        // Basic lighting
        Vec3 light_pos(sin(time) * 3, 2, cos(time) * 3 - 3);
        Vec3 light_dir = (light_pos - hit_point).normalize();
        
        // Shadow test
        Ray shadow_ray(hit_point + normal * 0.001f, light_dir);
        bool in_shadow = false;
        for (const auto& sphere : spheres) {
            if (&sphere != hit_sphere && sphere.intersect(shadow_ray) > 0) {
                in_shadow = true;
                break;
            }
        }
        
        float light_intensity = in_shadow ? 0.1f : std::max(0.1f, normal.dot(light_dir));
        Color final_color = material_color * light_intensity;
        
        // Handle reflections
        if (hit_sphere->metallic > 0.0f) {
            Vec3 reflect_dir = ray.direction.reflect(normal);
            Ray reflect_ray(hit_point + normal * 0.001f, reflect_dir);
            Color reflect_color = trace(reflect_ray, depth + 1);
            final_color = final_color * (1.0f - hit_sphere->metallic) + reflect_color * hit_sphere->metallic;
        }
        
        // Handle transparency and refraction
        if (hit_sphere->transparency > 0.0f) {
            Vec3 view_dir = ray.direction * -1.0f;
            float cos_i = view_dir.dot(normal);
            float eta = cos_i > 0 ? 1.0f / hit_sphere->refractive_index : hit_sphere->refractive_index;
            Vec3 refract_normal = cos_i > 0 ? normal : normal * -1.0f;
            
            Vec3 refract_dir = ray.direction.refract(refract_normal, eta);
            if (refract_dir.x != 0 || refract_dir.y != 0 || refract_dir.z != 0) {
                Ray refract_ray(hit_point - refract_normal * 0.001f, refract_dir);
                Color refract_color = trace(refract_ray, depth + 1);
                
                // Fresnel blend
                float fresnel_factor = fresnel(abs(cos_i), eta);
                Vec3 reflect_dir = ray.direction.reflect(normal);
                Ray reflect_ray(hit_point + normal * 0.001f, reflect_dir);
                Color reflect_color = trace(reflect_ray, depth + 1);
                
                Color transparent_color = reflect_color * fresnel_factor + refract_color * (1.0f - fresnel_factor);
                final_color = final_color * (1.0f - hit_sphere->transparency) + transparent_color * hit_sphere->transparency;
            }
        }
        
        // Global illumination
        if (depth < 3 && hit_sphere->metallic < 0.5f) {
            Vec3 random_dir = sampleHemisphere(normal);
            Ray gi_ray(hit_point + normal * 0.001f, random_dir);
            Color gi_color = trace(gi_ray, depth + 1);
            final_color = final_color + gi_color * material_color * 0.1f;
        }
        
        return final_color.clamp();
    }
    
    void render() {
        // Update camera position
        camera_pos.x = camera_distance * sin(camera_angle_x) * cos(camera_angle_y);
        camera_pos.y = camera_distance * sin(camera_angle_y);
        camera_pos.z = camera_distance * cos(camera_angle_x) * cos(camera_angle_y);
        
        // Animate spheres
        spheres[0].center.y = sin(time * 2) * 0.5f;
        spheres[1].center.x = sin(time) * 0.5f;
        spheres[2].center.z = -5 + sin(time * 1.5f) * 0.3f;
        
        // Ray trace each pixel with anti-aliasing
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color pixel_color;
                
                // Anti-aliasing: multiple samples per pixel
                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    // Random jitter for anti-aliasing
                    float jitter_x = dist(rng) - 0.5f;
                    float jitter_y = dist(rng) - 0.5f;
                    
                    float u = ((x + jitter_x) / (float)width) * 2.0f - 1.0f;
                    float v = ((y + jitter_y) / (float)height) * 2.0f - 1.0f;
                    v *= (float)height / width; 
                    
                    Vec3 ray_dir = Vec3(u, -v, -1).normalize();
                    Ray ray(camera_pos, ray_dir);
                    
                    pixel_color = pixel_color + trace(ray);
                }
                
                // Average the samples
                pixel_color = pixel_color * (1.0f / samples_per_pixel);
                
                int index = (y * width + x) * 3;
                frameBuffer[index] = (unsigned char)(pixel_color.r * 255);
                frameBuffer[index + 1] = (unsigned char)(pixel_color.g * 255);
                frameBuffer[index + 2] = (unsigned char)(pixel_color.b * 255);
            }
        }
        
        // Display the frame buffer
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());
    }
    
    void run() {
        auto last_time = std::chrono::high_resolution_clock::now();
        int frame_count = 0;
        
        std::cout << "Real-Time Ray Tracer Started!" << std::endl;
        std::cout << "Features: Reflections, Refractions, Anti-aliasing, Texture Mapping, Global Illumination" << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "- Mouse: Rotate camera" << std::endl;
        std::cout << "- Scroll: Zoom in/out" << std::endl;
        std::cout << "- WASD: Move camera" << std::endl;
        std::cout << "- Q/E: Adjust anti-aliasing quality" << std::endl;
        std::cout << "- ESC: Exit" << std::endl;
        
        while (!glfwWindowShouldClose(window)) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            time += delta_time;
            
            glClear(GL_COLOR_BUFFER_BIT);
            
            render();
            
            glfwSwapBuffers(window);
            glfwPollEvents();
            
            frame_count++;
            if (frame_count % 60 == 0) {
                std::cout << "FPS: " << (int)(60.0f / delta_time) << " | Samples: " << samples_per_pixel << "x AA | Time: " << time << "s" << std::endl;
            }
            
            last_time = current_time;
            
            // Cap framerate
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    ~RealTimeRayTracer() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    // Input callbacks
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        RealTimeRayTracer* app = static_cast<RealTimeRayTracer*>(glfwGetWindowUserPointer(window));
        
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key) {
                case GLFW_KEY_W: app->camera_angle_y += 0.1f; break;
                case GLFW_KEY_S: app->camera_angle_y -= 0.1f; break;
                case GLFW_KEY_A: app->camera_angle_x -= 0.1f; break;
                case GLFW_KEY_D: app->camera_angle_x += 0.1f; break;
                case GLFW_KEY_Q: 
                    app->samples_per_pixel = std::max(1, app->samples_per_pixel - 1);
                    std::cout << "Anti-aliasing: " << app->samples_per_pixel << "x" << std::endl;
                    break;
                case GLFW_KEY_E: 
                    app->samples_per_pixel = std::min(8, app->samples_per_pixel + 1);
                    std::cout << "Anti-aliasing: " << app->samples_per_pixel << "x" << std::endl;
                    break;
            }
        }
    }
    
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        static double last_x = xpos, last_y = ypos;
        RealTimeRayTracer* app = static_cast<RealTimeRayTracer*>(glfwGetWindowUserPointer(window));
        
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            app->camera_angle_x += (xpos - last_x) * 0.01f;
            app->camera_angle_y += (ypos - last_y) * 0.01f;
        }
        
        last_x = xpos;
        last_y = ypos;
    }
    
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        RealTimeRayTracer* app = static_cast<RealTimeRayTracer*>(glfwGetWindowUserPointer(window));
        app->camera_distance += yoffset * -0.5f;
        if (app->camera_distance < 1.0f) app->camera_distance = 1.0f;
        if (app->camera_distance > 20.0f) app->camera_distance = 20.0f;
    }
    
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        RealTimeRayTracer* app = static_cast<RealTimeRayTracer*>(glfwGetWindowUserPointer(window));
        app->width = width;
        app->height = height;
        app->frameBuffer.resize(width * height * 3);
        glViewport(0, 0, width, height);
    }
};

int main() {
    try {
        RealTimeRayTracer raytracer(1600, 1200); 
        raytracer.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}