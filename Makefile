# O nome das variáveis são totalmente arbitrárias mas segui um padrão de nomenclatura comum.
# CC - Compilador
CC = g++
# BIN_FILE - nome do arquivo binário resultante
BIN_FILE = cra-srv 
# BIN_TEST - nome do arquivo binário de teste
BIN_TEST = cras-test
# TEST_DIR - diretório para arquivos de testes
TEST_DIR = tests
# TEST_FILES - arquivos.cpp para testes
TEST_FILES = $(wildcard $(TEST_DIR)/*.cpp)
# CC_FLAGS - opções de compilação geral
CC_FLAGS = -std=c++11 -Ilibs
# CC_TEST_FLAGS - opções de compilação para tetes
CC_TEST_FLAGS = $(CC_FLAGS) -Wall
# CC_FLAGS - opções de compilação para o app
CC_APP_FLAGS = $(CC_FLAGS)
# CPP_FILES - arquivos.cpp
CPP_FILES = $(wildcard src/*.cpp)
# HPP_FILES - arquivos.hpp
HPP_FILES = $(wildcard src/*.hpp)
# LIBS_HEADER - arquivos.hpp
LIBS_HEADER = $(wildcard libs/*.hpp)
# OBJ_FILES - arquivos .o oriundas da compilação
OBJ_FILES = $(CPP_FILES:.cpp=.o)
# OBJ_DIR - diretório para arquivos de objetos .o
OBJ_DIR = obj

all: objDir $(BIN_FILE)

$(BIN_FILE): $(OBJ_FILES)
	@ echo "Construindo binário a partir do linker do GCC: $@"
	$(CC) $^ -o $@
	@ echo " "

$(OBJ_DIR)/%.o: src/%.cpp src/%.hpp
	@ echo "Construindo objetos a partir do compilador do GCC: $<"
	$(CC) $< $(CC_APP_FLAGS) -o $@
	@ echo " "

$(OBJ_DIR)/main.o: src/main.cpp $(LIBS_HEADER) $(HPP_FILES)
	@ echo "Construindo objetos a partir do compilador do GCC: $<"
	$(CC) $< $(CC_APP_FLAGS) -o $@
	@ echo " "

objDir:
	@ echo "Criando diretório $(OBJ_DIR)"
	@ mkdir -p $(OBJ_DIR)

clean:
	@ echo "Removendo arquivos em $(OBJ_DIR)"
	@ echo "Removendo arquivos de backup"
	@ echo "Removendo arquivo $(BIN_FILE)"
	@ rm -rf $(OBJ_DIR) $(BIN_FILE) *~
	@ echo "Removendo arquivos de teste"
	@ rm $(BIN_TEST)

test: $(BIN_TEST)
	@ echo "Iniciando testes"
	@ ./$(BIN_TEST) --reporter console --success

log-test: $(BIN_TEST)
	@ echo "Iniciando testes..."
	@ echo "Armazenando em $(TEST_DIR)/testlog.txt"
	@ ./$(BIN_TEST) --reporter console --success > "$(TEST_DIR)/testlog.txt"

$(BIN_TEST): $(TEST_FILES)
	@ echo "Construindo binário de teste a partir do linker do GCC: $@"
	$(CC) $^ $(CC_TEST_FLAGS) -o $@
	@ echo " "

.PHONY: all clean test log-test
