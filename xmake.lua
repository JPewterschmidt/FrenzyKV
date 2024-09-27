add_rules(
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
    "koios", 
    "fifo-lru-cache"
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
    add_cxflags("-Wconversion", { force = true })
    add_cxflags("-march=native -mtune=native", { force = true })
    add_files(
        "util/*.cc", 
        "table/*.cc", 
        "io/*.cc", 
        "db/*.cc", 
        "log/*.cc",
        "persistent/*.cc"
    )

target("FrenzyKV-test")
    set_kind("binary")
    add_deps("FrenzyKV", "koios", "toolpex")
    add_packages("concurrentqueue")
    add_cxflags("-Wconversion", { force = true })
    set_warnings("all", "error")
    add_files( "test/*.cc")
    add_packages(
        "gtest", "spdlog",
        "nlohmann_json"
    )
    after_build(function (target)
        print("xmake.lua: unittest start.")
        os.execv(target:targetfile(), {"--gtest_color=yes"})
        print("xmake.lua: unittest complete.")
    end)
    on_run(function (target)
        --nothing
    end)
    
target("FrenzyKV-example")
    set_kind("binary")
    add_deps("FrenzyKV", "koios", "toolpex")
    add_cxflags("-Wconversion", { force = true })
    add_files( "example/*.cc")
    set_policy("build.warning", true)
    add_packages(
        "gflags", 
        "concurrentqueue", 
        "nlohmann_json", 
        "spdlog"
    )
