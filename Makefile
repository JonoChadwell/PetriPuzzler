CARGS = -Os -Wall -Isrc -Ilibraries -isystem C:\raylib\src -DPLATFORM_WEB
LINKARGS = -s USE_GLFW=3 -LC:\raylib\src\libraylib.a
SRCFILES = petri_main

info:
	@echo Common targets: buildall, clean

build/%.o: src/%.c src/*.h
	@if not exist build mkdir build
	emcc $(CARGS) -c $< -o $@

buildall: $(addprefix build/, $(addsuffix .o, $(SRCFILES)))
	emcc $(CARGS) $(LINKARGS) $? C:\raylib\src\libraylib.a --shell-file C:\emsdk\upstream\emscripten\src\shell.html  -o build\petri_demo.html