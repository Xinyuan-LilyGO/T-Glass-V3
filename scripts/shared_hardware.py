Import("env")

import os


project_dir = env.subst("$PROJECT_DIR")
shared_hardware_src = os.path.join(project_dir, "src")
framework_dir = env.PioPlatform().get_package_dir(
    "framework-arduinoespressif32"
)
framework_library_src = os.path.join(framework_dir, "libraries")
framework_include_dirs = [
    os.path.join(framework_library_src, library_name, "src")
    for library_name in (
        "FS",
        "HTTPClient",
        "I2S",
        "Network",
        "NetworkClientSecure",
        "Preferences",
        "SD",
        "SPI",
        "SPIFFS",
        "WiFi",
        "WiFiClientSecure",
        "Wire",
    )
]
project_libdeps_src = env.subst(
    os.path.join("$PROJECT_LIBDEPS_DIR", "$PIOENV")
)
project_dependency_include_dirs = []
if os.path.isdir(project_libdeps_src):
    project_dependency_include_dirs = [
        os.path.join(project_libdeps_src, library_name, "src")
        for library_name in sorted(os.listdir(project_libdeps_src))
        if os.path.isdir(os.path.join(project_libdeps_src, library_name, "src"))
    ]

env.Append(
    CPPPATH=[shared_hardware_src]
        + framework_include_dirs
        + project_dependency_include_dirs
)
env.BuildSources(
    os.path.join(env.subst("$BUILD_DIR"), "shared_hardware"),
    shared_hardware_src,
    [
        "+<LilyGo_GlassV3.cpp>",
        "+<LilyGo_Button.cpp>",
        "+<PCA9570.cpp>",
        "+<initSequence.cpp>",
        "+<LV_Helper.cpp>",
    ],
)
