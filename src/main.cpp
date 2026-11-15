#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// window size
const int WIDTH = 1280;
const int HEIGHT = 720;

// camera
float cameraYaw = 52.0f;
float cameraPitch = 22.0f;
float cameraDistance = 8.0f;
glm::vec3 cameraTarget(0.0f, 0.5f, 0.0f);

// mouse
float lastMouseX = WIDTH / 2.0f;
float lastMouseY = HEIGHT / 2.0f;
bool firstMouse = true;

// rackets
float playerX = -1.05f;
float playerZ = 0.0f;
float opponentX = 1.05f;
float opponentZ = 0.0f;

// ball
glm::vec3 ballPosition(0.0f, 0.96f, 0.0f);

// number of vertices of the sphere
int sphereCount = 0;

// ---------------- SHADERS ----------------

string readFile(const char* path)
{
    ifstream file(path);
    if (!file.is_open())
        cout << "Cannot open " << path << endl;

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int makeShader(unsigned int type, string source)
{
    unsigned int shader = glCreateShader(type);
    const char* code = source.c_str();
    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);

    int ok;
    char log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        glGetShaderInfoLog(shader, 512, NULL, log);
        cout << "Shader error: " << log << endl;
    }
    return shader;
}

unsigned int makeProgram()
{
    unsigned int vs = makeShader(GL_VERTEX_SHADER, readFile("Shaders/base.vert"));
    unsigned int fs = makeShader(GL_FRAGMENT_SHADER, readFile("Shaders/base.frag"));

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok;
    char log[512];
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        glGetProgramInfoLog(program, 512, NULL, log);
        cout << "Link error: " << log << endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// ---------------- SHAPES ----------------

// each vertex = position (3 floats) + normal (3 floats)
float cubeVertices[] = {
    // back
    -0.5f,-0.5f,-0.5f,  0,0,-1,
     0.5f, 0.5f,-0.5f,  0,0,-1,
     0.5f,-0.5f,-0.5f,  0,0,-1,
     0.5f, 0.5f,-0.5f,  0,0,-1,
    -0.5f,-0.5f,-0.5f,  0,0,-1,
    -0.5f, 0.5f,-0.5f,  0,0,-1,
    // front
    -0.5f,-0.5f, 0.5f,  0,0,1,
     0.5f,-0.5f, 0.5f,  0,0,1,
     0.5f, 0.5f, 0.5f,  0,0,1,
     0.5f, 0.5f, 0.5f,  0,0,1,
    -0.5f, 0.5f, 0.5f,  0,0,1,
    -0.5f,-0.5f, 0.5f,  0,0,1,
    // left
    -0.5f, 0.5f, 0.5f, -1,0,0,
    -0.5f, 0.5f,-0.5f, -1,0,0,
    -0.5f,-0.5f,-0.5f, -1,0,0,
    -0.5f,-0.5f,-0.5f, -1,0,0,
    -0.5f,-0.5f, 0.5f, -1,0,0,
    -0.5f, 0.5f, 0.5f, -1,0,0,
    // right
     0.5f, 0.5f, 0.5f, 1,0,0,
     0.5f,-0.5f,-0.5f, 1,0,0,
     0.5f, 0.5f,-0.5f, 1,0,0,
     0.5f,-0.5f,-0.5f, 1,0,0,
     0.5f, 0.5f, 0.5f, 1,0,0,
     0.5f,-0.5f, 0.5f, 1,0,0,
     // bottom
     -0.5f,-0.5f,-0.5f, 0,-1,0,
      0.5f,-0.5f,-0.5f, 0,-1,0,
      0.5f,-0.5f, 0.5f, 0,-1,0,
      0.5f,-0.5f, 0.5f, 0,-1,0,
     -0.5f,-0.5f, 0.5f, 0,-1,0,
     -0.5f,-0.5f,-0.5f, 0,-1,0,
     // top
     -0.5f, 0.5f,-0.5f, 0,1,0,
      0.5f, 0.5f, 0.5f, 0,1,0,
      0.5f, 0.5f,-0.5f, 0,1,0,
      0.5f, 0.5f, 0.5f, 0,1,0,
     -0.5f, 0.5f,-0.5f, 0,1,0,
     -0.5f, 0.5f, 0.5f, 0,1,0
};

// makes a VAO from a list of floats (position + normal)
unsigned int makeVAO(float* data, int size)
{
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return VAO;
}

unsigned int makeSphere()
{
    vector<float> v;
    int sectors = 24;
    int stacks = 16;
    float PI = 3.14159265f;

    for (int i = 0; i < stacks; i++)
    {
        float a1 = PI * i / stacks;
        float a2 = PI * (i + 1) / stacks;

        for (int j = 0; j < sectors; j++)
        {
            float b1 = 2 * PI * j / sectors;
            float b2 = 2 * PI * (j + 1) / sectors;

            glm::vec3 p1(sin(a1) * cos(b1), cos(a1), sin(a1) * sin(b1));
            glm::vec3 p2(sin(a2) * cos(b1), cos(a2), sin(a2) * sin(b1));
            glm::vec3 p3(sin(a2) * cos(b2), cos(a2), sin(a2) * sin(b2));
            glm::vec3 p4(sin(a1) * cos(b2), cos(a1), sin(a1) * sin(b2));

            // 2 triangles
            glm::vec3 pts[6] = { p1, p2, p3, p1, p3, p4 };

            for (int k = 0; k < 6; k++)
            {
                // position
                v.push_back(pts[k].x);
                v.push_back(pts[k].y);
                v.push_back(pts[k].z);
                // normal (same as position on a sphere)
                v.push_back(pts[k].x);
                v.push_back(pts[k].y);
                v.push_back(pts[k].z);
            }
        }
    }

    sphereCount = v.size() / 6;
    return makeVAO(v.data(), v.size() * sizeof(float));
}

// ---------------- DRAWING ----------------

// draws any shape (cube or sphere) at a position with a scale
void draw(unsigned int shader, unsigned int VAO, int count, glm::vec3 pos, glm::vec3 scale)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, scale);

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void drawRacket(unsigned int shader, unsigned int cube, float x, float z)
{
    // head
    draw(shader, cube, 36, glm::vec3(x, 0.98f, z), glm::vec3(0.07f, 0.20f, 0.23f));
    // handle
    draw(shader, cube, 36, glm::vec3(x, 0.82f, z), glm::vec3(0.055f, 0.12f, 0.075f));
}

// ---------------- CAMERA AND INPUT ----------------

glm::mat4 getView()
{
    float yaw = glm::radians(cameraYaw);
    float pitch = glm::radians(cameraPitch);

    glm::vec3 pos;
    pos.x = cameraTarget.x + cameraDistance * cos(pitch) * sin(yaw);
    pos.y = cameraTarget.y + cameraDistance * sin(pitch);
    pos.z = cameraTarget.z + cameraDistance * cos(pitch) * cos(yaw);

    return glm::lookAt(pos, cameraTarget, glm::vec3(0, 1, 0));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float dx = xpos - lastMouseX;
    float dy = lastMouseY - ypos;
    lastMouseX = xpos;
    lastMouseY = ypos;

    cameraYaw += dx * 0.15f;
    cameraPitch += dy * 0.15f;

    // limit the pitch
    if (cameraPitch > 75.0f) cameraPitch = 75.0f;
    if (cameraPitch < -10.0f) cameraPitch = -10.0f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraDistance -= yoffset * 0.4f;

    // limit the zoom
    if (cameraDistance < 3.0f) cameraDistance = 3.0f;
    if (cameraDistance > 15.0f) cameraDistance = 15.0f;
}

void processInput(GLFWwindow* window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float move = 1.8f * dt;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  playerZ -= move;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) playerZ += move;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    playerX += move;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  playerX -= move;

    // keep the racket on its side of the table
    playerZ = glm::clamp(playerZ, -0.55f, 0.55f);
    playerX = glm::clamp(playerX, -1.20f, -0.78f);
}

// ---------------- MAIN ----------------

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ping Pong a la Royale", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    unsigned int shader = makeProgram();
    unsigned int cube = makeVAO(cubeVertices, sizeof(cubeVertices));
    unsigned int sphere = makeSphere();

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float now = glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;

        processInput(window, dt);

        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // send camera to the shader
        glm::mat4 view = getView();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // room
        draw(shader, cube, 36, glm::vec3(0, -0.12f, 0), glm::vec3(8, 0.2f, 8));      // floor
        draw(shader, cube, 36, glm::vec3(0, 2, -4), glm::vec3(8, 4, 0.15f));         // back wall
        draw(shader, cube, 36, glm::vec3(-4, 2, 0), glm::vec3(0.15f, 4, 8));         // left wall
        draw(shader, cube, 36, glm::vec3(4, 2, 0), glm::vec3(0.15f, 4, 8));          // right wall

        // lamp
        draw(shader, cube, 36, glm::vec3(0, 3.3f, 0), glm::vec3(0.025f, 0.8f, 0.025f));  // cable
        draw(shader, cube, 36, glm::vec3(0, 2.85f, 0), glm::vec3(0.65f, 0.12f, 0.65f));  // shade
        draw(shader, cube, 36, glm::vec3(0, 2.72f, 0), glm::vec3(0.45f, 0.15f, 0.45f));  // body
        draw(shader, sphere, sphereCount, glm::vec3(0, 2.61f, 0), glm::vec3(0.06f));      // bulb

        // table
        draw(shader, cube, 36, glm::vec3(0, 0.80f, 0), glm::vec3(2.74f, 0.08f, 1.525f)); // top

        // legs
        draw(shader, cube, 36, glm::vec3(-1.15f, 0.36f, -0.60f), glm::vec3(0.10f, 0.72f, 0.10f));
        draw(shader, cube, 36, glm::vec3(1.15f, 0.36f, -0.60f), glm::vec3(0.10f, 0.72f, 0.10f));
        draw(shader, cube, 36, glm::vec3(-1.15f, 0.36f, 0.60f), glm::vec3(0.10f, 0.72f, 0.10f));
        draw(shader, cube, 36, glm::vec3(1.15f, 0.36f, 0.60f), glm::vec3(0.10f, 0.72f, 0.10f));

        // support bars
        draw(shader, cube, 36, glm::vec3(0, 0.42f, -0.60f), glm::vec3(2.35f, 0.08f, 0.08f));
        draw(shader, cube, 36, glm::vec3(0, 0.42f, 0.60f), glm::vec3(2.35f, 0.08f, 0.08f));

        // rackets
        drawRacket(shader, cube, playerX, playerZ);
        drawRacket(shader, cube, opponentX, opponentZ);

        // ball
        draw(shader, sphere, sphereCount, ballPosition, glm::vec3(0.06f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}