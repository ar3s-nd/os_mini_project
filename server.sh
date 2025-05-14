mkdir -p bin
gcc src/server.c src/functions.c src/mutex.c -o bin/server
./bin/server