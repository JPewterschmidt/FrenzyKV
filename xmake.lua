add_rules(
    "mode.debug", "mode.release", "mode.asan"
)

add_requires(
    "gflags", 
    "gtest", 
    "concurrentqueue master",
    "nlohmann_json", 
    "spdlog", 
    "jemalloc", 
    "zstd", 
    "magic_enum", 
    "libuuid"
)
add_requires("crc32c", { configs = { pic = true } })
add_requireconfs("crc32c", { override = true }, { configs = { cxflags = "-fPIC" } })

includes(
    "koios", 
    "fifo-lru-cache"
)

set_arch("x64")

set_languages("c++23", "c17")
set_policy("build.warning", true)
set_policy("build.optimization.lto", false)
set_toolset("cc", "mold", {force = true}) 
add_packages("jeamalloc")

if not is_mode("release") then
    add_cxxflags(
        "-DFRENZYKV_DEBUG", 
        "-DKOIOS_DEBUG", 
        {force = true}
    )
end

add_cxxflags("-march=native", {force = true})

target("FrenzyKV")
    set_kind("shared")
    add_deps("koios", "toolpex", "fifo-lru-cache")
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
    add_cxflags("-march=native -mtune=native", { force = true })
    add_files(
        "util/*.cc", 
        "table/*.cc", 
        "io/*.cc", 
        "db/*.cc", 
        "log/*.cc",
        "persistent/*.cc"
    )
