#include "firescope/App.hpp"
#include "firescope/GL.hpp"
#include "firescope/Renderer.hpp"
#include "firescope/Scenario.hpp"
#include "firescope/CellInspection.hpp"
#include <cmath>
#include "firescope/BackendFactory.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

namespace firescope {
int runGui(const SimulationConfig& initialConfig, const EnvironmentParams& initialParams, bool smokeTest, const std::string& initialBackend) {
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
        auto backend = createBackend(initialBackend);
        std::string backendName = initialBackend;
        const auto backends = availableBackends();
        SimulationConfig config = initialConfig;
        EnvironmentParams params = initialParams;
        InitialGrid initialGrid;
        RenderState renderState;
        Camera camera;
        bool gridLines = false, vegetation = true, particles = true, moistureOverlay = false;
        std::optional<TerrainHit> hover;
        std::optional<ScenarioSnapshot> snapshot;
        std::optional<ComparisonResult> comparison;
        int comparisonSteps = 500;
        std::optional<std::pair<uint32_t,uint32_t>> inspected;
        GridData inspectedGrid;
        std::optional<float> lastProbability;
        bool propertiesDirty = false, forestDirty = false;
        auto reset = [&] {
            propertiesDirty = false; forestDirty = false;
            inspected.reset(); lastProbability.reset();
            snapshot.reset(); comparison.reset();
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
        float vegetationStrength = 0.65f, waterStrength = 0.25f;
        float windDegrees = params.windDirectionRadians * 180.0f / 3.14159265f;
        double renderMs = 0;
        int frames = 0;
        bool painting = false;
        int previousX = 0, previousY = 0;
        auto advance = [&](uint32_t count) {
            if (!inspected) { backend->step(params,count); return; }
            // Inspector mode deliberately reads full state; normal rendering stays compact.
            for (uint32_t i = 0; i < count; ++i) {
                backend->downloadGrid(inspectedGrid);
                lastProbability = inspectIgnition(inspectedGrid,config,params,inspected->first,inspected->second).probability;
                backend->step(params);
            }
            backend->downloadGrid(inspectedGrid);
        };
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
            if (ImGui::BeginCombo("Backend",backend->name())) {
                for (const auto& name : backends) {
                    if (ImGui::Selectable(name.c_str(),name == backendName)) {
                        backend = createBackend(name); backendName = name; playing = false; reset();
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Switching backend resets the seeded scenario.");
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
            ImGui::SameLine(); if (ImGui::Button("Step")) advance(1);
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
            ImGui::SameLine(); if (ImGui::RadioButton("Water",&brush,2)) moistureOverlay = true;
            ImGui::RadioButton("Vegetation",&brush,3); ImGui::SameLine(); ImGui::RadioButton("Inspect",&brush,4);
            ImGui::BeginDisabled(brush == 4); ImGui::SliderInt("Brush radius",&radius,1,30); ImGui::EndDisabled();
            if (brush == 2) {
                ImGui::SliderFloat("Water amount",&waterStrength,0,1);
                ImGui::TextWrapped("Water increases moisture up to 1. Existing fire keeps burning.");
            }
            if (brush == 3) {
                ImGui::SliderFloat("Forest density",&vegetationStrength,0,1);
                ImGui::TextWrapped("Sets fuel, vegetation and density. Zero clears. Burning/burned cells are preserved.");
            }
            if (brush == 4) ImGui::TextWrapped("Click a cell to pause and inspect. Step to observe its next update.");
            else ImGui::TextWrapped("Left-click or drag on terrain to paint.");
            ImGui::Separator();
            int mode = camera.mode() == CameraMode::Orbit ? 0 : 1;
            if (ImGui::RadioButton("Orbit",&mode,0)) camera.setMode(CameraMode::Orbit);
            ImGui::SameLine();
            if (ImGui::RadioButton("Free fly",&mode,1)) camera.setMode(CameraMode::FreeFly);
            if (ImGui::Button("Reset view")) camera.frame(config.width,config.height,renderer.terrain().centerHeight());
            ImGui::SameLine(); ImGui::Checkbox("Grid lines",&gridLines);
            if (ImGui::Button("Forest view")) {
                camera.setMode(CameraMode::Orbit); camera.frame(config.width,config.height,renderer.terrain().centerHeight());
                camera.rotate(0,-50); camera.zoom(5);
            }
            ImGui::Checkbox("Trees",&vegetation); ImGui::SameLine(); ImGui::Checkbox("Fire / smoke",&particles);
            ImGui::Checkbox("Moisture overlay",&moistureOverlay);
            if (moistureOverlay) ImGui::TextDisabled("Blue = moisture: 0 dry / 1 wet");
            ImGui::TextWrapped("Right-drag: rotate. Middle-drag: pan. Scroll: zoom. Free fly: WASD, Q/E down/up.");
            if (ImGui::CollapsingHeader("Scenario comparison")) {
                if (ImGui::Button("Capture")) {
                    playing = false; snapshot = captureScenario(*backend,config,params); comparison.reset();
                }
                ImGui::SameLine(); ImGui::BeginDisabled(!snapshot);
                if (ImGui::Button("Restore")) {
                    playing = false; inspected.reset(); lastProbability.reset(); config = snapshot->config; params = snapshot->params;
                    seed = params.seed; windDegrees = params.windDirectionRadians*180.0f/3.14159265f;
                    backend->initialize(config,snapshot->grid);
                    backend->restoreSnapshot(snapshot->grid,snapshot->timestep);
                    renderer.setTerrain(config,snapshot->grid,params.cellSizeMeters);
                    comparison.reset();
                }
                ImGui::EndDisabled();
                ImGui::InputInt("Run steps",&comparisonSteps); comparisonSteps = std::clamp(comparisonSteps,1,5000);
                const bool ready = snapshot && backend->stats().timestep == snapshot->timestep;
                ImGui::BeginDisabled(!ready);
                if (ImGui::Button("Demo firebreak")) {
                    playing = false;
                    for (int y = 0; y < static_cast<int>(config.height); y += 3)
                        backend->applyBrush(BrushTool::Firebreak,static_cast<int>(config.width*3/4),y,3);
                    backend->applyBrush(BrushTool::Firebreak,static_cast<int>(config.width*3/4),static_cast<int>(config.height)-1,3);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paints a north-south firebreak east of the central ignition.");
                if (ImGui::Button("Compare mitigation")) {
                    playing = false; inspected.reset(); lastProbability.reset(); GridData edited; backend->downloadGrid(edited);
                    comparison = compareMitigation(*snapshot,edited,backendName,static_cast<uint32_t>(comparisonSteps));
                    params = snapshot->params; seed = params.seed;
                    windDegrees = params.windDirectionRadians*180.0f/3.14159265f;
                    backend->restoreSnapshot(comparison->mitigatedFinal,comparison->mitigated.timestep);
                    renderer.refreshProperties(config,comparison->mitigatedFinal,true);
                }
                ImGui::EndDisabled();
                ImGui::TextWrapped("Capture, paint interventions while paused, then compare. Restore to repeat. Both runs use captured conditions.");
                if (snapshot && !ready && !comparison) ImGui::TextWrapped("Restore the captured timestep before comparing.");
                if (comparison) {
                    ImGui::Text("Baseline burned: %llu",static_cast<unsigned long long>(comparison->baseline.burned));
                    ImGui::Text("Mitigated burned: %llu",static_cast<unsigned long long>(comparison->mitigated.burned));
                    const double difference = static_cast<double>(comparison->baseline.burned)-comparison->mitigated.burned;
                    ImGui::Text("Reduction: %.1f%%",comparison->baseline.burned ? 100.0*difference/comparison->baseline.burned : 0.0);
                    ImGui::Text("Unburned: %llu",static_cast<unsigned long long>(comparison->mitigated.unburned));
                    ImGui::Text("No fuel: %llu",static_cast<unsigned long long>(comparison->mitigated.noFuel));
                    ImGui::TextWrapped("The view shows the mitigated result.");
                }
            }
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
            if (brush == 4 && hover && ImGui::IsMouseClicked(0)) {
                playing = false; inspected = {{hover->x,hover->y}}; lastProbability.reset();
                backend->downloadGrid(inspectedGrid);
            }
            if (brush != 4 && hover && !ImGui::IsMouseDown(1) && !ImGui::IsMouseDown(2) &&
                (ImGui::IsMouseDown(0) || ImGui::IsMouseClicked(0))) {
                const int x = static_cast<int>(hover->x), y = static_cast<int>(hover->y);
                const auto tool = static_cast<BrushTool>(brush);
                if (painting && !ImGui::IsMouseClicked(0)) {
                    const int segments = std::max(std::abs(x-previousX),std::abs(y-previousY));
                    for (int segment = 1; segment <= segments; ++segment)
                        backend->applyBrush(tool,previousX+(x-previousX)*segment/segments,
                            previousY+(y-previousY)*segment/segments,radius,brush == 3 ? vegetationStrength : waterStrength);
                }
                backend->applyBrush(tool,x,y,radius,brush == 3 ? vegetationStrength : waterStrength);
                if (tool == BrushTool::Water || tool == BrushTool::Vegetation) propertiesDirty = true;
                if (tool == BrushTool::Vegetation) forestDirty = true;
                previousX = x; previousY = y; painting = ImGui::IsMouseDown(0);
            } else painting = false;
            if (propertiesDirty && !ImGui::IsMouseDown(0)) {
                GridData edited; backend->downloadGrid(edited);
                renderer.refreshProperties(config,edited,forestDirty);
                if (inspected) inspectedGrid = edited;
                propertiesDirty = false; forestDirty = false;
            }
            if (hover) {
                ImGui::SetNextWindowPos(ImVec2(left+12,top+12));
                ImGui::SetNextWindowBgAlpha(0.7f);
                ImGui::Begin("Terrain cursor",nullptr,ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
                ImGui::Text("Cell %u, %u  |  Elevation %.1f m",hover->x,hover->y,hover->position.y*params.cellSizeMeters);
                ImGui::End();
            }
            if (playing) {
                advance(stepsPerFrame);
                if (!backend->stats().burning) playing = false;
            }
            if (smokeTest) advance(1);
            ImGui::SetNextWindowPos(ImVec2(left+viewWidth-162,top+12));
            ImGui::SetNextWindowBgAlpha(0.65f);
            ImGui::Begin("Wind",nullptr,ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
            ImGui::Text("Wind %.1f m/s",params.windSpeed);
            const ImVec2 origin = ImGui::GetCursorScreenPos(), center(origin.x+62,origin.y+40);
            auto* draw = ImGui::GetWindowDrawList();
            draw->AddCircle(center,27,IM_COL32(90,110,130,255));
            draw->AddText(ImVec2(center.x-4,center.y-40),IM_COL32_WHITE,"N");
            draw->AddText(ImVec2(center.x+34,center.y-7),IM_COL32_WHITE,"E");
            const ImVec2 direction(std::cos(params.windDirectionRadians),-std::sin(params.windDirectionRadians));
            const ImVec2 end(center.x+direction.x*25,center.y+direction.y*25);
            draw->AddLine(center,end,IM_COL32(130,200,255,255),2);
            draw->AddTriangleFilled(end,ImVec2(end.x-direction.x*8-direction.y*4,end.y-direction.y*8+direction.x*4),
                ImVec2(end.x-direction.x*8+direction.y*4,end.y-direction.y*8-direction.x*4),IM_COL32(130,200,255,255));
            ImGui::Dummy(ImVec2(130,80)); ImGui::End();
            if (inspected) {
                ImGui::SetNextWindowPos(ImVec2(left+12,top+65),ImGuiCond_Always);
                ImGui::SetNextWindowBgAlpha(0.85f);
                bool open = true;
                ImGui::Begin("Cell inspector",&open,ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
                const auto [x,y] = *inspected; const size_t i = static_cast<size_t>(y)*config.width+x;
                const char* states[] = {"No fuel","Unburned","Burning","Burned"};
                const auto probe = inspectIgnition(inspectedGrid,config,params,x,y);
                ImGui::Text("Cell %u, %u | step %llu",x,y,static_cast<unsigned long long>(backend->stats().timestep));
                ImGui::Text("State: %s",states[inspectedGrid.state[i]]);
                ImGui::Text("Fuel %.3f | Moisture %.3f",inspectedGrid.fuel[i],inspectedGrid.moisture[i]);
                ImGui::Text("Elevation %.1f m",inspectedGrid.elevation[i]);
                ImGui::Text("Vegetation %.3f | Density %.3f",inspectedGrid.vegetation[i],inspectedGrid.density[i]);
                ImGui::Text("Burn age: %u steps",inspectedGrid.burnTicks[i]);
                ImGui::Text("Burning neighbors: %u",probe.burningNeighbors);
                ImGui::Text("Next ignition probability: %.4f",probe.probability);
                if (lastProbability) ImGui::Text("Last observed probability: %.4f",*lastProbability);
                else ImGui::TextDisabled("Last probability: step to observe");
                if (ImGui::Button("Refresh")) backend->downloadGrid(inspectedGrid);
                ImGui::TextDisabled("Inspector readback adds overhead.");
                ImGui::End(); if (!open) { inspected.reset(); lastProbability.reset(); }
            }
            auto renderStart = std::chrono::steady_clock::now();
            backend->downloadRenderState(renderState); renderer.upload(renderState);
            glViewport(0,0,fw,fh); glClearColor(.035f,.045f,.06f,1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.draw(static_cast<int>(left*fw/ww),static_cast<int>((wh-top-viewHeight)*fh/wh),
                static_cast<int>(viewWidth*fw/ww),static_cast<int>(viewHeight*fh/wh),camera,gridLines,hover,radius,brush,params,vegetation,particles,moistureOverlay);
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
