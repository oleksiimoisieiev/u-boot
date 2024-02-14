# Call this before define_kleaf_workspace so we
# always prefer the u-boot py_toolchain.
# As a side effect, all u-boot targets, including those that do not need
# this special python, will use this special python.
register_toolchains("//u-boot:py_toolchain")

local_repository(
    name = "swig",
    path = "external/swig",
)

load("//build/kernel/kleaf:workspace.bzl", "define_kleaf_workspace")
define_kleaf_workspace(common_kernel_package = "u-boot")
