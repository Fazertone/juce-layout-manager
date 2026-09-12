# juce-layout-manager

## Clone and dependencies

Clone with submodules to include the required `melatonin_blur` JUCE module:

```bash
git clone --recurse-submodules https://github.com/Fazertone/juce-layout-manager.git
cd juce-layout-manager
```

For an existing checkout, or after pulling changes:

```bash
git submodule update --init --recursive
```

The parent repository pins the dependency to a specific commit. Use the command
above to fetch and check out that recorded version; `git submodule update --remote`
is only for intentionally upgrading the dependency. When upgrading, commit the
updated `modules/melatonin_blur` entry in this repository too. The `.gitmodules`
file and that entry must both be committed for fresh clones to work.

## Figma pipeline

fig2sketch "XXX.fig" /dev/null --dump-fig-json files/XXX.json && \
python scripts/figma_json_grabber.py files/XXX.json --output XXX.xml --frame_name XXX

## Module docs

- `layout_manager/README_SpriteSheet.md` — the XML-driven `SpriteSheet` system
  (`Sprite`, `SpriteSheetCollection`, `SpriteKnobComponent`,
  `SpriteSwitchComponent`) for building image-strip knobs/switches from an XML
  metadata file.

- `layout_manager/README_Effects.md` — XML drop shadows and glows for shapes, text,
  slider parts, and custom components; caching, scaling, clipping, and render tests.

- `layout_manager/README_Controls.md` — XML rotary symbols, vertical bar/dot
  sliders, numeric entry, and a reusable interactive AHDSR graph.
