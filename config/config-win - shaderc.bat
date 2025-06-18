@echo OFF

cmake -DUSE_SHADER_LANGUAGE=GLSL -S .. -B ..\build\win
::cmake -DUSE_SHADER_LANGUAGE=GLSL ..
pause