# # Shameless copy paste from tj/luna project
# # https://github.com/tj/luna/blob/master/Makefile

# SRC = $(wildcard src/*.c)
# OBJ = ${SRC:.c=.o}
OUT = crud
CFLAGS := $(CFLAGS) -lmysqlcppconn -pthread
CPP_FILES := $(CPP_FILES) app.cpp
# FLAGS="lmysqlcppconn pthread"
# F_STR=""
# INCLUDE=("/usr/local/include/mysql-connector/include/jdbc/")
# echoing:
# 	echo $(FLAGS) | sed 's/\s/ -/g' | sed 's/^./-/'

compile_crud:
		g++ -std=c++11 -I /usr/local/include/mysql-connector/include/jdbc/ $(CPP_FILES) -o $(OUT) $(CFLAGS)