# Projeto de Computação Gráfica — GB

> Aplicação desenvolvida para a disciplina de **Processamento Gráfico / Computação Gráfica** — Unisinos.

A aplicação renderiza modelos 3D carregados de arquivos `.obj`/`.mtl`, com:

- Iluminação de **Phong** em esquema de três pontos (key, fill e back light)
- Câmera em **primeira pessoa** com look-around via mouse
- Alternância entre material, textura e modo combinado
- Seleção e transformação de objetos (translação, rotação, escala)
- Animação por trajetória com **curvas de Bézier cúbicas**

---

## Setup

### Dependências

| Biblioteca | Função |
|---|---|
| [GLFW](https://www.glfw.org/) | Gerenciamento de janelas e inputs |
| [GLAD](https://glad.dav1d.de/) | Carregamento das funções do OpenGL |
| [GLM](https://glm.g-truc.net/) | Biblioteca matemática para transformações 3D |
| [stb_image](https://github.com/nothings/stb) | Carregamento das texturas |

### Estrutura de pastas esperada

```
projeto/
├── GrauB/
│   └── main.cpp
├── assets/
│   └── Modelos3D/
│       ├── Suzanne.obj
│       ├── Suzanne.mtl
│       └── [textura referenciada no map_Kd]
└── CMakeLists.txt
```

### Compilação

```bash
# 1. Entrar na pasta do projeto
cd projeto

# 2. Gerar os arquivos de build
cmake -B build

# 3. Compilar
cmake --build build

# 4. Executar
cd build
./cubo3d
```

> Para recompilar após editar o código, basta rodar `cmake --build build` novamente.

#### Windows (MinGW)

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
build\GB.exe
```

---

## Controles

| Tecla | Ação |
|---|---|
| `TAB` | Selecionar objeto |
| `←` / `→` | Translação no eixo X do objeto |
| `I` / `J` | Translação no eixo Y do objeto |
| `↑` / `↓` | Translação no eixo Z do objeto |
| `X` / `Y` / `Z` | Define eixo de rotação do objeto |
| `Espaço` | Pausa/retoma rotação |
| `[` / `]` | Diminui/aumenta escala uniforme |
| `1` / `2` / `3` | Liga/desliga key light / fill light / back light |
| `L` | Seleciona luz para editar intensidade |
| `=` / `-` | Aumenta/diminui intensidade da luz selecionada |
| `M` | Alterna modo de visualização (Phong+textura / só material / só textura) |
| `P` | Adiciona ponto de trajetória na posição atual do objeto |
| `T` | Ativa/desativa trajetória do objeto selecionado |
| `C` | Limpa a trajetória do objeto selecionado |
| `W` `A` `S` `D` | Movimenta a câmera |
| 🖱️ Mouse | Rotaciona a câmera (look-around) |
| `ESC` | Sai da aplicação |

---

## Assets

- **Suzanne** (`.obj`/`.mtl`): modelo geométrico clássico (a macaca mascote do software Blender), disponibilizado no material da aula.
- **Texturas**: carregadas a partir da leitura do arquivo `.mtl` fornecido pelo professor.

---

## Referências

- **[Learn OpenGL](https://learnopengl.com/)** — guia principal utilizado para os conceitos de câmera, texturização e iluminação (modelo de Phong).
- **[Documentação GLFW](https://www.glfw.org/documentation.html)** — consultada para gerenciamento de callbacks de teclado e mouse.
- **[Documentação GLM](https://glm.g-truc.net/0.9.9/index.html)** — consultada para funções de matrizes de projeção, visualização (`lookAt`) e rotações.
- **Materiais da disciplina** — notas de aula e snippets de código fornecidos ao longo do semestre na disciplina de Computação Gráfica — Unisinos.
