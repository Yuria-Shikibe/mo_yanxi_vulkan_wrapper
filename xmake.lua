add_rules("mode.debug", "mode.release")
set_arch("x64")
set_encodings("utf-8")
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

includes("**/config.lua");

mo_yanxi_utility_import_default("mo_yanxi.utility")
target("mo_yanxi.utility")
    add_vectorexts("avx", "avx2")
target_end()

target("mo_yanxi.vulkan_wrapper")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++latest")
    set_policy("build.c++.modules", true)

    set_warnings("all")
    set_warnings("pedantic")
    add_vectorexts("avx", "avx2")

    add_deps("mo_yanxi.utility")

    add_files("./src/vulkan_wrapper/**.cpp")
    add_files("./src/vulkan_wrapper/**.ixx")
    add_files("main.cpp")

    if is_mode("debug") then
        add_defines("MO_YANXI_VULKAN_WRAPPER_ENABLE_CHECK=1")
    else
        add_defines("MO_YANXI_VULKAN_WRAPPER_ENABLE_CHECK=0")
    end

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


task("gen_ide_hintonly_cmake")
set_menu({
    usage = "xmake gen_ide_hintonly_cmake [options]",
    description = "生成 CMakeLists.txt 并注入指定的 C++ 标准",
    options = {
        {'s', "std", "kv", "23", "指定 C++ 标准版本 (例如: 17, 20, 23). 默认为 23."}
    }
})

on_run(function ()
    import("core.base.option")

    -- 1. 获取参数 (如果未提供，默认值为菜单配置中的 "23")
    local std_ver = option.get("std")
    print("target c++ standard: C++" .. std_ver)

    -- 2. 调用 xmake 生成 CMakeLists.txt
    print("Executing: xmake project -k cmakelists ...")
    os.exec("xmake project -k cmakelists")

    local cmake_file = "CMakeLists.txt"
    if not os.isfile(cmake_file) then
        raise("Error: CMakeLists.txt failed to generate.")
    end

    -- 3. 读取文件内容
    local content = io.readfile(cmake_file)

    -- 4. 检查是否已经设置了 C++ 标准
    if content:find("CMAKE_CXX_STANDARD") then
        print("check: CMAKE_CXX_STANDARD is already set in generated file. Skipping injection.")
    else
        print(string.format("check: CMAKE_CXX_STANDARD not found. Injecting C++%s...", std_ver))

        -- 使用 string.format 动态生成配置块
        local injection = string.format([[

# [Injection by xmake script] Enforce C++%s Standard
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD %s)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()
]], std_ver, std_ver)

        -- 5. 寻找插入位置 (project(...) 之后)
        local pattern = "(project%s*%b%(%))"
        local new_content, count = content:gsub(pattern, "%1" .. injection, 1)

        if count > 0 then
            content = new_content
        else
            print("warning: 'project(...)' not found. Appending to header.")
            content = injection .. content
        end

        io.writefile(cmake_file, content)
        print("success: CMakeLists.txt updated.")
    end
end)
task_end()