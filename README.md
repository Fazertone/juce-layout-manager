Pipeline:

fig2sketch "XXX.fig" /dev/null --dump-fig-json files/XXX.json && \
python scripts/figma_json_grabber.py files/XXX.json --output XXX.xml --frame_name XXX

## Module docs

- `layout_manager/README_SpriteSheet.md` — the XML-driven `SpriteSheet` system
  (`Sprite`, `SpriteSheetCollection`, `SpriteKnobComponent`,
  `SpriteSwitchComponent`) for building image-strip knobs/switches from an XML
  metadata file.
