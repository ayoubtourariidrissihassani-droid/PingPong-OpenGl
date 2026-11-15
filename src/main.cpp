#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// window size
const int WIDTH = 1280;
const int HEIGHT = 720;

// ---------------- GAME SETTINGS (tune here) ----------------
const float TABLE_HALF_LENGTH = 1.37f;
const float TABLE_HALF_WIDTH = 0.7625f;
const float BALL_RADIUS = 0.06f;
const float BALL_Y = 0.96f;
const float FLOOR_BALL_Y = 0.04f;

const float RACKET_HALF_X = 0.035f;
const float RACKET_HALF_Z = 0.115f;

const float SERVE_SPEED = 2.0f;
const float MAX_SPEED = 4.5f;
const float SPEED_UP = 1.04f;
const float OPPONENT_REACT = 8.0f;
const float RESPAWN_DELAY = 0.8f;
const float PLAYER_SPEED = 2.4f;
const float HIT_MARGIN = 0.04f;

// ---------------- SPOT LIGHT SETTINGS (tune here) ----------------
const glm::vec3 LIGHT_POS(0.0f, 2.6f, 0.0f);          // just under the bulb
const glm::vec3 LIGHT_DIR(0.0f, -1.0f, 0.0f);         // pointing down
const glm::vec3 LIGHT_COLOR(1.0f, 0.92f, 0.70f);      // yellow, a bit white
const float LIGHT_INNER_ANGLE = 35.0f;
const float LIGHT_OUTER_ANGLE = 50.0f;
const float LIGHT_CONSTANT = 1.0f;
const float LIGHT_LINEAR = 0.07f;
const float LIGHT_QUADRATIC = 0.017f;

// ---------------- COLORS ----------------
// true = use the colors below, false = everything gray
const bool USE_COLORS = true;
const glm::vec3 COL_DEFAULT(0.65f, 0.65f, 0.65f);

// gray objects (same gray as before)
const glm::vec3 COL_GRAY_FLOOR(0.65f, 0.65f, 0.65f);
const glm::vec3 COL_GRAY_WALL(0.65f, 0.65f, 0.65f);
const glm::vec3 COL_METAL(0.65f, 0.65f, 0.65f);

// colored objects
const glm::vec3 COL_TABLE(0.10f, 0.25f, 0.75f);   // blue
const glm::vec3 COL_PLAYER(0.85f, 0.10f, 0.10f);   // red
const glm::vec3 COL_COMPUTER(0.08f, 0.08f, 0.08f);   // black
const glm::vec3 COL_BALL(1.00f, 0.85f, 0.10f);   // yellow

// ---------------- GAME STATE ----------------
enum GameState { WAITING, PLAYING, OUT };
GameState state = WAITING;

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
glm::vec3 ballStart(0.0f, BALL_Y, 0.0f);
glm::vec3 ballPosition = ballStart;
glm::vec3 ballVelocity(0.0f);
float outTimer = 0.0f;

// score
int rally = 0;
int bestRally = 0;

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

// small helpers to send values to the shader
void setVec3(unsigned int shader, const char* name, glm::vec3 v)
{
    glUniform3fv(glGetUniformLocation(shader, name), 1, glm::value_ptr(v));
}

void setFloat(unsigned int shader, const char* name, float v)
{
    glUniform1f(glGetUniformLocation(shader, name), v);
}

void setBool(unsigned int shader, const char* name, bool v)
{
    glUniform1i(glGetUniformLocation(shader, name), v ? 1 : 0);
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

// draws any shape (cube or sphere) at a position with a scale and a color
void draw(unsigned int shader, unsigned int VAO, int count,
    glm::vec3 pos, glm::vec3 scale,
    glm::vec3 color, bool emissive = false)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, scale);

    if (!USE_COLORS)
        color = COL_DEFAULT;   // everything gray

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    setVec3(shader, "uColor", color);
    setBool(shader, "uEmissive", emissive);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void drawRacket(unsigned int shader, unsigned int cube, float x, float z, glm::vec3 color)
{
    // head
    draw(shader, cube, 36, glm::vec3(x, 0.98f, z), glm::vec3(0.07f, 0.20f, 0.23f), color);
    // handle
    draw(shader, cube, 36, glm::vec3(x, 0.82f, z), glm::vec3(0.055f, 0.12f, 0.075f), color);
}

// ---------------- GAME LOGIC ----------------

float randomRange(float a, float b)
{
    return a + (b - a) * (rand() / (float)RAND_MAX);
}

void updateTitle(GLFWwindow* window)
{
    char title[160];
    if (state == WAITING)
        snprintf(title, sizeof(title),
            "Ping Pong a la Royale | Echange: %d | Record: %d | ESPACE pour servir",
            rally, bestRally);
    else
        snprintf(title, sizeof(title),
            "Ping Pong a la Royale | Echange: %d | Record: %d",
            rally, bestRally);
    glfwSetWindowTitle(window, title);
}

// puts the ball back at the center, waiting for a serve
void respawnBall()
{
    ballPosition = ballStart;
    ballVelocity = glm::vec3(0.0f);
    outTimer = 0.0f;
    state = WAITING;
    rally = 0;
}

// serve: the ball goes straight to the computer
void serveBall()
{
    ballPosition = ballStart;
    ballVelocity = glm::vec3(SERVE_SPEED, 0.0f, 0.0f);
    rally = 0;
    state = PLAYING;
}

void updateGame(float dt)
{
    // ---- computer racket: follows the ball on the z axis ----
    float target = 0.0f;
    if (state == PLAYING && ballVelocity.x > 0.0f)
        target = ballPosition.z;

    opponentZ += (target - opponentZ) * glm::min(1.0f, OPPONENT_REACT * dt);
    opponentZ = glm::clamp(opponentZ, -0.65f, 0.65f);

    // ---- waiting for the serve ----
    if (state == WAITING)
    {
        ballPosition = ballStart;
        return;
    }

    // ---- ball fell off the table ----
    if (state == OUT)
    {
        if (ballPosition.y > FLOOR_BALL_Y)
        {
            ballVelocity.y -= 9.8f * dt;               // gravity
            ballPosition += ballVelocity * dt;
        }
        else
        {
            ballPosition.y = FLOOR_BALL_Y;             // lies on the floor
            ballVelocity = glm::vec3(0.0f);
            outTimer += dt;
            if (outTimer > RESPAWN_DELAY)
                respawnBall();
        }
        return;
    }

    // ---- PLAYING: move the ball ----
    ballPosition.x += ballVelocity.x * dt;
    ballPosition.z += ballVelocity.z * dt;
    ballPosition.y = BALL_Y;

    // bounce on the sides of the table
    float limitZ = TABLE_HALF_WIDTH - BALL_RADIUS;
    if (ballPosition.z > limitZ) { ballPosition.z = limitZ;  ballVelocity.z = -ballVelocity.z; }
    if (ballPosition.z < -limitZ) { ballPosition.z = -limitZ; ballVelocity.z = -ballVelocity.z; }

    // computer racket: always hits the ball back
    if (ballVelocity.x > 0.0f &&
        ballPosition.x + BALL_RADIUS >= opponentX - RACKET_HALF_X)
    {
        ballPosition.x = opponentX - RACKET_HALF_X - BALL_RADIUS;
        float speed = glm::min(glm::abs(ballVelocity.x) * SPEED_UP, MAX_SPEED);
        ballVelocity.x = -speed;
        ballVelocity.z = randomRange(-0.9f, 0.9f);
    }

    // player racket
    if (ballVelocity.x < 0.0f &&
        ballPosition.x - BALL_RADIUS <= playerX + RACKET_HALF_X &&
        ballPosition.x + BALL_RADIUS >= playerX - RACKET_HALF_X &&
        glm::abs(ballPosition.z - playerZ) <= RACKET_HALF_Z + BALL_RADIUS + HIT_MARGIN)
    {
        ballPosition.x = playerX + RACKET_HALF_X + BALL_RADIUS;
        float speed = glm::min(glm::abs(ballVelocity.x) * SPEED_UP, MAX_SPEED);
        ballVelocity.x = speed;

        // the angle depends on where the ball touches the racket
        float offset = ballPosition.z - playerZ;
        ballVelocity.z = glm::clamp(offset * 6.0f, -1.8f, 1.8f);

        rally++;
        if (rally > bestRally) bestRally = rally;
    }

    // missed: the ball leaves the table on the player side
    if (ballPosition.x < -TABLE_HALF_LENGTH)
    {
        state = OUT;
        outTimer = 0.0f;
        ballVelocity.x *= 0.3f;
        ballVelocity.y = 0.0f;
    }
}

// ---------------- CAMERA AND INPUT ----------------

glm::vec3 getCameraPos()
{
    float yaw = glm::radians(cameraYaw);
    float pitch = glm::radians(cameraPitch);

    glm::vec3 pos;
    pos.x = cameraTarget.x + cameraDistance * cos(pitch) * sin(yaw);
    pos.y = cameraTarget.y + cameraDistance * sin(pitch);
    pos.z = cameraTarget.z + cameraDistance * cos(pitch) * cos(yaw);
    return pos;
}

glm::mat4 getView()
{
    return glm::lookAt(getCameraPos(), cameraTarget, glm::vec3(0, 1, 0));
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

    // serve with SPACE
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && state == WAITING)
        serveBall();

    float move = PLAYER_SPEED * dt;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  playerZ -= move;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) playerZ += move;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    playerX += move;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  playerX -= move;

    // keep the racket on its side of the table
    playerZ = glm::clamp(playerZ, -0.65f, 0.65f);
    playerX = glm::clamp(playerX, -1.20f, -0.78f);
}

// ---------------- MAIN ----------------

int main()
{
    srand((unsigned int)time(NULL));

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
    int lastRally = -1;
    GameState lastState = OUT;

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;
        if (dt > 0.05f) dt = 0.05f;   // avoid huge steps (window drag, lag)

        processInput(window, dt);
        updateGame(dt);

        // refresh the window title only when something changed
        if (rally != lastRally || state != lastState)
        {
            updateTitle(window);
            lastRally = rally;
            lastState = state;
        }

        // slightly dark gray background
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // send camera to the shader
        glm::mat4 view = getView();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        setVec3(shader, "viewPos", getCameraPos());

        // send the spot light to the shader
        setVec3(shader, "lightPos", LIGHT_POS);
        setVec3(shader, "lightDir", LIGHT_DIR);
        setVec3(shader, "lightColor", LIGHT_COLOR);
        setFloat(shader, "lightInnerCos", cos(glm::radians(LIGHT_INNER_ANGLE)));
        setFloat(shader, "lightOuterCos", cos(glm::radians(LIGHT_OUTER_ANGLE)));
        setFloat(shader, "lightConstant", LIGHT_CONSTANT);
        setFloat(shader, "lightLinear", LIGHT_LINEAR);
        setFloat(shader, "lightQuadratic", LIGHT_QUADRATIC);

        // room
        draw(shader, cube, 36, glm::vec3(0, -0.12f, 0), glm::vec3(8, 0.2f, 8), COL_GRAY_FLOOR);      // floor
        draw(shader, cube, 36, glm::vec3(0, 2, -4), glm::vec3(8, 4, 0.15f), COL_GRAY_WALL);          // back wall
        draw(shader, cube, 36, glm::vec3(-4, 2, 0), glm::vec3(0.15f, 4, 8), COL_GRAY_WALL);          // left wall
        draw(shader, cube, 36, glm::vec3(4, 2, 0), glm::vec3(0.15f, 4, 8), COL_GRAY_WALL);           // right wall

        // lamp
        draw(shader, cube, 36, glm::vec3(0, 3.3f, 0), glm::vec3(0.025f, 0.8f, 0.025f), COL_METAL);  // cable
        draw(shader, cube, 36, glm::vec3(0, 2.85f, 0), glm::vec3(0.65f, 0.12f, 0.65f), COL_METAL);  // shade
        draw(shader, cube, 36, glm::vec3(0, 2.72f, 0), glm::vec3(0.45f, 0.15f, 0.45f), COL_METAL);  // body
        draw(shader, sphere, sphereCount, glm::vec3(0, 2.61f, 0), glm::vec3(0.06f), LIGHT_COLOR, true); // bulb (emissive)

        // table
        draw(shader, cube, 36, glm::vec3(0, 0.80f, 0), glm::vec3(2.74f, 0.08f, 1.525f), COL_TABLE); // top

        // legs
        draw(shader, cube, 36, glm::vec3(-1.15f, 0.36f, -0.60f), glm::vec3(0.10f, 0.72f, 0.10f), COL_TABLE);
        draw(shader, cube, 36, glm::vec3(1.15f, 0.36f, -0.60f), glm::vec3(0.10f, 0.72f, 0.10f), COL_TABLE);
        draw(shader, cube, 36, glm::vec3(-1.15f, 0.36f, 0.60f), glm::vec3(0.10f, 0.72f, 0.10f), COL_TABLE);
        draw(shader, cube, 36, glm::vec3(1.15f, 0.36f, 0.60f), glm::vec3(0.10f, 0.72f, 0.10f), COL_TABLE);

        // support bars
        draw(shader, cube, 36, glm::vec3(0, 0.42f, -0.60f), glm::vec3(2.35f, 0.08f, 0.08f), COL_TABLE);
        draw(shader, cube, 36, glm::vec3(0, 0.42f, 0.60f), glm::vec3(2.35f, 0.08f, 0.08f), COL_TABLE);

        // rackets
        drawRacket(shader, cube, playerX, playerZ, COL_PLAYER);
        drawRacket(shader, cube, opponentX, opponentZ, COL_COMPUTER);

        // ball
        draw(shader, sphere, sphereCount, ballPosition, glm::vec3(BALL_RADIUS), COL_BALL);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}