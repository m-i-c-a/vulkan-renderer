${VULKAN_SDK}/bin/glslc glsl/std.vert -o spirv/std.vert.spv
${VULKAN_SDK}/bin/glslc glsl/std.frag -o spirv/std.frag.spv

${VULKAN_SDK}/bin/glslc glsl/depth_pass.vert -o spirv/depth_pass.vert.spv
${VULKAN_SDK}/bin/glslc glsl/depth_pass.frag -o spirv/depth_pass.frag.spv