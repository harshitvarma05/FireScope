#include "firescope/GL.hpp"
#include "firescope/App.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <string>

int firescope::runTriangle(bool smoke) {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    auto* window = glfwCreateWindow(1100, 760, "FireScope", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n';
    glfwSwapInterval(1);
    firescope::gl::load();
    namespace gl = firescope::gl;
    const char* vertex = "#version 410 core\nlayout(location=0) in vec2 p; void main(){gl_Position=vec4(p,0,1);}";
    const char* fragment = "#version 410 core\nout vec4 color; void main(){color=vec4(1,.35,.08,1);}";
    unsigned vs = gl::CreateShader(0x8B31), fs = gl::CreateShader(0x8B30);
    gl::ShaderSource(vs, 1, &vertex, nullptr); gl::CompileShader(vs);
    gl::ShaderSource(fs, 1, &fragment, nullptr); gl::CompileShader(fs);
    unsigned program = gl::CreateProgram();
    gl::AttachShader(program, vs); gl::AttachShader(program, fs); gl::LinkProgram(program);
    gl::DeleteShader(vs); gl::DeleteShader(fs);
    float points[] = {-.5f,-.5f,.5f,-.5f,0,.5f};
    unsigned vao, vbo;
    gl::GenVertexArrays(1,&vao); gl::BindVertexArray(vao);
    gl::GenBuffers(1,&vbo); gl::BindBuffer(0x8892,vbo);
    gl::BufferData(0x8892,sizeof(points),points,0x88E4);
    gl::VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);
    gl::EnableVertexAttribArray(0);
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::GetIO().IniFilename = nullptr; ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window,true); ImGui_ImplOpenGL3_Init("#version 410 core");
    int frames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); int w,h; glfwGetFramebufferSize(window,&w,&h);
        glViewport(0,0,w,h); glClearColor(.04f,.06f,.08f,1); glClear(GL_COLOR_BUFFER_BIT);
        gl::UseProgram(program); gl::BindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,3);
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        ImGui::Begin("FireScope"); ImGui::Text("Backend: CPU"); ImGui::Text("Grid: 128");
        ImGui::Button("Play"); ImGui::SameLine(); ImGui::Button("Reset"); ImGui::End();
        ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        if (smoke && ++frames >= 3) break;
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    gl::DeleteBuffers(1,&vbo); gl::DeleteVertexArrays(1,&vao); gl::DeleteProgram(program);
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
