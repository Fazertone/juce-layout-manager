# SpriteSheet — XML-driven sprite controls

`internal/SpriteSheet.h` / `.cpp` add a small, reusable system for building
image-strip UI controls (rotary knobs, multi-state switches) from an XML
metadata file plus JPG/PNG frame strips. It is part of the `layout_manager`
JUCE module and is included automatically via `layout_manager.h`.

The design goal is that a plugin never hardcodes frame counts, image names, or
pixel coordinates in C++: everything comes from an XML file that ships next to
the artwork.

## Classes

All classes live in the global namespace (matching the rest of the module).

### `Sprite`
One named control's data:

| member | meaning |
|---|---|
| `label` | unique key, e.g. `"knob_input"` |
| `image` | the frame strip/grid (loaded via `ImageCache`) |
| `columns`, `rows` | grid layout of frames within `image` |
| `placement` | `Rectangle<int>` x/y/w/h in *background-image* space |
| `stateFrames` | optional `"off"/"pressed"/"on"` → frame index map (switches) |

Key methods:
- `getNumFrames()` → `columns * rows`.
- `getStateFrame(state, fallback)` → frame index for a named state.
- `getFrameBounds(frameIndex)` → source rectangle for a frame. Cell edges are
  computed independently as `round(k * dim / count)` for `k` and `k+1`, so
  rounding error never accumulates across cells. This fixes the whole-pixel-only
  limitation of the old `SpriteKnobLookAndFeel` path in `LMLookAndFeel.cpp`
  (which used `frameIdx * (imgWidth/count)` and drifted for non-integer frame
  sizes).
- `drawFrame(g, frameIndex, destRect)` → blits the frame into `destRect`.

### `SpriteSheetCollection`
Loads and owns a set of sprites keyed by label.

```cpp
SpriteSheetCollection knobs;
knobs.loadFromXml (assetsDir.getChildFile ("spritesheet_knobs/sprites.xml"));

const Sprite* s  = knobs.get ("knob_input");   // nullptr if missing
const Sprite& s2 = knobs["knob_input"];        // jassert + empty Sprite if missing
```

- `loadFromXml(file)` parses the XML, resolving each `filename` relative to the
  XML file's own directory, and loads images with `ImageCache::getFromFile`.
  Returns `false` if the document can't be parsed or no sprites were read.
- `get(label)` returns `nullptr` when a label is absent (safe, silent).
- `operator[](label)` `jassert`s in Debug and returns a static empty `Sprite`
  in Release — never crashes, draws nothing.
- `type` / `numFramesDefault` expose the sheet-wide `type` and `num_frames`
  attributes.

Validation on load (Debug only, via `jassert` + `DBG`):
- missing / unreadable image file;
- image dimensions not matching `w*columns × h*rows` (tolerance ±1px).

### `SpriteKnobComponent : juce::Slider`
A rotary slider that paints itself from a `Sprite` strip.
- Constructed as `RotaryHorizontalVerticalDrag`, `NoTextBox`.
- `setSprite(const Sprite*)` (non-owning) / `getSprite()`.
- Frame chosen as `round(valueToProportionOfLength(getValue()) * (numFrames-1))`,
  so it honours the slider's range/skew.
- Works with a plain `AudioProcessorValueTreeState::SliderAttachment` because it
  *is* a `juce::Slider`.

### `SpriteSwitchComponent : juce::Button`
A button that paints itself from a `Sprite` strip using named states.
- `setSprite(const Sprite*)` / `getSprite()`.
- While held → `"pressed"` frame; otherwise `getToggleState() ? "on" : "off"`.
- Works with `ButtonAttachment`, radio groups, and `ParameterAttachment`
  because it *is* a `juce::Button`.

Why `Component` subclasses instead of `LookAndFeel`s? A LAF needs either one
instance per control or ComponentID-string dispatch (the pattern that turned the
old Pressure `SwitchLookAndFeel` into a ~19-image monster). Subclasses keep the
JUCE attachment pattern identical while owning their own sprite pointer.

## XML schema

Root element `<SpriteSheets>` with per-control `<Sprite>` children:

```xml
<SpriteSheets type="knob" num_frames="128">
  <Sprite id="0" label="knob_input" x="148" y="159" w="150" h="137"
          columns="8" rows="16" filename="knob_0.jpg" />
</SpriteSheets>
```

```xml
<SpriteSheets type="switch" num_frames="3">
  <Sprite id="0" label="switch_2to1" x="98" y="342" w="121" h="121"
          columns="1" rows="3" filename="switch_0.jpg"
          off="0" pressed="1" on="2" />
</SpriteSheets>
```

Attributes:
- `label` — unique lookup key (required).
- `x`, `y`, `w`, `h` — placement rectangle in background-image space. Because
  sprites are authored at 1× background resolution, `w`/`h` also equal one
  frame's pixel size, so `image` must be `w*columns × h*rows`.
- `columns`, `rows` — frame grid (default 1×1).
- `filename` — image file, resolved relative to the XML's directory.
- `edgeblur` — optional (default 0). Linear alpha feather width at the frame
  border, in background-image (source) pixels. The frames are opaque JPGs, so
  without this their rectangular edge lands as a hard seam over the panel;
  `edgeblur` ramps each border pixel's alpha 1→0 across the given width so the
  frame dissolves into whatever is behind it (hides lighting/shadow seams at the
  sprite rectangle). Applied per-frame in `drawFrame`; 0 = plain blit.
- `off` / `pressed` / `on` — optional state→frame indices for switches.
- `*_src` and other attributes are ignored (tooling metadata).

## Consuming pattern (host component)

```cpp
background = juce::ImageCache::getFromFile (dir.getChildFile ("bg.jpg"));
knobs.loadFromXml (dir.getChildFile ("spritesheet_knobs/sprites.xml"));

auto* k = new SpriteKnobComponent();
k->setSprite (knobs.get ("knob_input"));
addAndMakeVisible (k);
attachment = new SliderAttachment (vts, "in_volume", *k);

// resized(): one uniform scale from background space to component space
const float scale = getWidth() / (float) background.getWidth();
k->setBounds ((k->getSprite()->placement.toFloat() * scale).getSmallestIntegerContainer());
```

## Notes
- `ImageCache` caches files for ~5s; editing a sprite JPG while the plugin is
  open won't show until it reopens.
- Guard `resized()` against an invalid/zero-width background (divide-by-zero).
