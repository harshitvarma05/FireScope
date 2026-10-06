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
    GLFWwindow* window = glfwCreateWindow(1180,800,"FireScope — CPU milestone",nullptr,nullptr);
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
        auto reset = [&] {
            initialGrid = generateScenario(config,params.seed);
            backend->initialize(config,initialGrid);
            backend->applyBrush(BrushTool::Ignite,config.width / 2, config.height / 2,2);
        };
        reset();
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
            if (glfwGetKey(window,GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window,1);
            int fw,fh,ww,wh; glfwGetFramebufferSize(window,&fw,&fh); glfwGetWindowSize(window,&ww,&wh);
            if (ww <= 0 || wh <= 0 || fw <= 0 || fh <= 0) { glfwWaitEventsTimeout(0.05); continue; }
            ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
            const float panelWidth = std::min(290.0f,static_cast<float>(ww) * 0.4f);
            ImGui::SetNextWindowPos(ImVec2(0,0)); ImGui::SetNextWindowSize(ImVec2(panelWidth,static_cast<float>(wh)));
            ImGui::Begin("FireScope",nullptr,ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
            ImGui::PushItemWidth(panelWidth * 0.48f);
            ImGui::Text("CPU reference milestone"); ImGui::Separator();
            ImGui::Text("Backend: %s",backend->name());
            ImGui::TextDisabled("CUDA / Metal: later milestones");
            const uint32_t sizes[] = {128,256,512,1024};
            std::string gridLabel = std::to_string(config.width) + " x " + std::to_string(config.height);
            if (ImGui::BeginCombo("Grid",gridLabel.c_str())) {
                for (auto size : sizes) {
                    std::string label = std::to_string(size) + " x " + std::to_string(size);
                    if (ImGui::Selectable(label.c_str(),size == config.width)) {
                        config.width = config.height = size; playing = false; reset();
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
            ImGui::TextWrapped("Click or drag on the grid to paint. Reset restores the same seeded forest and central ignition.");
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

            const int side = std::max(1,std::min(static_cast<int>(ww-panelWidth)-32,wh-32));
            const float left = panelWidth + (ww-panelWidth-side)*0.5f;
            const float top = (wh-side)*0.5f;
            const ImVec2 mouse = ImGui::GetMousePos();
            const float mx = mouse.x, my = mouse.y;
            if (!ImGui::GetIO().WantCaptureMouse && (ImGui::IsMouseDown(0) || ImGui::IsMouseClicked(0)) &&
                mx >= left && mx < left+side && my >= top && my < top+side) {
                int x = static_cast<int>((mx-left)/side*config.width);
                int y = static_cast<int>((top+side-my)/side*config.height);
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
            if (playing) {
                backend->step(params,stepsPerFrame);
                if (!backend->stats().burning) playing = false;
            }
            if (smokeTest) backend->step(params);
            auto renderStart = std::chrono::steady_clock::now();
            backend->downloadRenderState(renderState); renderer.upload(renderState);
            glViewport(0,0,fw,fh); glClearColor(.035f,.045f,.06f,1); glClear(GL_COLOR_BUFFER_BIT);
            renderer.draw(static_cast<int>(left*fw/ww),static_cast<int>((wh-top-side)*fh/wh),side*fw/ww,side*fh/wh);
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
