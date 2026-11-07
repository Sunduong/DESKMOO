
from PIL import Image
from pathlib import Path

INPUT = Path("Side_strech_final7.png")
OUTPUT = Path("background_rgb666.h")

WIDTH = 480
HEIGHT = 320

img = Image.open(INPUT).convert("RGB")

if img.size != (WIDTH, HEIGHT):
    raise ValueError(
        f"Expected {WIDTH}x{HEIGHT}, got {img.size}"
    )

data = bytearray()

for r, g, b in img.getdata():
    data.extend((
        r & 0xFC,
        g & 0xFC,
        b & 0xFC
    ))

with OUTPUT.open("w", encoding="utf-8") as f:
    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")
    f.write(
        "static const uint8_t background_rgb666[] "
        "__attribute__((aligned(4))) = {\n"
    )

    for i in range(0, len(data), 12):
        chunk = data[i:i + 12]
        f.write(
            "    " +
            ", ".join(f"0x{b:02X}" for b in chunk) +
            ",\n"
        )

    f.write("};\n")

print(f"Generated {OUTPUT}: {len(data)} bytes")
