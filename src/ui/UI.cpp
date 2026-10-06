#include "firescope/App.hpp"
#include "firescope/GL.hpp"
#include "firescope/Renderer.hpp"
#include "firescope/Scenario.hpp"
#include "CPUBackend.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

namespace firescope {
int runGui(const SimulationConfig& initialConfig, const EnvironmentParams& initialParams, bool smokeTest) {
    glfwSetErrorCallback([](int code, const char* message) { std::cerr << "GLFW " << code << ": " << message << '\n'; });
    if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1180,800,"FireScope",nullptr,nullptr);
    if (!window) { glfwTerminate(); throw std::runtime_error("Could not create OpenGL 4.1 window"); }
    glfwMakeContextCurrent(window); glfwSwapInterval(1); gl::load();
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n';
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window,true); ImGui_ImplOpenGL3_Init("#version 410 core");
    int result = 0;
    try {
        OpenGLRenderer renderer;
        std::unique_ptr<ISimulationBackend> backend = std::make_unique<CPUBackend>();
        SimulationConfig config = initialConfig;
        EnvironmentParams params = initialParams;
        InitialGrid initialGrid;
        RenderState renderState;
        Camera camera;
        bool gridLines = false;
        std::optional<TerrainHit> hover;
        auto reset = [&] {
            initialGrid = generateScenario(config,params.seed);
            backend->initialize(config,initialGrid);
            renderer.setTerrain(config,initialGrid,params.cellSizeMeters);
            backend->applyBrush(BrushTool::Ignite,config.width / 2, config.height / 2,2);
        };
        reset();
        camera.frame(config.width,config.height,renderer.terrain().centerHeight());
        bool playing = false;
        int brush = 0, radius = 2, stepsPerFrame = 1;
        uint32_t seed = params.seed;
        float windDegrees = params.windDirectionRadians * 180.0f / 3.14159265f;
        double renderMs = 0;
        int frames = 0;
        bool painting = false;
        int previousX = 0, previousY = 0;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            int fw,fh,ww,wh; glfwGetFramebufferSize(window,&fw,&fh); glfwGetWindowSize(window,&ww,&wh);
            if (ww <= 0 || wh <= 0 || fw <= 0 || fh <= 0) { glfwWaitEventsTimeout(0.05); continue; }
            ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) glfwSetWindowShouldClose(window,1);
            const float panelWidth = std::min(320.0f,static_cast<float>(ww) * 0.4f);
            ImGui::SetNextWindowPos(ImVec2(0,0)); ImGui::SetNextWindowSize(ImVec2(panelWidth,static_cast<float>(wh)));
            ImGui::Begin("FireScope",nullptr,ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
            ImGui::PushItemWidth(panelWidth * 0.48f);
            ImGui::Text("Wildfire propagation"); ImGui::Separator();
            ImGui::Text("Backend: %s",backend->name());
            const uint32_t sizes[] = {128,256,512,1024};
            std::string gridLabel = std::to_string(config.width) + " x " + std::to_string(config.height);
            if (ImGui::BeginCombo("Grid",gridLabel.c_str())) {
                for (auto size : sizes) {
                    std::string label = std::to_string(size) + " x " + std::to_string(size);
                    if (ImGui::Selectable(label.c_str(),size == config.width)) {
                        config.width = config.height = size; playing = false; reset();
                        camera.frame(config.width,config.height,renderer.terrain().centerHeight());
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::InputScalar("Seed",ImGuiDataType_U32,&seed);
            ImGui::TextDisabled("Seed applies on Reset");
            if (ImGui::Button(playing ? "Pause" : "Play")) playing = !playing;
            ImGui::SameLine(); if (ImGui::Button("Step")) backend->step(params);
            ImGui::SameLine(); if (ImGui::Button("Reset")) {
                params.seed = seed; playing = false; reset();
            }
            if (ImGui::BeginCombo("Steps/frame",std::to_string(stepsPerFrame).c_str())) {
                for (int count : {1,2,4,8})
                    if (ImGui::Selectable(std::to_string(count).c_str(),count == stepsPerFrame)) stepsPerFrame = count;
                ImGui::EndCombo();
            }
            ImGui::Separator();
            ImGui::SliderFloat("Wind m/s",&params.windSpeed,0,20,"%.1f");
            if (ImGui::SliderFloat("Direction",&windDegrees,0,360,"%.0f deg"))
                params.windDirectionRadians = windDegrees * 3.14159265f / 180.0f;
            ImGui::TextDisabled("0 east / 90 north");
            ImGui::SliderFloat("Humidity",&params.globalHumidity,0,1);
            ImGui::SliderFloat("Ignition",&params.baseIgnitionProbability,0,1);
            ImGui::SliderFloat("Moisture effect",&params.moistureStrength,0,8);
            ImGui::SliderFloat("Slope effect",&params.slopeStrength,0,8);
            ImGui::Separator();
            ImGui::RadioButton("Ignite",&brush,0); ImGui::SameLine(); ImGui::RadioButton("Firebreak",&brush,1);
            ImGui::SliderInt("Brush radius",&radius,1,30);
            ImGui::TextWrapped("Left-click or drag on terrain to paint.");
            ImGui::Separator();
            int mode = camera.mode() == CameraMode::Orbit ? 0 : 1;
            if (ImGui::RadioButton("Orbit",&mode,0)) camera.setMode(CameraMode::Orbit);
            ImGui::SameLine();
            if (ImGui::RadioButton("Free fly",&mode,1)) camera.setMode(CameraMode::FreeFly);
            if (ImGui::Button("Reset view")) camera.frame(config.width,config.height,renderer.terrain().centerHeight());
            ImGui::SameLine(); ImGui::Checkbox("Grid lines",&gridLines);
            ImGui::TextWrapped("Right-drag: rotate. Middle-drag: pan. Scroll: zoom. Free fly: WASD, Q/E down/up.");
            ImGui::Separator();
            const auto stats = backend->stats();
            ImGui::Text("Timestep: %llu",static_cast<unsigned long long>(stats.timestep));
            ImGui::Text("Burning: %llu",static_cast<unsigned long long>(stats.burning));
            ImGui::Text("Burned: %llu (%.1f%%)",static_cast<unsigned long long>(stats.burned),
                100.0 * stats.burned / (static_cast<double>(config.width)*config.height));
            ImGui::Text("Unburned: %llu",static_cast<unsigned long long>(stats.unburned));
            ImGui::Text("No fuel: %llu",static_cast<unsigned long long>(stats.noFuel));
            ImGui::Text("Simulation: %.3f ms/step",stats.simulationMs);
            ImGui::Text("Render: %.3f ms",renderMs);
            ImGui::Text("FPS: %.1f",ImGui::GetIO().Framerate);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(.4f,.8f,.45f,1),"Green: unburned");
            ImGui::TextColored(ImVec4(1,.55f,.12f,1),"Orange: burning");
            ImGui::TextDisabled("Charcoal: burned / Brown: no fuel");
            ImGui::TextWrapped("Educational prototype. Parameters are uncalibrated.");
            ImGui::PopItemWidth(); ImGui::End();

            const float left = panelWidth+12, top = 12;
            const float viewWidth = std::max(1.0f,ww-left-12), viewHeight = std::max(1.0f,wh-24.0f);
            const ImVec2 mouse = ImGui::GetMousePos();
            const float mx = mouse.x, my = mouse.y;
            const bool inView = mx >= left && mx < left+viewWidth && my >= top && my < top+viewHeight;
            const bool inputAllowed = inView && !ImGui::GetIO().WantCaptureMouse;
            const auto& io = ImGui::GetIO();
            if (inputAllowed) {
                if (ImGui::IsMouseDown(1)) camera.rotate(io.MouseDelta.x,io.MouseDelta.y);
                if (ImGui::IsMouseDown(2)) camera.pan(io.MouseDelta.x,io.MouseDelta.y);
                if (io.MouseWheel != 0) camera.zoom(io.MouseWheel);
                if (camera.mode() == CameraMode::FreeFly && !io.WantCaptureKeyboard) {
                    const float f = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_W))-ImGui::IsKeyDown(ImGuiKey_S);
                    const float r = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_D))-ImGui::IsKeyDown(ImGuiKey_A);
                    const float u = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_E))-ImGui::IsKeyDown(ImGuiKey_Q);
                    camera.move(f,r,u,io.DeltaTime);
                }
            }
            hover = inputAllowed ? renderer.terrain().intersect(camera.screenRay(mx-left,my-top,viewWidth,viewHeight)) : std::nullopt;
            if (hover && !ImGui::IsMouseDown(1) && !ImGui::IsMouseDown(2) &&
                (ImGui::IsMouseDown(0) || ImGui::IsMouseClicked(0))) {
                const int x = static_cast<int>(hover->x), y = static_cast<int>(hover->y);
                const auto tool = brush == 0 ? BrushTool::Ignite : BrushTool::Firebreak;
                if (painting && !ImGui::IsMouseClicked(0)) {
                    const int segments = std::max(std::abs(x-previousX),std::abs(y-previousY));
                    for (int segment = 1; segment <= segments; ++segment)
                        backend->applyBrush(tool,previousX+(x-previousX)*segment/segments,
                            previousY+(y-previousY)*segment/segments,radius);
                }
                backend->applyBrush(tool,x,y,radius);
                previousX = x; previousY = y; painting = ImGui::IsMouseDown(0);
            } else painting = false;
            if (hover) {
                ImGui::SetNextWindowPos(ImVec2(left+12,top+12));
                ImGui::SetNextWindowBgAlpha(0.7f);
                ImGui::Begin("Terrain cursor",nullptr,ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
                ImGui::Text("Cell %u, %u  |  Elevation %.1f m",hover->x,hover->y,hover->position.y*params.cellSizeMeters);
                ImGui::End();
            }
            if (playing) {
                backend->step(params,stepsPerFrame);
                if (!backend->stats().burning) playing = false;
            }
            if (smokeTest) backend->step(params);
            auto renderStart = std::chrono::steady_clock::now();
            backend->downloadRenderState(renderState); renderer.upload(renderState);
            glViewport(0,0,fw,fh); glClearColor(.035f,.045f,.06f,1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.draw(static_cast<int>(left*fw/ww),static_cast<int>((wh-top-viewHeight)*fh/wh),
                static_cast<int>(viewWidth*fw/ww),static_cast<int>(viewHeight*fh/wh),camera,gridLines,hover,radius);
            ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (smokeTest && glGetError() != GL_NO_ERROR) throw std::runtime_error("OpenGL error in smoke test");
            renderMs = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-renderStart).count();
            glfwSwapBuffers(window);
            if (smokeTest && ++frames >= 5) {
                std::cout << "GUI smoke test passed: 5 frames, " << backend->stats().burning << " burning cells\n";
                break;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "FireScope GUI: " << error.what() << '\n'; result = 1;
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(window); glfwTerminate(); return result;
}
}
