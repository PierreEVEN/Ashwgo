add_rules("mode.debug", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

set_project("Ashwga")
set_languages("clatest", "cxx20")
set_allowedarchs("windows|x64")
set_warnings("allextra")
set_allowedmodes("debug", "release")
set_defaultmode("release")
set_rundir(".")

DEBUG = false;
BUILD_MONOLITHIC = true;

if is_mode("debug") then
    set_optimize("fastest")
    set_strip("all")
end

if is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
end

set_runtimes(is_mode("debug") and "MTd" or "MT")
		
add_requires("vulkan-loader", "glfw", "glm", "imgui docking", "vulkan-memory-allocator", "directxshadercompiler", "spirv-reflect", "assimp", "concurrentqueue")

add_requires("freeimage", {configs = {rgb = true}})

function declare_module(module_name, deps, packages, is_executable, enable_reflection)
    if DEBUG then
        print("### "..module_name.." ###")
    end
    target(module_name, function (target)

        if is_executable then
            set_kind("binary")
        elseif BUILD_MONOLITHIC then
            set_kind("static")
        else
            set_kind("shared")
        end


        add_defines("GLM_FORCE_LEFT_HANDED", "GLM_FORCE_DEPTH_ZERO_TO_ONE")
        
        if enable_reflection then
            add_deps('header_tool')
            add_deps('reflection') 

            before_build(function (target)
                os.mkdir("$(buildir)/reflection/"..module_name.."/private/")
                os.mkdir("$(buildir)/reflection/"..module_name.."/public/")
                if (DEBUG) then
                    print("xmake run header_tool "..target:scriptdir().." $(buildir)/reflection/"..module_name)
                end
                os.exec("xmake run header_tool "..target:scriptdir().." $(buildir)/reflection/"..module_name)
            end)
            on_config(function (target) 
                for _, file in pairs(os.files(os.projectdir().."/$(buildir)/reflection/"..module_name.."/private/**.cpp")) do
                    target:add("files", file)
                end
            end)

            add_includedirs("$(buildir)/reflection/"..module_name.."/public/", { public = true })
        end

        add_includedirs("private", { public = false })
        if not is_executable then
            add_includedirs("public", { public = true })
        end

        for _, file in pairs(os.files("private/**.cpp")) do
            add_files(file)
        end

        for _, file in pairs(os.files("private/**.hpp")) do
            add_headerfiles(file)
        end
        
        for _, file in pairs(os.files("public/**.hpp")) do
            add_headerfiles(file)
        end

        if deps then
            if DEBUG then
                print(table.unpack({ "\t-- dependencies :", table.unpack(deps) }))
            end
            add_deps(table.unpack(deps))
        end
        if packages then
            packages_name = "";
            for _, package in ipairs(packages) do
                if type(package) == "table" then
                    if not BUILD_MONOLITHIC then
                        add_packages(package.name, {public = true})
                    else
                        add_packages(package.name, {public = package.public})
                    end
                    packages_name = packages_name..", "..package.name
                else
                    if not BUILD_MONOLITHIC then
                        add_packages(package, {public = true})
                    else
                        add_packages(package)
                    end
                    packages_name = packages_name..", "..package
                end
            end
            if DEBUG then
                print(table.unpack({ "\t-- packages :", packages_name}))
            end
        end
    end)
end


target("data", function(target)
    set_kind("object")
    for _, file in pairs(os.files("resources/**")) do
        add_headerfiles(file)
    end
end)

add_defines("ENABLE_VALIDATION_LAYER")
if DEBUG then
    print("################ building modules ################")
end

includes("src/**.lua");