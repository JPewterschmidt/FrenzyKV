add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release"
)

add_requires(
    "gflags", 
    "gtest", 
    "concurrentqueue master",
    "benchmark", 
    "nlohmann_json", 
    "spdlog", 
    "jemalloc", 
    "zstd", 
    "crc32c", 
    "magic_enum", 
    "libuuid"
)

includes(
    "koios"
)

set_arch("x64")

set_languages("c++23", "c17")
set_policy("build.warning", true)
set_policy("build.optimization.lto", false)
add_packages("jeamalloc")

if not is_mode("release") then
    add_cxxflags(
        "-DFRENZYKV_DEBUG", 
        "-DKOIOS_DEBUG", 
        {force = true}
    )
end

target("FrenzyKV")
    set_kind("shared")
    add_deps("koios", "toolpex")
    add_includedirs(
        "./include",
        { public = true }
    )
    add_includedirs( "smhasher/src", { public = true })
    add_files( "smhasher/src/MurmurHash*.cpp")
    add_packages(
        "gflags", 
        "concurrentqueue", 
        "nlohmann_json", 
        "spdlog",
        "zstd", 
        "crc32c", 
        "magic_enum", 
        "libuuid"
    )
    set_warnings("all", "error")
    add_cxflags("-Wconversion", { force = true })
    add_files(
        "util/*.cc", 
        "io/*.cc", 
        "db/*.cc", 
        "log/*.cc",
        "persistent/*.cc"
    )
