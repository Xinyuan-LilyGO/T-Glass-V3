Import("env")

import os


project_dir = env.subst("$PROJECT_DIR")
shared_hardware_src = os.path.join(project_dir, "src")
env.Replace(
    PROJECT_SRC_DIR=os.path.join(project_dir, "examples", "DinoJump"),
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
