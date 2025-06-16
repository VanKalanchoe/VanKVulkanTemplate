@echo OFF

cmake -S .. -B ..\build\win
::cmake -DUSE_SHADER_LANGUAGE=GLSL ..
pause