Import("env")

import os
import subprocess
import sys


project_dir = env.subst("$PROJECT_DIR")
scripts_dir = os.path.join(project_dir, "scripts")
if scripts_dir not in sys.path:
    sys.path.insert(0, scripts_dir)
from ensure_espdl import ensure_espdl


astra_port_src = os.path.join(project_dir, "examples", "AstraPort")
radio_demo_src = os.path.join(project_dir, "examples", "RadioDemo")
shared_hardware_src = os.path.join(project_dir, "src")
espdl_root = str(ensure_espdl(project_dir))
gesture_models_src = os.path.join(astra_port_src, "camera", "Gesture3D")
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
# BuildSources creates these objects before the normal dependency scanner adds
# the active environment's library include directories.
project_dependency_include_dirs = []
if os.path.isdir(project_libdeps_src):
    project_dependency_include_dirs = [
        os.path.join(project_libdeps_src, library_name, "src")
        for library_name in sorted(os.listdir(project_libdeps_src))
        if os.path.isdir(os.path.join(project_libdeps_src, library_name, "src"))
    ]
env.Replace(
    PROJECT_SRC_DIR=astra_port_src,
    PROJECTSRC_DIR="$PROJECT_SRC_DIR",
)
env.Append(
    CPPPATH=[shared_hardware_src, radio_demo_src]
        + framework_include_dirs
        + project_dependency_include_dirs
)

espdl_generated = os.path.join(env.subst("$BUILD_DIR"), "espdl_generated")
os.makedirs(espdl_generated, exist_ok=True)
compile_finalize = os.path.join(espdl_root, "esp-dl", "cmake", "compile_finalize.py")
espdl_component = os.path.join(espdl_root, "esp-dl")
subprocess.check_call(
    [
        sys.executable,
        compile_finalize,
        "--ops",
        os.path.join(espdl_component, "spec", "ops.yml"),
        "--kernels",
        os.path.join(espdl_component, "spec", "kernels.yml"),
        "--conv-yml",
        os.path.join(espdl_component, "spec", "select", "Conv.yml"),
        "--target",
        "esp32s3",
        "--component-dir",
        espdl_component,
        "--header",
        os.path.join(espdl_generated, "dl_compile_config.h"),
        "--srcs",
        os.path.join(espdl_generated, "dl_compile_srcs.cmake"),
        "--kernel-inc",
        os.path.join(espdl_generated, "dl_kernel.inc"),
        "--conv-select",
        os.path.join(espdl_generated, "dl_conv_select.inc"),
        "--module-includes-inc",
        os.path.join(espdl_generated, "dl_module_includes.inc"),
        "--module-register-inc",
        os.path.join(espdl_generated, "dl_module_register.inc"),
    ]
)

espdl_include_dirs = [
    espdl_component,
    os.path.join(espdl_component, "dl"),
    os.path.join(espdl_component, "dl", "tool", "include"),
    os.path.join(espdl_component, "dl", "tensor", "include"),
    os.path.join(espdl_component, "dl", "base"),
    os.path.join(espdl_component, "dl", "base", "isa"),
    os.path.join(espdl_component, "dl", "base", "isa", "xtensa"),
    os.path.join(espdl_component, "dl", "base", "isa", "tie728"),
    os.path.join(espdl_component, "dl", "math", "include"),
    os.path.join(espdl_component, "dl", "model", "include"),
    os.path.join(espdl_component, "dl", "module", "include"),
    os.path.join(espdl_component, "fbs_loader", "include"),
    os.path.join(espdl_component, "vision", "detect"),
    os.path.join(espdl_component, "vision", "image"),
    os.path.join(espdl_component, "vision", "image", "isa"),
    os.path.join(espdl_component, "vision", "recognition"),
    os.path.join(espdl_component, "vision", "classification"),
    os.path.join(espdl_root, "models", "hand_detect"),
    os.path.join(espdl_root, "models", "hand_gesture_recognition"),
]
env.Append(CPPPATH=[espdl_generated, gesture_models_src] + espdl_include_dirs)

generated_srcs = []
with open(os.path.join(espdl_generated, "dl_compile_srcs.cmake"), encoding="utf-8") as source_list:
    for line in source_list:
        line = line.strip()
        if line and not line.startswith("#") and not line.startswith("set(") and line != ")":
            generated_srcs.append("+<" + line.replace("\\", "/") + ">")

espdl_sources = generated_srcs + [
    "+<dl/base/dl_kernel.cpp>",
    "+<dl/tool/isa/xtensa/*.S>",
    "+<dl/tool/isa/tie728/*.S>",
    "+<dl/tool/src/*.cpp>",
    "+<dl/tensor/src/*.cpp>",
    "+<dl/math/src/*.cpp>",
    "+<dl/model/src/*.cpp>",
    "+<dl/module/src/*.cpp>",
    "+<fbs_loader/src/*.cpp>",
    "+<vision/detect/dl_detect_base.cpp>",
    "+<vision/detect/dl_detect_postprocessor.cpp>",
    "+<vision/detect/dl_detect_espdet_postprocessor.cpp>",
    "+<vision/image/dl_image_preprocessor.cpp>",
    "+<vision/image/dl_image_process.cpp>",
    "+<vision/image/dl_image_pixel_cvt_dispatch_rgb8882rgb888.cpp>",
    "+<vision/classification/dl_cls_base.cpp>",
    "+<vision/classification/dl_cls_postprocessor.cpp>",
    "+<vision/classification/hand_gesture_cls_postprocessor.cpp>",
]
env.BuildSources(
    os.path.join(env.subst("$BUILD_DIR"), "espdl"),
    espdl_component,
    espdl_sources,
)
fbs_model_lib = os.path.join(espdl_component, "fbs_loader", "lib", "esp32s3", "libfbs_model.a")
env.Append(LIBPATH=[os.path.dirname(fbs_model_lib)], LIBS=["fbs_model"])
env.Append(CXXFLAGS=["-O3", "-ffast-math", "-Wno-array-bounds", "-Wno-deprecated-copy"])
env.BuildSources(
    os.path.join(env.subst("$BUILD_DIR"), "shared_hardware"),
    shared_hardware_src,
    [
        "+<LilyGo_GlassV3.cpp>",
        "+<LilyGo_Button.cpp>",
        "+<PCA9570.cpp>",
        "+<initSequence.cpp>",
    ],
)
env.BuildSources(
    os.path.join(env.subst("$BUILD_DIR"), "radio_demo"),
    radio_demo_src,
    ["+<RadioDemoPlayer.cpp>"],
)
