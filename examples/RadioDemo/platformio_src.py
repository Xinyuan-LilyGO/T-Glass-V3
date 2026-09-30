Import("env")

import os


project_dir = env.subst("$PROJECT_DIR")
radio_demo_src = os.path.join(project_dir, "examples", "RadioDemo")
shared_hardware_src = os.path.join(project_dir, "src")
env.Replace(
    PROJECT_SRC_DIR=radio_demo_src,
    PROJECTSRC_DIR="$PROJECT_SRC_DIR",
)
env.Append(CPPPATH=[shared_hardware_src])
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
