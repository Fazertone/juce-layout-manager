Pipeline:

fig2sketch "XXX.fig" /dev/null --dump-fig-json files/XXX.json && \
python scripts/figma_json_grabber.py files/XXX.json --output XXX.xml --frame_name XXX