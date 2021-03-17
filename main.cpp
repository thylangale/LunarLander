#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#include <iostream>
#include <vector>

struct GameState {
    Entity* rocket;
    Entity* platform;
};

GameState state;
int PLATFORM_COUNT = 29;
GLuint fontTextureID;
bool gameWon = false;
bool gameOver = false;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text,
    float size, float spacing, glm::vec3 position)
{
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;

    std::vector<float> vertices;
    std::vector<float> texCoords;

    for (int i = 0; i < text.size(); i++) {

        int index = (int)text[i];
        float offset = (size + spacing) * i;
        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;
        vertices.insert(vertices.end(), {
        offset + (-0.5f * size), 0.5f * size,
        offset + (-0.5f * size), -0.5f * size,
        offset + (0.5f * size), 0.5f * size,
        offset + (0.5f * size), -0.5f * size,
        offset + (0.5f * size), 0.5f * size,
        offset + (-0.5f * size), -0.5f * size,
            });
        texCoords.insert(texCoords.end(), {
            u, v,
            u, v + height,
            u + width, v,
            u + width, v + height,
            u + width, v,
            u, v + height,
            });

    } // end of for loop

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);

    glUseProgram(program->programID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}


void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Thy-Lan Gale - Project 3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); //background color
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // Initialize Game Objects
    fontTextureID = LoadTexture("font1.png");

    // Initialize rocket
    state.rocket = new Entity();
    state.rocket->position = glm::vec3(0, 4.0f, 0);
    state.rocket->movement = glm::vec3(0);
    state.rocket->acceleration = glm::vec3(0, -0.10f, 0);
    state.rocket->speed = 1.0f;
    state.rocket->textureID = LoadTexture("rocket.jpg");

    // Initialize platform
    state.platform = new Entity[PLATFORM_COUNT];
    GLuint platformTextureID = LoadTexture("platform.jfif");
    GLuint rockTextureID = LoadTexture("rock.jpg");

    state.platform[0].position = glm::vec3(2.0, -3.5f, 0);
    state.platform[0].textureID = platformTextureID;

    state.platform[1].position = glm::vec3(1.0f, -3.5f, 0);
    state.platform[1].textureID = platformTextureID;
    
    //Initialize rocks
    int x = -4.0f;
    for (int i = 2; i < 9; i++) { //bottom row rocks
        state.platform[i].position = glm::vec3(x, -3.5f, 0);
        if (x == 0) x = 3.0f;
        else x += 1.0f;
        state.platform[i].textureID = rockTextureID;
    }
    int y = -2.5f;
    for (int i = 9; i < 15; i++) { //left col rocks
        state.platform[i].position = glm::vec3(-4.0f, y, 0);
        state.platform[i].textureID = rockTextureID;
        y += 1.0f;
    }
    int y2 = -2.5f;
    for (int i = 15; i < 21; i++) { //right col rocks
        state.platform[i].position = glm::vec3(4.0f, y2, 0);
        state.platform[i].textureID = rockTextureID;
        y2 += 1.0f;
    }
    state.platform[21].position = glm::vec3(-3.0f, 2.0f, 0);
    state.platform[21].textureID = rockTextureID;

    state.platform[22].position = glm::vec3(-2.0f, 2.0f, 0);
    state.platform[22].textureID = rockTextureID;

    state.platform[23].position = glm::vec3(1.0f, 1.0f, 0);
    state.platform[23].textureID = rockTextureID;
    
    state.platform[24].position = glm::vec3(3.0f, 1.0f, 0);
    state.platform[24].textureID = rockTextureID;
    
    state.platform[25].position = glm::vec3(2.0f, 1.0f, 0);
    state.platform[25].textureID = rockTextureID;

    state.platform[26].position = glm::vec3(-2.0f, -2.0f, 0);
    state.platform[26].textureID = rockTextureID;

    state.platform[27].position = glm::vec3(-3.0f, -2.0f, 0);
    state.platform[27].textureID = rockTextureID;

    state.platform[28].position = glm::vec3(-1.0f, -2.0f, 0);
    state.platform[28].textureID = rockTextureID;

    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platform[i].Update(0, NULL, 0);
    }
    

}

void ProcessInput() {

    state.rocket->movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
                // Move the rocket left
                break;

            case SDLK_RIGHT:
                // Move the rocket right
                break;

            case SDLK_SPACE:
                // Some sort of action
                break;
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        if (!gameOver && !gameWon) {
            state.rocket->movement.x = -1.0f;
        }
        
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        if (!gameOver && !gameWon) {
            state.rocket->movement.x = 1.0f;
        }
    }


    if (glm::length(state.rocket->movement) > 1.0f) {
        state.rocket->movement = glm::normalize(state.rocket->movement);
    }

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;
void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    deltaTime += accumulator;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.rocket->Update(FIXED_TIMESTEP, state.platform, PLATFORM_COUNT);
        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;
    if (state.rocket->collidedBottom && state.rocket->position.x >= 1.0f && state.rocket->position.x <= 2.0f &&
        state.rocket->position.y < -2.0f) gameWon = true;
    else if (state.rocket->collidedBottom || state.rocket->collidedLeft ||state.rocket->collidedRight) gameOver = true;
}


void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    state.rocket->Render(&program);
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platform[i].Render(&program);
    }
    
    if (gameWon) { //print mission successful
        DrawText(&program, fontTextureID, "Mission Successful", 0.5f, -0.25f, glm::vec3(-2.0f, 3.3, 0));
    }
    else if (gameOver) { //print mission failed
        DrawText(&program, fontTextureID, "Mission Failed", 0.5f, -0.25f, glm::vec3(-1.5f, 3.3, 0));
    }
    

    SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}