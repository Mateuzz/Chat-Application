workspace "project"

    configurations  {"Debug", "Release"}
    platforms {"Linux64", "Linux32"}

    language "C"
    buildoptions {"-Wall", "-W", "-Wextra", "-Wpedantic"}
    targetdir "bin"

    filter "configurations:Release"
        kind "WindowedApp"
        defines {"NDEBUG"}
        optimize "On"

    filter "configurations:Debug"
        kind "ConsoleApp"
        defines {"DEBUG"}
        symbols "On"

    filter {"platforms:Linux64"}
        system "linux"
        architecture "x86_64"

    filter {"platforms:Linux32"}
        system "linux"
        architecture "x86"

    filter {}
        links {"SDL2", "OpenGL", "GL", "GLEW", "m", "GLU"}

    project "main"
        files {"src/**.c", "src/**.h"}
        includedirs {"src/", "vendor/"}
        targetname "main"
