
/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Classe Camera

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        Front = glm::vec3(0.0f, 0.0f, -1.0f);
        MovementSpeed = 2.5f;
        MouseSensitivity = 0.1f;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Encapsula a ação de "Mover"
    void Move(CameraMovement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)  Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
        if (direction == UP)       Position += WorldUp * velocity;
        if (direction == DOWN)     Position -= WorldUp * velocity;
    }

    // Encapsula a ação de "Rotacionar"
    void Rotate(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)  Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;

private:
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded

const GLchar* vertexShaderSource = "#version 450\n"
									"layout (location = 0) in vec3 position;\n"
									"layout (location = 1) in vec3 color;\n"
									"layout (location = 2) in vec2 tex_coord;\n" // acrescentado para processar os vertices de textura
									"layout (location = 3) in vec3 normal;\n" // acrescentado para processar os vertices de iluminação
									"uniform mat4 model;\n"
									"uniform mat4 projection;\n"
									"uniform mat4 view;\n"
									"out vec4 finalColor;\n"
									"out vec2 texCoord;\n" // saída para o fragment da Textura 
									"out vec3 iluNormal;\n" // saída para o fragment do Normal da iluminação
									"out vec3 iluPos;\n" // saída para o fragment da posição da iluminação
									"void main()\n"
									"{\n"
										//...pode ter mais linhas de código aqui!
										"gl_Position = projection * view * model * vec4(position, 1.0f);\n"
										"finalColor = vec4(color, 1.0);\n"
										"texCoord = vec2(tex_coord.x,tex_coord.y);\n"
										"iluPos = vec3(model * vec4(position,1.0f));\n"
										"iluNormal = mat3(transpose(inverse(model))) * normal;\n"
									"}\0";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
										"in vec2 texCoord;\n"
										"in vec3 iluNormal;\n"
										"in vec3 iluPos;\n"
										"uniform sampler2D texBuff;\n"

										// Propriedades do material
										"uniform vec3 Ka;\n"
										"uniform vec3 Ks;\n"
										"uniform float Ns;\n"

										// Key light
										"uniform vec3 keyLightPos;\n"
										"uniform vec3 keyLightColor;\n"
										"uniform bool keyLightEnabled;\n"

										// Fill light
										"uniform vec3 fillLightPos;\n"
										"uniform vec3 fillLightColor;\n"
										"uniform bool fillLightEnabled;\n"

										// Back light
										"uniform vec3 backLightPos;\n"
										"uniform vec3 backLightColor;\n"
										"uniform bool backLightEnabled;\n"

										"uniform vec3 viewPos;\n"
										"uniform int renderMode;\n"
										"out vec4 color;\n"

										// Função de atenuação
										"float attenuation(float distance)\n"
										"{\n"
											"float Kc = 1.0;\n"
											"float Kl = 0.09;\n"
											"float Kq = 0.032;\n"
											"return 1.0 / (Kc + Kl * distance + Kq * distance * distance);\n"
										"}\n"
										// Ambiente
										"vec3 ambient = Ka * vec3(1.0f);\n"
										// Função que calcula a contribuição de UMA luz
										"vec3 calculateLight(vec3 lightPos, vec3 lightColor, vec3 norm, vec3 viewDir)\n"
										"{\n"
											"vec3 lightDir = normalize(lightPos - iluPos);\n"
											"float dist = length(lightPos - iluPos);\n"
											"float att = attenuation(dist);\n"



											// Difusa (com atenuação)
											"float diff = max(dot(norm, lightDir), 0.0);\n"
											"vec3 diffuse = att * diff * lightColor;\n"

											// Especular
											"vec3 reflectDir = reflect(-lightDir, norm);\n"
											"float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);\n"
											"vec3 specular = spec * Ks * lightColor;\n"

											"return diffuse + specular;\n"
										"}\n"

										"void main()\n"
										"{\n"
											"vec3 norm = normalize(iluNormal);\n"
											"vec3 viewDir = normalize(viewPos - iluPos);\n"

											"vec3 ambient = Ka * vec3(1.0);\n"   // ambiente calculado uma única vez
											"vec3 result = ambient;\n"            // já inicia com o ambiente

											// Lógica para ligar e desligar a luz
											"if (keyLightEnabled)\n"
												"result += calculateLight(keyLightPos, keyLightColor, norm, viewDir);\n"

											"if (fillLightEnabled)\n"
												"result += calculateLight(fillLightPos, fillLightColor, norm, viewDir);\n"

											"if (backLightEnabled)\n"
												"result += calculateLight(backLightPos, backLightColor, norm, viewDir);\n"

											"vec4 texColor = texture(texBuff, texCoord);\n"
											"if (renderMode == 0) {\n"
												"color = vec4(result, 1.0) * texColor;\n"     // Phong + textura (modo normal)
											"} else if (renderMode == 1) {\n"
												"color = vec4(result, 1.0);\n"                 // só material (Ka/Kd/Ks), sem textura
											"} else {\n"
												"color = texColor;\n"                          // só textura, sem iluminação
											"}\n"
										"}\0";

// Estrutura do OBJ
struct OBJ {
	GLuint VAO;
	GLuint textureID;
	int nVertices;
	float translateX;
	float translateY;
	float translateZ;
	float scaleUniform;
	float angle = 1.0f;
	bool rotateX = false;
	bool rotateY = false;
	bool rotateZ = false;
	bool paused = false;

	// Pontos de controle da trajetória
	std::vector<glm::vec3> trajectory;
	int currentPoint = 0;
	float trajectoryT = 0.0f;
	bool followTrajectory = false;
};

// Estrutura para ler o MTL

struct MTL {
	glm::vec3 Ka;
	glm::vec3 Kd;
	glm::vec3 Ks;
	glm::vec3 Ke;
	float Ns;
	float Ni;
	float d;
	string mapKd;
	float illum;	
};

// Estrutura para definir a iluminação 
struct Light {
    glm::vec3 position;
    glm::vec3 color;
    bool enabled = true;
};
// instancias 
std::vector<OBJ> obj;
int selectedObj = 0;
int renderMode = 0; // 0 = Phong + textura; 1 = só material; 2 = só textura
int selectedLight; // 0 = Key; 1 = Fill; 2 = Back
Light keyLight, fillLight, backLight;

Light& getSelectedLight()
{
	if (selectedLight == 0) return keyLight;
	if (selectedLight == 1) return fillLight;
	return backLight;
}



Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame  = 0.0f;

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Carregamento do arquivo OBJ
int loadSimpleOBJ(string filePath,int &nVertices);
// Carregamento do arquivo MLT
MTL loadMTL(string filePath);
// Carregamento de textura
GLuint loadTexture(string filePath);

void setupThreePointLighting(OBJ &object);
void sendLightUniforms(GLuint shaderID);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void updateTrajectory(OBJ &object, float deltaTime);
glm::vec3 cubicBezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t);



// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	//Muita atenção aqui: alguns ambientes não aceitam essas configurações
	//Você deve adaptar para a versão do OpenGL suportada por sua placa
	//Sugestão: comente essas linhas de código para desobrir a versão e
	//depois atualize (por exemplo: 4.5 com 4 e 5)
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Essencial para computadores da Apple
	//#ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//#endif

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Marcos Krol!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	// Quando o usuário pressionar a tecla
	glfwSetKeyCallback(window, key_callback);

	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Desativa o cursor e mantém imagem travada

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	// cout << "Renderer: " << renderer << endl;
	// cout << "OpenGL version supported " << version << endl;
	cout << "===========================================================================" << endl;
	cout << "Bem vindo ao desafio do modulo 06!" << endl;
	cout << "Para controlar os cubos, voce deve utilizar as seguintes teclas:" << endl;
	cout << "===========================================================================" << endl;
	cout << "Selecionar cubo                       : TAB" << endl;
	cout << "Movimentar no eixo X (objeto)         : SETA ESQUERDA | SETA DIREITA" << endl;
	cout << "Movimentar no eixo Y (objeto)         : I | J" << endl;
	cout << "Movimentar no eixo Z (objeto)         : SETA CIMA | SETA BAIXO" << endl;
	cout << "Rotacionar os eixos                   : X | Y | Z" << endl;
	cout << "Pausar/retomar rotacao                : ESPACO" << endl;
	cout << "Aumentar                     		   : ]" << endl;
	cout << "Diminuir                     	       : [" << endl;
	cout << "Ativar e desativar a iluminação       : 1 | 2 | 3" << endl;
	cout << "Adicionar ponto de trajetoria         : P" << endl;
	cout << "Ativar/desativar trajetoria           : T" << endl;
	cout << "Limpar trajetoria                     : C" << endl;
	cout << "Alternar modo material/textura        : M" << endl;
	cout << "Selecionar luz para editar            : L" << endl;
	cout << "Aumentar/diminuir intensidade da luz  : = | -" << endl;
	cout << "Mover camera                          : W | A | S | D" << endl;
	cout << "Olhar (rotacionar camera)             : MOUSE" << endl;
	cout << "Sair                                  : ESC" << endl;
	cout << "===========================================================================" << endl;
	cout << "Quadrado selecionado: "<< selectedObj + 1 << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Trocado o buffer simples, para carregar o OBJ
	int nVertices;
	// Carrega o OBJ
	GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj",nVertices);
	// Carrega MTL
	MTL textureName = loadMTL("../assets/Modelos3D/Suzanne.mtl");
	// Carrega a textura
	GLuint textureID = loadTexture("../assets/Modelos3D/"+ textureName.mapKd);


	glUseProgram(shaderID);

	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f); // matriz identidade
	GLint projLoc = glGetUniformLocation(shaderID, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	GLint viewLoc = glGetUniformLocation(shaderID, "view");
	GLint renderModeLoc = glGetUniformLocation(shaderID, "renderMode");

	// definir a iluminação
	// definindo a posição da luz
	glm::vec3 lightPos(0.0f,0.0f, 1.0f);
	// definindo a posição da camera
	glm::vec3 viewPos(0.0f,0.0f,1.0f);
	// definindo a cor da luz
	glm::vec3 lightColor(1.0f,1.0f,1.0f);

	glUniform3f(glGetUniformLocation(shaderID, "Ka"), textureName.Ka.r, textureName.Ka.g, textureName.Ka.b);
	glUniform3f(glGetUniformLocation(shaderID, "Kd"), textureName.Kd.r, textureName.Kd.g, textureName.Kd.b);
	glUniform3f(glGetUniformLocation(shaderID, "Ks"), textureName.Ks.r, textureName.Ks.g, textureName.Ks.b);
	glUniform1f(glGetUniformLocation(shaderID, "Ns"), textureName.Ns);

	// posição inicial do OBJ
	OBJ obj1;
	obj1.translateX   =  -0.5f;
	obj1.scaleUniform =  0.4f;
	obj1.translateY   =  0.0f;
	obj1.VAO = VAO;
	obj1.nVertices = nVertices;
	obj1.textureID = textureID;

	setupThreePointLighting(obj1);

	obj.push_back(obj1);

	OBJ obj2;
	obj2.translateX   =  0.5f;
	obj2.scaleUniform =  0.4f;
	obj2.translateY   =  0.0f;
	obj2.VAO = VAO;
	obj2.nVertices = nVertices;
	obj2.textureID = textureID;

	setupThreePointLighting(obj2);

	obj.push_back(obj2);

	glEnable(GL_DEPTH_TEST);


	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Calculo do frame a cada loop
		float currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
		
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.75f, 0.75f, 0.75f, 0.75f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Desenha os quadrados em cada canto do triângulo
		// glLineWidth(10);
		// glPointSize(20);

		float globalTime = (GLfloat)glfwGetTime();

		glm::mat4 view = camera.GetViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniform3f(glGetUniformLocation(shaderID, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
		glUniform1i(renderModeLoc, renderMode);

		for (int i =0; i <obj.size(); i++)
		{
			updateTrajectory(obj[i], deltaTime);

			if(!obj[i].paused)
			{
				obj[i].angle = globalTime;
			}

			// Chamada de desenho - drawcall
			// Poligono Preenchido - GL_TRIANGLES
			glBindVertexArray(obj[i].VAO);

			model = glm::mat4(1); 

			// Realiza o translado do cubo
			model = glm::translate(model, glm::vec3(obj[i].translateX, 
													obj[i].translateY, 
													obj[i].translateZ));
			// Realiza a escala do cubo
			model = glm::scale(model, glm::vec3(obj[i].scaleUniform, 
												obj[i].scaleUniform, 
												obj[i].scaleUniform));

			// Posição inicial do cubo
			model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			
			// Realiza a rotação do cubo
			if (obj[i].rotateX)
			{
				model = glm::rotate(model, obj[i].angle, glm::vec3(1.0f, 0.0f, 0.0f));
				
			}
			else if (obj[i].rotateY)
			{
				model = glm::rotate(model, obj[i].angle, glm::vec3(0.0f, 1.0f, 0.0f));

			}
			else if (obj[i].rotateZ)
			{
				model = glm::rotate(model, obj[i].angle, glm::vec3(0.0f, 0.0f, 1.0f));

			}

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			sendLightUniforms(shaderID);
			glBindTexture(GL_TEXTURE_2D, obj[i].textureID);
			glDrawArrays(GL_TRIANGLES, 0, nVertices);

			// Chamada de desenho - drawcall
			// CONTORNO - GL_LINE_LOOP
			// glDrawArrays(GL_POINTS, 0, nVertices);
		}
		glBindVertexArray(0);

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		obj[selectedObj].rotateX = true;
		obj[selectedObj].rotateY = false;
		obj[selectedObj].rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		obj[selectedObj].rotateX = false;
		obj[selectedObj].rotateY = true;
		obj[selectedObj].rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		obj[selectedObj].rotateX = false;
		obj[selectedObj].rotateY = false;
		obj[selectedObj].rotateZ = true;
	}

	// Para a rotação do cubo
	if (glfwGetKey(window, GLFW_KEY_SPACE) && action == GLFW_PRESS)
	{
		obj[selectedObj].paused = !obj[selectedObj].paused;
	}

	// Insersão de lógica para tranladar o cubo na página

	if (glfwGetKey(window,GLFW_KEY_UP) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].translateZ += 0.05f;
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].translateZ -= 0.05f;
	}

	if (glfwGetKey(window,GLFW_KEY_LEFT) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{ 
		obj[selectedObj].translateX -= 0.05f;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].translateX += 0.05f;
	}

	if (glfwGetKey(window, GLFW_KEY_I) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].translateY += 0.05f;
	}

	if (glfwGetKey(window, GLFW_KEY_J) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].translateY -= 0.05f;
	}

	// Inserção de lógica para escalar o cubo na página

	if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET)&& (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].scaleUniform -= 0.05f;

		if(obj[selectedObj].scaleUniform < 0.05f)
		{
			obj[selectedObj].scaleUniform = 0.05f;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		obj[selectedObj].scaleUniform += 0.05f;
	}

	if (glfwGetKey(window, GLFW_KEY_TAB) && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		selectedObj = (selectedObj + 1) % obj.size();
		cout << "Quadrado selecionado: " << selectedObj + 1 << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_1) && action == GLFW_PRESS)
	{
		keyLight.enabled = !keyLight.enabled;
		cout << "Key Light: " << (keyLight.enabled ? "ON" : "OFF") << endl;
	}

	if (glfwGetKey(window, GLFW_KEY_2) && action == GLFW_PRESS)
	{
		fillLight.enabled = !fillLight.enabled;
		cout << "Fill Light: " << (fillLight.enabled ? "ON" : "OFF") << endl;
	}

	if (glfwGetKey(window, GLFW_KEY_3) && action == GLFW_PRESS)
	{
		backLight.enabled = !backLight.enabled;
		cout << "Back Light: " << (backLight.enabled ? "ON" : "OFF") << endl;
	}

	// Adiciona a posição atual do objeto selecionado como um novo ponto de trajetória
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		glm::vec3 point(obj[selectedObj].translateX, obj[selectedObj].translateY, obj[selectedObj].translateZ);
		obj[selectedObj].trajectory.push_back(point);
		cout << "Ponto adicionado a trajetoria do objeto " << selectedObj + 1
			<< ": (" << point.x << ", " << point.y << ", " << point.z << ")"
			<< " | Total de pontos: " << obj[selectedObj].trajectory.size() << endl;
	}

	// Ativa/desativa o seguimento da trajetória para o objeto selecionado
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		if (obj[selectedObj].trajectory.size() < 2)
		{
			cout << "Adicione ao menos 2 pontos antes de ativar a trajetoria (tecla P)." << endl;
		}
		else
		{
			obj[selectedObj].followTrajectory = !obj[selectedObj].followTrajectory;
			obj[selectedObj].currentPoint = 0;
			obj[selectedObj].trajectoryT = 0.0f;
			cout << "Trajetoria do objeto " << selectedObj + 1 << ": "
				<< (obj[selectedObj].followTrajectory ? "ATIVADA" : "DESATIVADA") << endl;
		}
	}

	// Limpa a trajetória do objeto selecionado
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		obj[selectedObj].trajectory.clear();
		obj[selectedObj].followTrajectory = false;
		obj[selectedObj].currentPoint = 0;
		obj[selectedObj].trajectoryT = 0.0f;
		cout << "Trajetoria do objeto " << selectedObj + 1 << " foi limpa." << endl;
	}
	if (key == GLFW_KEY_M && action == GLFW_PRESS)
	{
		renderMode = (renderMode + 1) % 3;
		string modos[3] = {"Phong + Textura", "Somente Material ", "Somente Textura"};
		cout << "Modo de visualizacao: " << modos[renderMode] << endl;
	}
	// Seleciona qual luz será editada
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		selectedLight = (selectedLight + 1) % 3;
		string nomes[3] = {"Key Light", "Fill Light", "Back Light"};
		cout << "Luz selecionada para edicao: " << nomes[selectedLight] << endl;
	}
	// Aumenta a intensidade (cor) da luz selecionada
	if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		Light &l = getSelectedLight();
		l.color *= 1.1f;
		cout << "Intensidade: (" << l.color.x << ", " << l.color.y << ", " << l.color.z << ")" << endl;
	}

	// Diminui a intensidade (cor) da luz selecionada
	if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		Light &l = getSelectedLight();
		l.color *= 0.9f;
		cout << "Intensidade: (" << l.color.x << ", " << l.color.y << ", " << l.color.z << ")" << endl;
	}
}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Parser do MTL 
MTL loadMTL (string filtePATH)
{
	MTL mtl;
	ifstream arqEntrada(filtePATH.c_str());
	if (!arqEntrada.is_open())
	{
		cerr << "Erro ao ler o arquivo " << filtePATH << endl;
		return MTL{};
	}
	string line;
	while (getline(arqEntrada, line))
	{
		istringstream ssline(line);
		string word;
		ssline >> word;
		if (word == "Ns")
		{
			ssline >> mtl.Ns;
		}
		else if (word == "Ka")
		{
			ssline >> mtl.Ka.r >> mtl.Ka.g >> mtl.Ka.b;
		}
		else if (word == "Ks")
		{
			ssline >> mtl.Ks.r >> mtl.Ks.g >> mtl.Ks.b;
		}
		else if (word == "Ke")
		{
			ssline >> mtl.Ke.r >> mtl.Ke.g >> mtl.Ke.b;
		}
		else if (word == "Kd")
		{
			ssline >> mtl.Kd.r >> mtl.Kd.g >> mtl.Kd.b;
		}
		else if (word == "Ni")
		{
			ssline >> mtl.Ni;
		}
		else if(word == "d")
		{
			ssline >> mtl.d;
		}
		else if(word == "illum")
		{
			ssline >> mtl.illum;
		}
		else if(word == "map_Kd")
		{
			ssline >> mtl.mapKd;
		}
	}

	return mtl;
}

// Removido o setupGeometry, pois inserimos o carregamento do LoadSimpleOBJ que o substitui
int loadSimpleOBJ(string filePath, int &nVertices)


 {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

    std::ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open()) 
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePath << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) 
	{
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") 
		{
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } 
        else if (word == "vt") 
		{
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } 
        else if (word == "vn") 
		{
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
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
				// Inserido as coordenadas da textura
				vBuffer.push_back(texCoords[ti].s);
				vBuffer.push_back(1.0f -texCoords[ti].t);
				// inserindo os normais para iluminação
				vBuffer.push_back(normals[ni].x);
				vBuffer.push_back(normals[ni].y);
				vBuffer.push_back(normals[ni].z);

            }
        }
    }

    arqEntrada.close();

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

	// Location 2 - tex_coord
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// Location 3 - normals
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);
		
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	nVertices = vBuffer.size() / 11;  // x, y, z, r, g, b, s, t, ni.x, ni.y, ni.z (valores atualmente armazenados por vértice) 11 vertices

    return VAO;
}

// Carregamento da textura
GLuint loadTexture(string filePath)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int width, height, nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

// Mouse callback para movimentar a camera
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // invertido: y cresce de baixo pra cima na tela

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.Rotate(xoffset, yoffset);
}

// Função para movimentar a
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.Move(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.Move(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.Move(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.Move(RIGHT, deltaTime);
}

// Função para configurar os três pontos de luz 
void setupThreePointLighting(OBJ &object)
{
    glm::vec3 objPos(object.translateX, object.translateY, object.translateZ);
    float scale = object.scaleUniform;

    // Key light: na frente, à direita, mais alta — a mais intensa
    keyLight.position = objPos + glm::vec3(2.0f, 2.0f, 2.0f) * scale;
    keyLight.color = glm::vec3(1.0f, 1.0f, 1.0f); // branca, forte

    // Fill light: na frente, à esquerda, mais baixa — mais fraca, suaviza sombra
    fillLight.position = objPos + glm::vec3(-2.0f, 0.5f, 1.5f) * scale;
    fillLight.color = glm::vec3(0.4f, 0.4f, 0.4f); // mais fraca

    // Back light: atrás do objeto — separa do fundo
    backLight.position = objPos + glm::vec3(0.0f, 1.5f, -2.5f) * scale;
    backLight.color = glm::vec3(0.6f, 0.6f, 0.6f); // intensidade média
}

// Função para enviar o shader
void sendLightUniforms(GLuint shaderID)
{
    glUniform3f(glGetUniformLocation(shaderID, "keyLightPos"), keyLight.position.x, keyLight.position.y, keyLight.position.z);
    glUniform3f(glGetUniformLocation(shaderID, "keyLightColor"), keyLight.color.x, keyLight.color.y, keyLight.color.z);
    glUniform1i(glGetUniformLocation(shaderID, "keyLightEnabled"), keyLight.enabled);

    glUniform3f(glGetUniformLocation(shaderID, "fillLightPos"), fillLight.position.x, fillLight.position.y, fillLight.position.z);
    glUniform3f(glGetUniformLocation(shaderID, "fillLightColor"), fillLight.color.x, fillLight.color.y, fillLight.color.z);
    glUniform1i(glGetUniformLocation(shaderID, "fillLightEnabled"), fillLight.enabled);

    glUniform3f(glGetUniformLocation(shaderID, "backLightPos"), backLight.position.x, backLight.position.y, backLight.position.z);
    glUniform3f(glGetUniformLocation(shaderID, "backLightColor"), backLight.color.x, backLight.color.y, backLight.color.z);
    glUniform1i(glGetUniformLocation(shaderID, "backLightEnabled"), backLight.enabled);
}


// Avalia um ponto em uma curva de Bézier cúbica, dados os 4 pontos de controle, no parâmetro t (0 a 1)
glm::vec3 cubicBezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
	float u = 1.0f - t;
	return (u * u * u) * p0
		 + (3.0f * u * u * t) * p1
		 + (3.0f * u * t * t) * p2
		 + (t * t * t) * p3;
}

// Função para calcular a trajetória
void updateTrajectory(OBJ &object, float deltaTime)
{
	int n = (int)object.trajectory.size();
	if (!object.followTrajectory || n < 2)
		return;

	float speed = 1.0f;        // unidades por segundo
	float curveTension = 0.2f; // controla o "arredondamento" da curva

	int i0 = object.currentPoint;
	int i1 = (i0 + 1) % n;
	int iPrev = (i0 - 1 + n) % n;
	int iNext = (i1 + 1) % n;

	glm::vec3 p0    = object.trajectory[i0];
	glm::vec3 p1    = object.trajectory[i1];
	glm::vec3 pPrev = object.trajectory[iPrev];
	glm::vec3 pNext = object.trajectory[iNext];

	// Handles calculados pela tangente local (curva passa pelos pontos originais)
	glm::vec3 h1 = p0 + curveTension * (p1 - pPrev);
	glm::vec3 h2 = p1 - curveTension * (pNext - p0);

	float segmentLength = glm::length(p1 - p0);
	if (segmentLength < 0.0001f) segmentLength = 0.0001f;

	object.trajectoryT += (speed * deltaTime) / segmentLength;

	if (object.trajectoryT >= 1.0f)
	{
		object.trajectoryT = 0.0f;
		object.currentPoint = (object.currentPoint + 1) % n;
	}

	glm::vec3 pos = cubicBezier(p0, h1, h2, p1, object.trajectoryT);

	object.translateX = pos.x;
	object.translateY = pos.y;
	object.translateZ = pos.z;
}