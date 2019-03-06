#-I /usr/src/kernels/4.19.9-300.fc29.x86_64/arch/x86/include/asm/ -I /usr/src/kernels/4.19.9-300.fc29.x86_64/arch/x86/include/
INC= -I /usr/include/python3.7m 

LIBS=-L /usr/lib64 \
	 -L /usr/lib64/python3.7/lib-dynload

LIB=-lpython3.7m\
	-lboost_filesystem \
	-lboost_system \
	-lboost_python

BOSS: ./src/*.cpp
	g++ -std=c++17 $(LIBS) $(INC) ./src/*.cpp -o ./bin/BOSS_main $(LIB)
