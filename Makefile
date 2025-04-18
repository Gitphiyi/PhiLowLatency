all: build run

build:
	@echo "Building..."
	@clang++ -O0 -std=c++11 simd.cpp -o simd
	@echo "Complete!"

run: 
	@./simd
