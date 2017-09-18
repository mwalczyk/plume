@echo off
del "../shaders/*.spv"
for %%f in (../shaders/*.vert) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -o ../shaders/%%~nf_vert.spv ../shaders/%%f
for %%f in (../shaders/*.tesc) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -o ../shaders/%%~nf_tesc.spv ../shaders/%%f
for %%f in (../shaders/*.tese) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -o ../shaders/%%~nf_tese.spv ../shaders/%%f
for %%f in (../shaders/*.geom) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -o ../shaders/%%~nf_geom.spv ../shaders/%%f
for %%f in (../shaders/*.frag) do %VULKAN_SDK%/Bin/glslangValidator.exe -V -o ../shaders/%%~nf_frag.spv ../shaders/%%f
