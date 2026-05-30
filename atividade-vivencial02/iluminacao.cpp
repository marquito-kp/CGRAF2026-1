/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 *
 * ============================================================
  * Adaptado por Marcos Krol Pacheco
  * MÓDULO 5 — Iluminação de 3 Pontos
  * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <assert.h>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Textura
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader atualizado com normais e iluminação

const GLchar* vertexShaderSource = "#version 450\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "layout (location = 2) in vec2 texCoord;\n"
    "layout (location = 3) in vec3 normal;\n"           
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec4 finalColor;\n"
    "out vec2 fragTexCoord;\n"
    "out vec3 fragPos;\n"                               
    "out vec3 fragNormal;\n"                            
    "void main()\n"
    "{\n"
    "    vec4 worldPos   = model * vec4(position, 1.0);\n"
    "    gl_Position     = projection * view * worldPos;\n"
    "    finalColor      = vec4(color, 1.0);\n"
    "    fragTexCoord    = texCoord;\n"
    "    fragPos         = vec3(worldPos);\n"            
    "    fragNormal      = mat3(transpose(inverse(model))) * normal;\n" 
    "}\0";

// Fragment Shader com Phong + atenuação + 3 luzes

const GLchar* fragmentShaderSource = "#version 450\n"
    "in vec4 finalColor;\n"
    "in vec2 fragTexCoord;\n"
    "in vec3 fragPos;\n"                                
    "in vec3 fragNormal;\n"                             
    "uniform sampler2D texBuff;\n"

    // 3 luzes pontuais
    "uniform vec3  lightPos[3];\n"
    "uniform vec3  lightColor[3];\n"
    "uniform float lightIntensity[3];\n"
    "uniform bool  lightEnabled[3];\n"

    // câmera 
    "uniform vec3 viewPos;\n"

    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    vec3 norm    = normalize(fragNormal);\n"
    "    vec4 texColor = texture(texBuff, fragTexCoord);\n"
    "    vec3 result  = vec3(0.0);\n"

    // constantes de atenuação
    "    float kc = 1.0;\n"
    "    float kl = 0.09;\n"
    "    float kq = 0.032;\n"

    // loop sobre as 3 luzes
    "    for(int i = 0; i < 3; i++)\n"
    "    {\n"
    "        if(!lightEnabled[i]) continue;\n"

    // Atenuação por distância (req. 1 do exercício)
    "        float dist        = length(lightPos[i] - fragPos);\n"
    "        float attenuation = 1.0 / (kc + kl * dist + kq * dist * dist);\n"

    // Ambiente
    "        vec3 ambient = 0.08 * lightColor[i] * lightIntensity[i];\n"

    // Difuso
    "        vec3  lightDir = normalize(lightPos[i] - fragPos);\n"
    "        float diff     = max(dot(norm, lightDir), 0.0);\n"
    "        vec3  diffuse  = diff * lightColor[i] * lightIntensity[i] * attenuation;\n" // [ADIÇÃO] atenuação aplicada

    // Especular (Phong, shininess = 32)
    "        vec3  viewDir   = normalize(viewPos - fragPos);\n"
    "        vec3  reflDir   = reflect(-lightDir, norm);\n"
    "        float spec      = pow(max(dot(viewDir, reflDir), 0.0), 32.0);\n"
    "        vec3  specular  = 0.4 * spec * lightColor[i] * lightIntensity[i] * attenuation;\n"

    "        result += ambient + diffuse + specular;\n"
    "    }\n"

    "    color = vec4(result, 1.0) * texColor;\n"
    "}\n\0";


// Estrutura PointLight e array das 3 luzes

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float     intensity;
    bool      enabled;
};

// Array com as 3 luzes (inicializado depois, dentro de main,
// baseado na posição do objeto principal)
PointLight lights[3];

// Função que posiciona as 3 luzes automaticamente

void setupThreePointLights(const glm::vec3& objPos, float objScale)
{
    // distância radial e altura proporcionais ao tamanho do objeto
    float radius = objScale * 5.0f;
    float height = objScale * 3.5f;

    // ---- KEY LIGHT (luz principal) ----
    // Frente-direita, acima  ~45° horizontal / +35° vertical
    // Mais intensa — define o tom geral da cena
    lights[0].position  = objPos + glm::vec3( radius * 0.707f,  height,  radius * 0.707f);
    lights[0].color     = glm::vec3(1.0f, 0.95f, 0.85f);  // branco levemente quente
    lights[0].intensity = 1.5f;
    lights[0].enabled   = true;

    // ---- FILL LIGHT (luz de preenchimento) ----
    // Frente-esquerda, altura média  ~135° horizontal / +15° vertical
    // Metade da intensidade da key — suaviza sombras sem eliminar
    lights[1].position  = objPos + glm::vec3(-radius * 0.707f,  height * 0.4f,  radius * 0.5f);
    lights[1].color     = glm::vec3(0.8f, 0.85f, 1.0f);   // levemente fria/azulada
    lights[1].intensity = 0.7f;
    lights[1].enabled   = true;

    // ---- BACK LIGHT (luz de fundo / contorno) ----
    // Atrás-esquerda, bem acima  ~225° horizontal / +45° vertical
    // Cria o contorno do objeto separando-o do fundo
    lights[2].position  = objPos + glm::vec3(-radius * 0.5f,  height * 1.2f, -radius);
    lights[2].color     = glm::vec3(0.9f, 0.9f, 1.0f);    // branco-frio
    lights[2].intensity = 1.0f;
    lights[2].enabled   = true;
}

// Função de carregamento de textura 
GLuint loadTexture(const std::string& path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 0);

    if (data)
    {
        GLenum format = (nChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
                     0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Falha ao carregar textura: " << path << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// Função para carregar MTL 
std::string loadMTL(const std::string& mtlPath)
{
    std::ifstream file(mtlPath);
    std::string line, textureName;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string keyword;
        ss >> keyword;

        if (keyword == "map_Kd")
        {
            ss >> textureName;
            break;
        }
    }
    return textureName;
}

// Stride passa de 8 para 11 floats: pos(3) + cor(3) + uv(2) + normal(3)
int loadSimpleOBJ(string filePATH, int &nVertices)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line))
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt") {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn") {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index))      ni = !index.empty() ? std::stoi(index) - 1 : 0;

                // posição
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                // cor
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
                // UV
                if (!texCoords.empty() && ti < (int)texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
               
                if (!normals.empty() && ni < (int)normals.size()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(1.0f);  
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arqEntrada.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // stride de 11 floats: pos(3) + cor(3) + uv(2) + normal(3) 
    const int STRIDE = 11;

    // posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    // [ADIÇÃO] normal — location 3
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / STRIDE;
    return VAO;
}

// Estrutura do objeto
struct Object3D {
    GLuint VAO;
    GLuint textureID;
    int numVertices;
    float translateX = 0.0f;
    float translateY = 0.0f;
    float translateZ = 0.0f;
    float scaleUniform = 1.0f;
    bool rotateX = false;
    bool rotateY = false;
    bool rotateZ = false;
};

std::vector<Object3D> objects;
int selectedObject = 0;


// Função MAIN
int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Marcos Krol Pacheco!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    cout << "\n=== CONTROLES ===" << endl;
    cout << "Alternar entre objetos : TAB" << endl;
    cout << "Rotacao                : X, Y, Z" << endl;
    cout << "Mover XZ               : W (frente) S (tras) A (esq) D (dir)" << endl;
    cout << "Mover Y                : I (sobe) J (desce)" << endl;
    cout << "Escala                 : [ (diminui)  ] (aumenta)" << endl;
    // ontroles das luzes impressos no terminal
    cout << "--- Luzes ---" << endl;
    cout << "Key  light (principal)    : tecla 1" << endl;
    cout << "Fill light (preenchimento): tecla 2" << endl;
    cout << "Back light (fundo)        : tecla 3" << endl;
    cout << "Sair                      : ESC" << endl;
    cout << "=================" << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    GLuint VAO      = setupGeometry();

    glUseProgram(shaderID);

    glm::mat4 model = glm::mat4(1);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    GLint viewLoc       = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");

    // posição da câmera (guardamos para enviar ao shader como viewPos)
    glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 7.0f);
    glm::mat4 view = glm::lookAt(
        cameraPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WIDTH / (float)HEIGHT,
        0.1f, 100.0f
    );
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // envia posição da câmera ao shader
    GLint viewPosLoc = glGetUniformLocation(shaderID, "viewPos");
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

    glEnable(GL_DEPTH_TEST);

    // carrega o OBJ
    int nVertices;
    GLuint objVAO   = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);
    string textureName = loadMTL("../assets/Modelos3D/Suzanne.mtl");
    GLuint textureID   = loadTexture("../assets/Modelos3D/" + textureName);

    Object3D obj1;
    obj1.VAO          = objVAO;
    obj1.numVertices  = nVertices;
    obj1.translateX   = -3.0f;
    obj1.translateY   =  0.0f;
    obj1.translateZ   =  0.0f;
    obj1.scaleUniform =  1.0f;
    obj1.textureID    = textureID;
    objects.push_back(obj1);

    Object3D obj2;
    obj2.VAO          = objVAO;
    obj2.numVertices  = nVertices;
    obj2.translateX   =  3.0f;
    obj2.translateY   =  0.0f;
    obj2.translateZ   =  0.0f;
    obj2.scaleUniform =  1.0f;
    obj2.textureID    = textureID;
    objects.push_back(obj2);

    
    // Posiciona as 3 luzes a partir do objeto principal

    glm::vec3 mainObjPos1(obj1.translateX, obj1.translateY, obj1.translateZ);
    setupThreePointLights(mainObjPos1, obj1.scaleUniform);

    glm::vec3 mainObjPos2(obj2.translateX, obj2.translateY, obj2.translateZ);
    setupThreePointLights(mainObjPos2, obj2.scaleUniform);

    // Obtém as locations dos uniforms de iluminação
   
    GLint lightPosLoc[3], lightColorLoc[3], lightIntensityLoc[3], lightEnabledLoc[3];
    for (int i = 0; i < 3; i++)
    {
        string base = "lightPos["      + to_string(i) + "]";
        lightPosLoc[i]       = glGetUniformLocation(shaderID, ("lightPos["       + to_string(i) + "]").c_str());
        lightColorLoc[i]     = glGetUniformLocation(shaderID, ("lightColor["     + to_string(i) + "]").c_str());
        lightIntensityLoc[i] = glGetUniformLocation(shaderID, ("lightIntensity[" + to_string(i) + "]").c_str());
        lightEnabledLoc[i]   = glGetUniformLocation(shaderID, ("lightEnabled["   + to_string(i) + "]").c_str());
    }

    // Game Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        float angle = (GLfloat)glfwGetTime();


        // Atualiza as luzes caso o objeto principal mova
        // Recalcula posicionamento automático a cada frame

        mainObjPos1 = glm::vec3(
            objects[0].translateX,
            objects[0].translateY,
            objects[0].translateZ
        );
        setupThreePointLights(mainObjPos1, objects[0].scaleUniform);

        mainObjPos2 = glm::vec3(
            objects[0].translateX,
            objects[0].translateY,
            objects[0].translateZ
        );
        setupThreePointLights(mainObjPos2, objects[0].scaleUniform);

        // Envia uniforms das 3 luzes ao shader
        for (int i = 0; i < 3; i++)
        {
            glUniform3fv (lightPosLoc[i],       1,    glm::value_ptr(lights[i].position));
            glUniform3fv (lightColorLoc[i],     1,    glm::value_ptr(lights[i].color));
            glUniform1f  (lightIntensityLoc[i],       lights[i].intensity);
            glUniform1i  (lightEnabledLoc[i],         lights[i].enabled ? 1 : 0);
        }

        // loop de desenho dos objetos (estrutura original preservada)
        for (int i = 0; i < (int)objects.size(); i++)
        {
            model = glm::mat4(1);
            model = glm::translate(model, glm::vec3(
                objects[i].translateX,
                objects[i].translateY,
                objects[i].translateZ
            ));
            model = glm::scale(model, glm::vec3(
                objects[i].scaleUniform,
                objects[i].scaleUniform,
                objects[i].scaleUniform
            ));

            if (objects[i].rotateX)
                model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            else if (objects[i].rotateY)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            else if (objects[i].rotateZ)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, objects[i].textureID);
            glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(objects[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, objects[i].numVertices);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// Função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        objects[selectedObject].rotateX = true;
        objects[selectedObject].rotateY = false;
        objects[selectedObject].rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
        objects[selectedObject].rotateX = false;
        objects[selectedObject].rotateY = true;
        objects[selectedObject].rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        objects[selectedObject].rotateX = false;
        objects[selectedObject].rotateY = false;
        objects[selectedObject].rotateZ = true;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedObject = (selectedObject + 1) % objects.size();
        cout << "Objeto " << (selectedObject + 1) << " selecionado" << endl;
    }

    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].translateX += 0.1f;
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].translateX -= 0.1f;
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].translateZ -= 0.1f;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].translateZ += 0.1f;
    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].translateY += 0.1f;
    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].translateY -= 0.1f;

    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        objects[selectedObject].scaleUniform -= 0.05f;
        if (objects[selectedObject].scaleUniform < 0.05f)
            objects[selectedObject].scaleUniform = 0.05f;
    }
    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].scaleUniform += 0.05f;

    // Teclas 1, 2, 3 ligam e desligam as 3 luzes
    if (key == 49 && action == GLFW_PRESS)  // '1'
    {
        lights[0].enabled = !lights[0].enabled;
        cout << "Key  light: " << (lights[0].enabled ? "ON" : "OFF") << endl;
    }
    if (key == 50 && action == GLFW_PRESS)  // '2'
    {
        lights[1].enabled = !lights[1].enabled;
        cout << "Fill light: " << (lights[1].enabled ? "ON" : "OFF") << endl;
    }
    if (key == 51 && action == GLFW_PRESS)  // '3'
    {
        lights[2].enabled = !lights[2].enabled;
        cout << "Back light: " << (lights[2].enabled ? "ON" : "OFF") << endl;
    }
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// stride atualizado para 11 floats para consistência com o OBJ
int setupGeometry()
{
    GLfloat vertices[] = {
        // FACE FRONTAL (z = +0.5) — VERMELHO
        -0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 0.2f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 0.2f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.2f, 0.2f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.2f, 0.2f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.2f, 0.2f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 0.2f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
        // FACE TRASEIRA (z = -0.5) — VERDE
         0.5f, -0.5f, -0.5f,  0.2f, 1.0f, 0.2f,  0.0f, 0.0f,  0.0f, 0.0f,-1.0f,
        -0.5f, -0.5f, -0.5f,  0.2f, 1.0f, 0.2f,  1.0f, 0.0f,  0.0f, 0.0f,-1.0f,
        -0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 0.2f,  1.0f, 1.0f,  0.0f, 0.0f,-1.0f,
        -0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 0.2f,  1.0f, 1.0f,  0.0f, 0.0f,-1.0f,
         0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 0.2f,  0.0f, 1.0f,  0.0f, 0.0f,-1.0f,
         0.5f, -0.5f, -0.5f,  0.2f, 1.0f, 0.2f,  0.0f, 0.0f,  0.0f, 0.0f,-1.0f,
        // FACE ESQUERDA (x = -0.5) — AZUL
        -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.2f, 0.4f, 1.0f,  1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,  1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,  1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.2f, 0.4f, 1.0f,  0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        // FACE DIREITA (x = +0.5) — AMARELO
         0.5f, -0.5f,  0.5f,  1.0f, 0.9f, 0.1f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.9f, 0.1f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.9f, 0.1f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.9f, 0.1f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.9f, 0.1f,  0.0f, 1.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.9f, 0.1f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        // FACE SUPERIOR (y = +0.5) — CIANO
        -0.5f,  0.5f,  0.5f,  0.1f, 0.9f, 0.9f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.1f, 0.9f, 0.9f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.1f, 0.9f, 0.9f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.1f, 0.9f, 0.9f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.1f, 0.9f, 0.9f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.1f, 0.9f, 0.9f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        // FACE INFERIOR (y = -0.5) — MAGENTA
        -0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,  0.0f, 0.0f,  0.0f,-1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,  1.0f, 0.0f,  0.0f,-1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,  1.0f, 1.0f,  0.0f,-1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,  1.0f, 1.0f,  0.0f,-1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,  0.0f, 1.0f,  0.0f,-1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,  0.0f, 0.0f,  0.0f,-1.0f, 0.0f,
    };

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // stride = 11 floats: pos(3) + cor(3) + uv(2) + normal(3) [ADIÇÃO: +3 normais]
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    // [ADIÇÃO] normal — location 3
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}