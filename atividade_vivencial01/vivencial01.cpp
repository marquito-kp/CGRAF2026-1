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

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
// adicionados uniforms de view e projection para perspectiva correta com múltiplos cubos
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"       //  uniform de câmera (view matrix)
"uniform mat4 projection;\n" // uniform de projeção perspectiva
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = projection * view * model * vec4(position, 1.0);\n" 
"finalColor = vec4(color, 1.0);\n"
"}\0";

//Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

// instacia o OBJ

int loadSimpleOBJ(string filePATH, int &nVertices)
 {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);
    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) 
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
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
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	nVertices = vBuffer.size() / 6;  // x, y, z, r, g, b (valores atualmente armazenados por vértice)

    return VAO;
}

struct Object3D{
	GLuint VAO;
	int numVertices;
    float translateX = 0.0f;
	float translateY = 0.0f;
	float translateZ = 0.0f;
	float scaleUniform = 1.0f;
	bool rotateX = false;
    bool rotateY = false;
    bool rotateZ = false;
};

// array com as 3 instâncias de cubo na cena
std::vector<Object3D> objects;
// índice do cubo atualmente selecionado (0, 1 ou 2)
int selectedObject = 0;


// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Marcos Krol Pacheco!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// imprime no terminal os controles disponíveis para o usuário movimentar os cubos
	cout << "\n=== CONTROLES ===" << endl;
	cout << "Alternar entre os cubos: tecla TAB" << endl;
	cout << "Rotacao         : X, Y, Z" << endl;
	cout << "Mover XZ        : W (frente) S (tras) A (esq) D (dir)" << endl;
	cout << "Mover Y         : I (sobe) J (desce)" << endl;
	cout << "Escala          : [ (diminui)  ] (aumenta)" << endl;
	cout << "Sair            : ESC" << endl;
	cout << "=================" << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando um buffer simples, com a geometria de um triângulo
	GLuint VAO = setupGeometry();


	glUseProgram(shaderID);

	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shaderID, "model");

	// obtendo as localizações dos novos uniforms view e projection
	GLint viewLoc       = glGetUniformLocation(shaderID, "view");
	GLint projectionLoc = glGetUniformLocation(shaderID, "projection");

	// configurando a câmera com lookAt: posicionada acima e à frente da cena
	glm::mat4 view = glm::lookAt(
		glm::vec3(0.0f, 3.0f, 7.0f),  // posição da câmera
		glm::vec3(0.0f, 0.0f, 0.0f),  // ponto que a câmera olha
		glm::vec3(0.0f, 1.0f, 0.0f)   // vetor "up"
	);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// projeção perspectiva para dar sensação de profundidade
	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		(float)WIDTH / (float)HEIGHT,
		0.1f,
		100.0f
	);
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glEnable(GL_DEPTH_TEST);

	// carregando o OBJ
	
	int nVertices;
	GLuint objVAO = loadSimpleOBJ("../assets/Modelos3D/Cube.obj", nVertices);

	Object3D obj1;
	obj1.VAO = objVAO;
	obj1.numVertices = nVertices;
	obj1.translateX = -3.0f;
	obj1.translateY = 0.0f;
	obj1.translateZ = 0.0f;
	obj1.scaleUniform = 1.0f;

	objects.push_back(obj1);

	Object3D obj2;
	obj2.VAO = objVAO;
	obj2.numVertices = nVertices;
	obj2.translateX = 3.0f;
	obj2.translateY = 0.0f;
	obj2.translateZ = 0.0f;
	obj2.scaleUniform = 1.0f;

	objects.push_back(obj2);


	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		// loop que desenha cada instância de cubo com sua própria matrix model
		for (int i = 0; i < objects.size(); i++)
		{
			model = glm::mat4(1);

			// aplica translação individual de cada instância
			model = glm::translate(model, glm::vec3(
				objects[i].translateX,
				objects[i].translateY,
				objects[i].translateZ
			));

			// aplica escala uniforme individual de cada instância
			model = glm::scale(model, glm::vec3(
				objects[i].scaleUniform,
				objects[i].scaleUniform,
				objects[i].scaleUniform
			));

			// aplica rotação individual de cada instância (mantém lógica original)
			if (objects[i].rotateX)
			{
				model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else if (objects[i].rotateY)
			{
				model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
			}
			else if (objects[i].rotateZ)
			{
				model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
			}

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glBindVertexArray(objects[i].VAO);
			glDrawArrays(GL_TRIANGLES, 0, objects[i].numVertices);
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
		// rotação atua sobre o cubo selecionado individualmente
		objects[selectedObject].rotateX = true;
		objects[selectedObject].rotateY = false;
		objects[selectedObject].rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		objects[selectedObject].rotateX = false;
		objects[selectedObject].rotateY = true;
		objects[selectedObject].rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		objects[selectedObject].rotateX = false;
		objects[selectedObject].rotateY = false;
		objects[selectedObject].rotateZ = true;
	}

	// seleção do cubo ativo pelas teclas 1, 2, 3
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		selectedObject = (selectedObject + 1) % objects.size();
		cout << "Objeto " << (selectedObject + 1) << " selecionado" << endl;
	}

	// translação no eixo X e Z com WASD (aplicada ao cubo selecionado)
	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].translateX += 0.1f;

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].translateX -= 0.1f;

	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].translateZ -= 0.1f;

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].translateZ += 0.1f;

	// translação no eixo Y com I (sobe) e J (desce) para o cubo selecionado
	if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].translateY += 0.1f;

	if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].translateY -= 0.1f;

	// escala uniforme com [ (diminui) e ] (aumenta) para o cubo selecionado
	if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		objects[selectedObject].scaleUniform -= 0.05f;
		// impede escala negativa ou nula
		if (objects[selectedObject].scaleUniform < 0.05f)
			objects[selectedObject].scaleUniform = 0.05f;
	}

	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
		objects[selectedObject].scaleUniform += 0.05f;
}

//Esta função está bastante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no início deste arquivo
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

// Esta função está bastante hardcoded - objetivo é criar os buffers que armazenam a 
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser armazenado em um VBO único ou em VBOs separados

	// substituída a geometria da pirâmide pela geometria de um cubo.
	GLfloat vertices[] = {

		// ============================================================
		// FACE FRONTAL (z = +0.5) — COR: VERMELHO
		// ============================================================
		-0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 0.2f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 0.2f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.2f, 0.2f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.2f, 0.2f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.2f, 0.2f,
		-0.5f, -0.5f,  0.5f,  1.0f, 0.2f, 0.2f,

		// ============================================================
		// FACE TRASEIRA (z = -0.5) — COR: VERDE
		// ============================================================
		 0.5f, -0.5f, -0.5f,  0.2f, 1.0f, 0.2f,
		-0.5f, -0.5f, -0.5f,  0.2f, 1.0f, 0.2f,
		-0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 0.2f,

		-0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 0.2f,
		 0.5f,  0.5f, -0.5f,  0.2f, 1.0f, 0.2f,
		 0.5f, -0.5f, -0.5f,  0.2f, 1.0f, 0.2f,

		// ============================================================
		// FACE ESQUERDA (x = -0.5) — COR: AZUL
		// ============================================================
		-0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,

		-0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,

		// ============================================================
		// FACE DIREITA (x = +0.5) — COR: AMARELO
		// ============================================================
		 0.5f, -0.5f,  0.5f,  1.0f, 0.9f, 0.1f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.9f, 0.1f,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.9f, 0.1f,

		 0.5f,  0.5f, -0.5f,  1.0f, 0.9f, 0.1f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.9f, 0.1f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.9f, 0.1f,

		// ============================================================
		// FACE SUPERIOR (y = +0.5) — COR: CIANO
		// ============================================================
		-0.5f,  0.5f,  0.5f,  0.1f, 0.9f, 0.9f,
		 0.5f,  0.5f,  0.5f,  0.1f, 0.9f, 0.9f,
		 0.5f,  0.5f, -0.5f,  0.1f, 0.9f, 0.9f,

		 0.5f,  0.5f, -0.5f,  0.1f, 0.9f, 0.9f,
		-0.5f,  0.5f, -0.5f,  0.1f, 0.9f, 0.9f,
		-0.5f,  0.5f,  0.5f,  0.1f, 0.9f, 0.9f,

		// ============================================================
		// FACE INFERIOR (y = -0.5) — COR: MAGENTA
		// ============================================================
		-0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,
		 0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,
		 0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,

		 0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,
		-0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,
		-0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,
	};

	GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);


	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}