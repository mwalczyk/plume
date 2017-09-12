@echo off
del "../shaders/*.spv"
for %%f in (../shaders/*.vert) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -H -o ../shaders/%%~nf_vert.spv ../shaders/%%f
for %%f in (../shaders/*.frag) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -H -o ../shaders/%%~nf_frag.spv ../shaders/%%f

