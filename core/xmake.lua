set_project("flame")
set_languages("c17","cxx20")

local vendor = {
    ["gsl"]     = "/data/vendor/gsl",
    ["boost"]   = "/data/vendor/boost",
    ["php"]     = "/data/server/php"
}

---
target("flame-core")
    set_kind("static")
    add_rules("mode.debug", "mode.release", "mode.releasedbg")
    add_cxflags("-fPIC")
    add_includedirs(
        -- 内部头文件
        "include",
        vendor["php"] .. "/include",
        -- PHP 内部 INCLUDE 路径，来源于 php-config --includes 命令
        vendor["php"] .. "/include/php",
        vendor["php"] .. "/include/php/main",
        vendor["php"] .. "/include/php/TSRM",
        vendor["php"] .. "/include/php/Zend",
        vendor["php"] .. "/include/php/ext",
        vendor["php"] .. "/include/php/ext/date/lib",
        -- 其他依赖
        vendor["boost"] .. "/include",
        vendor["gsl"] .. "/include"
    )
    add_syslinks("pthread")
    add_files("src/flame/**.cpp")

---
target("demo")
    set_kind("shared")
    add_deps("flame-core")
    set_prefixname("")
    add_rules("mode.debug", "mode.release", "mode.releasedbg")
    add_includedirs(
        "include",
        vendor["boost"] .. "/include",
        vendor["gsl"] .. "/include"
    )
    add_syslinks("pthread")
    -- add_links(vendor["php-cpp"] .. "/lib/libphpcpp.a")
    add_files("src/*.cpp")
