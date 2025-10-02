﻿#include <iostream>

// OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

// Setup VS and PS in GLSL
const char* vertexShaderSource = "\n"
"#version 460 core\n"
"layout (location = 0) in vec3 inPos;\n"
"layout (location = 1) in vec3 inColor;\n"
"out vec3 vertexColor;\n"
"uniform mat4 modelViewProj;\n"
"void main()\n"
"{\n"
"gl_Position = modelViewProj * vec4(inPos, 1.0);\n"
"vertexColor = inColor;\n"
"}\0";

const char* fragmentShaderSource = "\n"
"#version 460 core\n"
"in vec3 vertexColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(vertexColor, 1.0f);\n"
"}\0";

int main()
{
    // Window Resolution
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    // Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return -1;
    }

    // Setup Min/Major version for using OpenGL 4.6
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    // Set Core Profile Mode
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create OpenGL window using SDL
    SDL_Window* window = SDL_CreateWindow("EnginishGL",
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    // Early out if window not valid
    if (window == nullptr)
    {
        SDL_Quit();
        return -1;
    }

    // OpenGL is context based and thread local
    // Link OpenGL context to SDL, after this you can load OpenGL functions and start rendering
    // Multi-threading -> Define multiple context and make them current
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Init all OpenGL function pointers at runtime (not linked at compile time)
    gladLoadGL();

    // Used for mapping NDC coordinates (-1.0f to 1.0f) to pixel coordinates (e.g. 1920x1080)
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init();

    // Create & compile vertex and fragment shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create Program and bind shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Delete shaders since we've created a program already and they are contained there
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Local Space
    GLfloat cubeVertices[]
    {
    //  Position                Color
        -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f,      0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, -0.5f,     1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f,       1.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,
    };

    const unsigned int NUM_INDICES = 36;
    GLuint cubeIndices[]
    {
        // Top face
        3, 2, 6,
        6, 7, 3,
        // Bottom face
        0, 1, 5,
        5, 4, 0,
        // Left face
        0, 4, 7,
        7, 3, 0,
        // Right face
        1, 5, 6,
        6, 2, 1,
        // Back face
        0, 1, 2,
        2, 3, 0,
        // Front face
        4, 5, 6,
        6, 7, 4,
    };

    // View Matrix
    glm::vec3 position(0.0f, 0.0f, -5.0f);
    glm::vec3 forward(0.0f, 0.0f, 1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 viewMatrix = glm::lookAt(position             // Camera Position
                                     , position + forward   // Target Position
                                     , up);                 // Up Vector
    // Projection Matrix
    const float FOV = 45.0f;
    const float NEAR_PLANE = 0.1f;
    const float FAR_PLANE = 100.0f;
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FOV), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), NEAR_PLANE, FAR_PLANE);


    // Create ModelViewProjection matrix
    GLuint modelViewProjLocation = glGetUniformLocation(shaderProgram, "modelViewProj");

    // Create VAO & VBO & EBO
    GLuint VAO;
    glGenVertexArrays(1, &VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);

    GLuint EBO;
    glGenBuffers(1, &EBO);

    // Bind VAO and VBO
    glBindVertexArray(VAO);

    // Link GL_ARRAY_BUFFER to vertices data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Link GL_ELEMENT_ARRAY_BUFFER to indices data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Define Vertex layout and set attribute index
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // Unlink VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Rotate the cube over time
    SDL_Time prevTime;
    SDL_GetCurrentTime(&prevTime);
    float rotation = 0.0f;
    const float SPEED = 100.0f;

    bool isRunning = true;
    while (isRunning)
    {
        // INPUT
        // SDL EVENTS
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                isRunning = false;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP 
                && event.button.button == SDL_BUTTON_RIGHT)
            {
            }

            if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN 
                && event.button.button == SDL_BUTTON_RIGHT))
            {
            }

        }

        // SDL KEYS
        int numKeys;
        const bool* keys = SDL_GetKeyboardState(&numKeys);
        if (keys[SDL_SCANCODE_W])
        {
        }

        // UPDATE
        // Model Matrix
        glm::mat4 modelMatrix(1.0f);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 modelViewProj = projectionMatrix * viewMatrix * modelMatrix;

        // Perform Rotation
        SDL_Time currentTime;
        SDL_GetCurrentTime(&currentTime);

        const float dt = (currentTime - prevTime) / 1000000000.0f;
        rotation += SPEED * dt;
        prevTime = currentTime;

        // RENDER
        // Clear screen color
        glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

        // Clear Color Buffer and Depth Buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // RENDER
        // Use shader program & bind VAO
        glUseProgram(shaderProgram);
        glUniformMatrix4fv (modelViewProjLocation, 1, GL_FALSE, glm::value_ptr(modelViewProj));

        glBindVertexArray(VAO);

        // Specify primitive type and vertex count
        glDrawElements(GL_TRIANGLES, NUM_INDICES, GL_UNSIGNED_INT, 0);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Show sample demo from ImGui
        ImGui::ShowDemoWindow();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap window
        SDL_GL_SwapWindow(window);

        // FRAME CONTROL
        // [...]
    }

    // Delete VAO, VBO and shader program
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Deinit ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // DeInit SDL
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
