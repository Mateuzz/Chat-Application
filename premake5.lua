workspace "chat"

    configurations  {"Debug", "Release"}
    platforms {"Linux64", "Linux32", "Win64", "Win32"}

    language "C"
    buildoptions {"-Wall", "-W", "-Wextra", "-Wpedantic"}
    targetdir "bin"

    filter "configurations:Release"
        defines {"RELEASE"}
        optimize "On"

    filter "configurations:Debug"
        defines {"DEBUG"}
        symbols "On"

    filter {"platforms:Linux64"}
        system "linux"
        architecture "x86_64"

    filter {"platforms:Linux32"}
        system "linux"
        architecture "x86" filter {"platforms:Win64"} system "Windows"
        architecture "x86_64"

    filter {"platforms:Win32"}
        system "Windows"
        architecture "x86"

    filter {}
        kind "ConsoleApp"
        -- kind "WindowedApp"
        libdirs {"lib"}
        links {"SDL2", "OpenGL", "GL", "GLEW", "m", "GLU"}
        includedirs {"src/", "vendor/"}

    project "main"
        files {"src/**.c", "src/**.h"}
        targetname "main"

    project "server"
        files {"demo/demo_server.c", "src/chat_server.c", "src/network.c"}
        targetname "server"

    project "client"
        files {"demo/demo_client.c", "src/chat_client.c", "src/network.c"}
        targetname "client"
