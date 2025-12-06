
add_rules("mode.debug", "mode.release")

set_project("mo_yanxi.vulkan_wrapper")

if is_plat("windows") then
    if is_mode("debug") then
        set_runtimes("MDd")
    else
        set_runtimes("MD")
    end
else
    set_runtimes("c++_shared")
end

includes("external/**/xmake.lua");

target("mo_yanxi.vulkan_wrapper")
    set_kind("static")
    set_languages("c++23")
    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")

    if is_mode("debug") then
        add_defines("MO_YANXI_VULKAN_WRAPPER_ENABLE_CHECK=1")
    else
        add_defines("MO_YANXI_VULKAN_WRAPPER_ENABLE_CHECK=0")
        set_optimize("faster")
    end

    add_deps("mo_yanxi.utility")

    add_files("./src/vulkan_wrapper/**.cpp")
    add_files("./src/vulkan_wrapper/**.ixx", {public = true})

    add_includedirs("external/VulkanMemoryAllocator/include", {public = true})

    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if not vulkan_sdk then
        raise("Vulkan SDK not found!")
    end
    add_includedirs(path.join(vulkan_sdk, "Include"), {public = true})
    add_linkdirs(path.join(vulkan_sdk, "Lib"), {public = true})
    add_links("vulkan-1", {public = true})
    add_defines("VK_USE_64_BIT_PTR_DEFINES=1", {public = true})
target_end()


target("mo_yanxi.vulkan_wrapper.test")
    set_extension(".exe")
    set_kind("binary")
    set_languages("c++latest")


    add_deps("mo_yanxi.vulkan_wrapper")
    add_files("main.cpp")
target_end()

includes("./external/mo_yanxi_utility/xmake2cmake.lua")