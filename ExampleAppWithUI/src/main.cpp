#include "../../ProfilerLib/profilerlib.hpp"

#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>

#include "./utils/hwinfo.hpp"
#include "./utils/threading.hpp"
#include "app.hpp"

void errorCallback(int errorCode, const char* description);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseBtnCallback(GLFWwindow* window, int key, int action, int mods);
void scrollCallback(GLFWwindow* window, double dx, double dy);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

int initGLFW(GLFWwindow*& window, int w, int h, App& app);
int initGLEW();
int initImGui(GLFWwindow* window);

void exitGLFW(GLFWwindow*& window);
void exitGLEW();
void exitImGui();

void initialization(App& app, GLFWwindow*& outwindow, int w, int h);
void frameStart(GLFWwindow& window, int& outw, int& outh);
void frameUpdate(App& app);
void frameRender(App& app, int w, int h);
void frameEnd(GLFWwindow& window);

#if defined(_CONSOLE)
int main(int argc, char* argv[])
#elif defined(_WIN32)
int WinMain(int argc, char* argv[])
#endif
{
    ////////////////////////////////////////////////////////////////////////////
    // Pre-Initialization
    hwinfo::init();
    atexit(hwinfo::exit);
    threading::lockCurrentThread(hwinfo::extra::runningCoreInd());

    ////////////////////////////////////////////////////////////////////////////
    // Initialization
    profiler::Enable();
    profiler::FrameStart();
    ///////////////////////
    int w = 1920, h = 1080;
    App app{};
    GLFWwindow* window = nullptr;
    initialization(app, window, w, h);
    ///////////////////////
    profiler::FrameEnd();
    profiler::Disable();
    printf("\n\n[History]\n\n");
    profiler::LogHistory();
    printf("\n\n[Initialization]\n\n");
    profiler::LogStats();
    profiler::ClearStats();

    ////////////////////////////////////////////////////////////////////////////
    // Main Loop
    profiler::Enable();
    ///////////////////////
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window))
    {
        ///////////////////////
        profiler::Disable();
        if (!app.isFrozen())
            profiler::Enable();
        profiler::FrameStart();
        ///////////////////////
        frameStart(*window, w, h);
        frameUpdate(app);
        frameRender(app, w, h);
        frameEnd(*window);
        ///////////////////////
        profiler::FrameEnd();
        //profiler::ClearStats();
    }
    ///////////////////////
    profiler::Disable();
    printf("\n\n[Main]\n\n");
    profiler::LogStats();

    ////////////////////////////////////////////////////////////////////////////
    // Termination
    exitImGui();
    exitGLEW();
    exitGLFW(window);
    exit(0);
}

// ==========================================================================================
// Internal (profiling)
// ==========================================================================================

void initialization(App& app, GLFWwindow*& outwindow, int w, int h) {
    if (int err = initGLFW(outwindow, w, h, app); err != 0) {
        std::cerr << "Error initializing GLFW, <code> = " << err << "\n";
        exit(err);
    }
    if (int err = initGLEW(); err != GLEW_OK) {
        std::cerr << "Error initializing GLEW, <code> = " << glewGetErrorString(err) << "\n";
        exit(err);
    }
    if (int err = initImGui(outwindow); err != 0) {
        std::cerr << "Error initializing ImGui, <code> = " << err << "\n";
        exit(err);
    }
    app.initialize();
}

void frameStart(GLFWwindow& window, int& outw, int& outh) {
    glfwGetFramebufferSize(&window, &outw, &outh);
    glfwPollEvents();
}

void frameUpdate(App& app) {
    static double lastFrameTimeSec = glfwGetTime();
    double now = glfwGetTime();
    double dtSec = (now - lastFrameTimeSec);
    lastFrameTimeSec = now;
    app.update(dtSec);
}

void frameRender(App& app, int w, int h) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetShortcutRouting(ImGuiMod_Ctrl | ImGuiKey_Tab, ImGuiKeyOwner_None);
    ImGui::SetShortcutRouting(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab, ImGuiKeyOwner_None);
    app.render(w, h);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void frameEnd(GLFWwindow& window) {
    glfwSwapBuffers(&window);
}

// ==========================================================================================
// GLFW
// ==========================================================================================

int initGLFW(GLFWwindow*& window, int w, int h, App& app) {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 4.5+ only
    window = glfwCreateWindow(w, h, "3D Cellular Automata", NULL, NULL);
    if (!window) return 2;
    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseBtnCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwMakeContextCurrent(window);
    return 0;
}

void exitGLFW(GLFWwindow*& window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void errorCallback(int errorCode, const char* description) {
    std::cerr << "GLFW error, <code> = " << errorCode << " : " << description << "\n";
    exit(errorCode);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;
    App& app = *(App*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS) app.onKeyDown(key);
    if (action == GLFW_RELEASE) app.onKeyUp(key);
    if (action == GLFW_REPEAT) app.onKeyDown(key); // TODO: Improve ?
}

void mouseBtnCallback(GLFWwindow* window, int key, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    App& app = *(App*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS) app.onMouseBtnDown(key);
    if (action == GLFW_RELEASE) app.onMouseBtnUp(key);
}

void scrollCallback(GLFWwindow* window, double dx, double dy) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    App& app = *(App*)glfwGetWindowUserPointer(window);
    app.onMouseWheel(dx, dy);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    App& app = *(App*)glfwGetWindowUserPointer(window);
    app.onResize(width, height);
}

// ==========================================================================================
// GLEW
// ==========================================================================================

int initGLEW() {
    GLenum err = glewInit();
    if (GLEW_OK != err) return err;
    return GLEW_OK;
}

void exitGLEW() {
    //
}

// ==========================================================================================
// IMGUI
// ==========================================================================================

int initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) return 1;
    if (!ImGui_ImplOpenGL3_Init("#version 450")) return 2;
    //////////////////////////////////////////////////////
    // Font
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.SizePixels = 22;
    cfg.RasterizerMultiply = 1.4f;
    //io.Fonts->AddFontDefault(&cfg);
    // https://www.fontsquirrel.com/fonts/Luxi-Mono
    io.Fonts->AddFontFromFileTTF(".\\assets\\luximr.ttf", cfg.SizePixels, &cfg);
    // https://larsenwork.com/monoid/
    //io.Fonts->AddFontFromFileTTF(".\\assets\\Monoid-Regular.ttf", cfg.SizePixels);
    //io.Fonts->AddFontFromFileTTF(".\\assets\\Monoid-Retina.ttf", cfg.SizePixels);
    //////////////////////////////////////////////////////
    // Colors
    ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(0, 156, 255, 64));
    return 0;
}

void exitImGui() {
    ImGui::PopStyleColor(); // ImGuiCol_Separator
    //////////////////////////////////////////////////////
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
