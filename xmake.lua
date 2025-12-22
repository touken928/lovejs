add_rules("mode.debug", "mode.release")
set_languages("c++17")

add_requires("libsdl2")
add_requires("libsdl2_image")
add_requires("quickjs")

target("lovejs")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("libsdl2")
    add_packages("libsdl2_image")
    add_packages("quickjs")
    add_includedirs(".", "src")
