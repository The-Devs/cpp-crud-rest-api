# # Shameless copy paste from tj/luna project
# # https://github.com/tj/luna/blob/master/Makefile

OUT = crud
CFLAGS := $(CFLAGS) -lmysqlcppconn -pthread
CPP_FILES := $(CPP_FILES) app.cpp
# g++ -std=c++11 -I /usr/local/include/mysql-connector/include/jdbc/ app.cpp -o crud -lmysqlcppconn -pthread
compile_crud:
		g++ -std=c++11 -I /usr/local/include/mysql-connector/include/jdbc/ $(CPP_FILES) -o $(OUT) $(CFLAGS)