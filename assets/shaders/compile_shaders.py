import sys
import os
import glob
import subprocess

if len(sys.argv) < 2:
	sys.exit('Please provide a target directory')

if not os.path.exists(sys.argv[1]):
	sys.exit('{} is not a valid directory'.format(sys.argv[1]))

path = sys.argv[1]

# gather all shader files in the given directory
shaderfiles = []
for exts in ('*.vert', '*.frag', '*.comp', '*.geom', '*.tesc', '*.tese'):
	shaderfiles.extend(glob.glob(os.path.join(path, exts)))

# compile to spir-v
failed_shaders = []
for shaderfile in shaderfiles:
	print('\n-------- {} --------\n'.format(shaderfile))
	vulkan_sdk_path = os.path.expandvars('$VULKAN_SDK')
	if subprocess.call('{}/bin/glslangValidator -V {} -o {}.spv'.format(vulkan_sdk_path, shaderfile, shaderfile), shell=True) != 0:
		failed_shaders.append(shaderfile)

# print results
print("\n-------- Compilation result --------\n")
if len(failed_shaders) == 0:
	print("SUCCESS: All shaders compiled to SPIR-V")
else:
	print('ERROR: {} shader(s) could not be compiled:\n'.format(len(failed_shaders)))
	for failed in failed_shaders:
		print('\t' + failed)
