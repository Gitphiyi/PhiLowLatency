all: build run

build:
	@echo "Building..."
	@clang++ -std=c++11 simd.cpp -o simd
	@echo "Complete!"

run: 
	@./simd
