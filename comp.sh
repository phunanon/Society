#sudo apt install libsfml-dev
g++ -c Society.cpp -std=c++14 -g #-O3
#g++ -c raycast.cpp -std=c++11 -O3
g++ Society.o -o Society.elf -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -fno-exceptions #-static-libstdc++ #-pthread
#g++ Society.cpp -o Society.elf --std=c++11 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio #-static-libstdc++
