#include <iostream>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "ogl_resource.hpp"
#include "error_handling.hpp"
#include "window.hpp"
#include "shader.hpp"
#include "scene_definition.hpp"
#include "renderer.hpp"
#include "ogl_geometry_factory.hpp"
#include "ogl_material_factory.hpp"

void toggle(const std::string& aToggleName, bool& aToggleValue) {
    aToggleValue = !aToggleValue;
    std::cout << aToggleName << ": "
        << (aToggleValue ? "ON\n" : "OFF\n");
}

struct Config {
    int     currentSceneIdx = 0;
    bool    showSolid = true;
    bool    showWireframe = false;
    bool    useZOffset = false;

    glm::vec2 focusUV = glm::vec2(0.5f, 0.5f);  // normalized mouse
    float     focusRange = 0.1f;                   // ±10% of depth span
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    try {
        Window window;
        MouseTracking mouseTracking;
        Config config;

        // Camera
        Camera camera(window.aspectRatio());
        camera.setPosition({ 0.0f, 10.0f, 50.0f });
        camera.lookAt({ 0.0f,  0.0f,  0.0f });

        // Light
        SpotLight light;
        light.setPosition({ 25.0f, 40.0f, 30.0f });
        light.lookAt({ 0.0f,  0.0f,  0.0f });

        // 1) Poll the mouse each frame to update config.focusUV
        window.onCheckInput([&](GLFWwindow* w) {
            // orbit camera if left button is down
            mouseTracking.update(w);
            if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                camera.orbit(-0.4f * mouseTracking.offset(), { 0,0,0 });
            }

            // read current cursor pos
            double mx, my;
            glfwGetCursorPos(w, &mx, &my);
            config.focusUV.x = static_cast<float>(mx) / window.size()[0];
            config.focusUV.y = 1.0f - static_cast<float>(my) / window.size()[1];
            });

        // 2) Tweak focusRange with C/V
        window.setKeyCallback([&](GLFWwindow*, int key, int, int action, int) {
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                switch (key) {
                case GLFW_KEY_C:
                    config.focusRange = std::max(0.01f, config.focusRange - 0.01f);
                    std::cout << "focusRange = " << config.focusRange << "\n";
                    break;
                case GLFW_KEY_V:
                    config.focusRange += 0.01f;
                    std::cout << "focusRange = " << config.focusRange << "\n";
                    break;
                }
            }
            });

        // 3) Load your resources
        OGLMaterialFactory materialFactory;
        materialFactory.loadShadersFromDir("./shaders/");
        materialFactory.loadTexturesFromDir("./data/textures/");

        // Fix: declare an object, not a function
        OGLGeometryFactory geometryFactory;

        std::array<SimpleScene, 1> scenes{ {
            createCottageScene(materialFactory, geometryFactory)
        } };

        // 4) Set up renderer & resize callback
        Renderer renderer(materialFactory);
        window.onResize([&](int w, int h) {
            camera.setAspectRatio(window.aspectRatio());
            renderer.initialize(w, h);
            });
        renderer.initialize(window.size()[0], window.size()[1]);

        // 5) Main loop: shadow, geometry, composite, blur, DoF
        window.runLoop([&] {
            renderer.shadowMapPass(scenes[config.currentSceneIdx], light);
            renderer.clear();
            renderer.geometryPass(scenes[config.currentSceneIdx], camera, RenderOptions{ "solid" });
            renderer.compositingPass(light);
            renderer.blurPass();
            // ← Pass the vec2 then the float
            renderer.dofPass(config.focusUV, config.focusRange);
            });

    }
    catch (ShaderCompilationError& exc) {
        std::cerr << "Shader compilation error!\n"
            << "Shader type: " << exc.shaderTypeName()
            << "\nError: " << exc.what() << "\n";
        glfwTerminate();
        return -3;
    }
    catch (OpenGLError& exc) {
        std::cerr << "OpenGL error: " << exc.what() << "\n";
        glfwTerminate();
        return -2;
    }
    catch (std::exception& exc) {
        std::cerr << "Error: " << exc.what() << "\n";
        glfwTerminate();
        return -1;
    }

    glfwTerminate();
    return 0;
}
