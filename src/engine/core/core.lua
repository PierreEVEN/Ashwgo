declare_module("core", {"types"}, {"vulkan-loader", "directxshadercompiler", "vulkan-validationlayers", "vulkan-memory-allocator", "spirv-reflect", "glfw", {name = "glm", public = true}, "imgui docking", "stb"}, false)

target("core")
set_group("engine")