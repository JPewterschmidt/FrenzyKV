add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release"
)

add_requires(
    "gflags", 
    "gtest", 
    "concurrentqueue master",
    "benchmark", 
    "botan", 
    "nlohmann_json", 
    "protobuf-cpp", 
    "spdlog", 
    "jemalloc"
)

includes(
    "koios"
)

add_includedirs(
    "koios/toolpex/include", 
    "koios/include",
    { public = true }
)

add_includedirs(
    ".",
    { public = false }
)

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
    add_packages(
        "gflags", 
        "concurrentqueue", 
        "botan", 
        "nlohmann_json", 
        "spdlog"
    )
    add_packages("protobuf-cpp")
    add_rules("protobuf.cpp")
    set_warnings("all", "error")
    add_cxflags("-Wconversion", { force = true })
    add_syslinks(
        "uring"
    )
    add_files(
        "util/*.cc", 
        "io/*.cc", 
        "db/*.cc", 
        "log/*.cc"
    )
    add_files("proto/*.proto", { proto_public = true })

--target("FrenzyKV-test")
--    set_kind("binary")
--    add_packages("protobuf-cpp")
--    add_rules("protobuf.cpp")
--    add_packages("concurrentqueue")
--    add_cxflags("-Wconversion", { force = true })
--    add_deps("FrenzyKV", "koios")
--    set_warnings("all", "error")
--    add_files( "test/*.cc")
--    add_packages(
--        "gtest", "spdlog",
--        "botan",
--        "nlohmann_json"
--    )
--    after_build(function (target)
--        os.exec(target:targetfile())
--        print("xmake: unittest complete.")
--    end)
--    on_run(function (target)
--        --nothing
--    end)
--    
--target("FrenzyKV-example")
--    set_kind("binary")
--    add_packages("protobuf-cpp", { public = false })
--    add_rules("protobuf.cpp")
--    add_files("proto/*.proto", { proto_public = false })
--    add_cxflags("-Wconversion", { force = true })
--    add_deps("FrenzyKV", "koios")
--    add_files( "example/*.cc")
--    set_policy("build.warning", true)
--    add_packages(
--        "gflags", 
--        "concurrentqueue", 
--        "botan",
--        "nlohmann_json", 
--        "spdlog"
--    )
    

