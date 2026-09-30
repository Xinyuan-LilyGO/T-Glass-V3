# LilyGo3D

This is a standalone T-Glass V3 demo that renders the word `LILYGO` as a
rotating low-poly 3D object.

The mesh is generated at runtime from a small 5x7 bitmap. Every contiguous
horizontal run becomes an extruded cuboid, so the front, top, side and back
faces can use different RGB565 colors. This is a block-style wordmark, not an
official LilyGo font asset.

## Bundled Jet dependency

The Jet rasterizer is bundled in this project at `third_party/Jet/src`.
`JetSources.cpp` includes its implementation files once, while
`JetConfig.hpp` is kept beside the Jet headers so `FastMath.hpp` can resolve it
without an external include path. The original Jet license is retained at
`third_party/Jet/LICENSE`.

## Selecting the demo

The repository's root `platformio.ini` is currently configured to select
`examples/LilyGo3D` as `src_dir`. Its `T-Glass` environment adds both the
example directory and Jet's `src` directory to the include path, removes the
framework's conflicting `-std=gnu++11` default, and enables `-std=gnu++17`,
which Jet requires.

The bundled Jet configuration uses a full 126x126 RGB565 framebuffer and a
matching Z-buffer, with half-width buffers, field buffers, textures, lighting
and post-processing disabled for this small display.

## Display path

The visible window is flushed through the same raw path as `DinoJump`:

- render into a 126x126 RGB565 buffer;
- set the JD9613 window at display Y offset 168;
- push the contiguous buffer with `glass.pushColors()`.

No compile, upload or flash operation is performed by this change.
