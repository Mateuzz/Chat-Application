workspace "chat"

    configurations  {"Debug", "Release"}
    platforms {"Linux64", "Linux32", "Win64", "Win32"}

    language "C"
    buildoptions {"-Wall", "-W", "-Wextra"}
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
        architecture "x86"

    filter {"platforms:Win64"}
        system "Windows"
        architecture "x86_64"

    filter {"platforms:Win32"}
        system "Windows"
        architecture "x86"

    filter {}

    project "main"
        kind "WindowedApp"
        -- kind "ConsoleApp"
        pchheader "vendor/nuklear/nuklear.h"
        files {"src/**.c", "src/**.h"}
        includedirs {"src/", "vendor/"}
        links {"SDL2", "SDL2main"}
        libdirs {"lib"}
        targetname "main"
