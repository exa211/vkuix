@echo off
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe default.vert --target-env=vulkan1.2 -o default.vert.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe default.frag --target-env=vulkan1.2 -o default.frag.spv
echo Compiled Shaders