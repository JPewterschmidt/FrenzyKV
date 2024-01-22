add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release"
)

add_requires(
    "fmt", 
    "gflags", 
    "gtest", 
    "concurrentqueue master",
    "benchmark", 
    "botan", 
    "nlohmann_json"
)

includes(
    "koios"
)

add_includedirs(
    "include", 
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
        "fmt", 
        "gflags", 
        "concurrentqueue", 
        "botan", 
        "nlohmann_json"
    )
    set_warnings("all", "error")
    add_cxflags("-Wconversion", { force = true })
    add_syslinks(
        "spdlog", 
        "uring"
    )
    add_files(
        "util/*.cc", 
        "io/*.cc"
    )

--target("FrenzyKV-test")
--    set_kind("binary")
--    add_packages("concurrentqueue")
--    add_cxflags("-Wconversion", { force = true })
--    add_deps("FrenzyKV", "koios")
--    set_warnings("all", "error")
--    add_files( "test/*.cc")
--    add_packages(
--        "gtest", "fmt", "spdlog",
--        "botan",
--        "nlohmann_json"
--    after_build(function (target)
--    )
--    if not is_mode("release") then
--        add_cxxflags("-DFRENZYKV_DEBUG", {force = true})
--    end
--    on_run(function (target)
--        --nothing
--    end)
--    
--target("FrenzyKV-example")
--    set_kind("binary")
--    add_packages("")
--    add_cxflags("-Wconversion", { force = true })
--    add_deps("FrenzyKV", "koios")
--    add_files( "example/*.cc")
--    add_syslinks("spdlog")
--    set_policy("build.warning", true)
--    add_packages(
--    after_build(function (target)
--        os.exec(target:targetfile())
--        print("xmake: unittest complete.")
--    end)
--    on_run(function (target)
--        --nothing
--    end)
    
--target("FrenzyKV-example")
--    set_kind("binary")
--    add_packages("")
--    add_cxflags("-Wconversion", { force = true })
--    add_deps("FrenzyKV", "koios")
--    )
--    add_files( "example/*.cc")
--    add_syslinks("spdlog")
--    set_policy("build.warning", true)
--    add_packages(
--        "fmt", "gflags", 
--        "concurrentqueue", 
--        "botan",
--        "nlohmann_json"
--    )
    

