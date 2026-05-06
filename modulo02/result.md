# Módulo 02 -  Instanciando objetos na cena 3D

Neste módulo, a geometria da pirâmide do projeto base foi substituída por um cubo, composto
por 12 triângulos (2 por face) e 36 vértices. Cada face recebeu uma cor distinta para
facilitar a visualização sem o uso de texturas ou iluminação.
 
Foram adicionados controles de translação nos três eixos (WASD para X e Z, IJ para Y)
e de escala uniforme ([ para diminuir, ] para aumentar), mantendo a rotação original em X, Y e Z.
 
Por fim, foram instanciados 3 cubos na cena, cada um com transformações independentes.
A seleção do cubo ativo é feita pelas teclas 1, 2 e 3.

# Como executar o projeto

## Dependências

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install cmake build-essential libglfw3-dev libglm-dev
```
### macOS
```bash
brew install cmake glfw glm
```

---

## GLAD

1. Acesse [glad.dav1d.de](https://glad.dav1d.de/) e configure:
   - Language: **C/C++**
   - API gl: **3.3**
   - Profile: **Core**
2. Clique em *Generate* e baixe o zip
3. Extraia a pasta `glad/` na raiz do projeto

---

## Estrutura esperada

```
projeto/
├── CMakeLists.txt
├── glad/
│   ├── include/
│   └── src/glad.c
└── src/
    └── main.cpp
```

---

## Compilar e executar

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

### Windows (MinGW)
```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
build\cubo3d.exe
```

---

# 🎮 Controles

## Selecionar cubo

| Tecla | Ação |
|---|---|
| `1` | Seleciona o cubo da esquerda |
| `2` | Seleciona o cubo do centro |
| `3` | Seleciona o cubo da direita |

> Todas as transformações abaixo atuam sobre o cubo selecionado.

---

## Rotação

| Tecla | Eixo |
|---|---|
| `X` | Rotaciona no eixo X |
| `Y` | Rotaciona no eixo Y |
| `Z` | Rotaciona no eixo Z |

---

## Translação

| Tecla | Ação |
|---|---|
| `W` | Move para frente (eixo Z) |
| `S` | Move para trás (eixo Z) |
| `A` | Move para a esquerda (eixo X) |
| `D` | Move para a direita (eixo X) |
| `I` | Move para cima (eixo Y) |
| `J` | Move para baixo (eixo Y) |

---

## Escala

| Tecla | Ação |
|---|---|
| `[` | Diminui o cubo |
| `]` | Aumenta o cubo |

> A escala mínima é `0.05` para evitar que o cubo desapareça.

---

## Geral

| Tecla | Ação |
|---|---|
| `ESC` | Encerra o programa |