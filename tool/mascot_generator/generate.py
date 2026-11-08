from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
INPUT_DIR = BASE / "frames"
OUTPUT = BASE / "mascot_resource.h"

WIDTH = 128
HEIGHT = 240
FRAME_COUNT = 8

# Alpha threshold:
# >= 128 => opaque
# < 128  => transparent
ALPHA_THRESHOLD = 128

paths = [
    INPUT_DIR / f"frame_{i:02d}.png"
    for i in range(FRAME_COUNT)
]

# --------------------------------------------------
# STEP 1: Load all frames
# --------------------------------------------------
images_rgba = []

for path in paths:
    img = Image.open(path).convert("RGBA")

    if img.size != (WIDTH, HEIGHT):
        raise ValueError(
            f"{path.name}: expected {WIDTH}x{HEIGHT}, got {img.size}"
        )

    images_rgba.append(img)

# --------------------------------------------------
# STEP 2: Build a big training image for shared palette
# Quantize all 8 frames together -> one shared palette
# --------------------------------------------------
sheet_rgb = Image.new("RGB", (WIDTH * FRAME_COUNT, HEIGHT), (0, 0, 0))

for i, img in enumerate(images_rgba):
    px = img.load()

    # Create RGB image after binary alpha threshold
    frame_rgb = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
    frame_px = frame_rgb.load()

    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b, a = px[x, y]
            a_bin = 255 if a >= ALPHA_THRESHOLD else 0

            if a_bin == 255:
                # Reduce to RGB666 grid immediately
                frame_px[x, y] = (r & 0xFC, g & 0xFC, b & 0xFC)
            else:
                # Transparent pixels are ignored later anyway
                frame_px[x, y] = (0, 0, 0)

    sheet_rgb.paste(frame_rgb, (i * WIDTH, 0))

# --------------------------------------------------
# STEP 3: Quantize the whole sheet to 255 colors
# Index 0 in our runtime data is reserved for transparency,
# so visible colors must fit in 255 slots.
# --------------------------------------------------
quantized_sheet = sheet_rgb.quantize(
    colors=255,
    method=Image.Quantize.MEDIANCUT,
    dither=Image.Dither.NONE
)

# Extract palette from quantized sheet
raw_palette = quantized_sheet.getpalette()[:255 * 3]

palette_colors = []
for i in range(255):
    r = raw_palette[i * 3 + 0] & 0xFC
    g = raw_palette[i * 3 + 1] & 0xFC
    b = raw_palette[i * 3 + 2] & 0xFC
    palette_colors.append((r, g, b))

# Runtime palette:
# index 0 => transparent
# index 1..255 => visible colors
runtime_palette = [(0, 0, 0)] + palette_colors

# --------------------------------------------------
# STEP 4: Convert each frame to indexed pixels
# using the SAME shared palette
# --------------------------------------------------
def frame_to_indices(img_rgba, shared_palette_image):
    rgb_img = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
    rgb_px = rgb_img.load()
    src_px = img_rgba.load()

    # First apply alpha threshold + RGB666 reduction
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b, a = src_px[x, y]
            a_bin = 255 if a >= ALPHA_THRESHOLD else 0

            if a_bin == 255:
                rgb_px[x, y] = (r & 0xFC, g & 0xFC, b & 0xFC)
            else:
                rgb_px[x, y] = (0, 0, 0)

    # Quantize using the shared palette
    indexed_img = rgb_img.quantize(
        palette=shared_palette_image,
        dither=Image.Dither.NONE
    )

    idx_px = indexed_img.load()
    indices = bytearray()

    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b, a = src_px[x, y]
            a_bin = 255 if a >= ALPHA_THRESHOLD else 0

            if a_bin == 0:
                indices.append(0)  # transparent
            else:
                # +1 because runtime palette index 0 is transparent
                indices.append(idx_px[x, y] + 1)

    return bytes(indices)

# --------------------------------------------------
# STEP 5: RLE encode/decode
# Format: [count, index]
# --------------------------------------------------
def rle_encode(data):
    output = bytearray()
    i = 0

    while i < len(data):
        value = data[i]
        count = 1

        while (
            i + count < len(data)
            and data[i + count] == value
            and count < 255
        ):
            count += 1

        output.extend((count, value))
        i += count

    return bytes(output)

def rle_decode(data):
    if len(data) % 2 != 0:
        raise ValueError("Invalid RLE stream length")

    output = bytearray()

    for i in range(0, len(data), 2):
        count = data[i]
        value = data[i + 1]

        if count == 0:
            raise ValueError("Invalid zero-length run")

        output.extend([value] * count)

    return bytes(output)

# --------------------------------------------------
# STEP 6: Encode all frames
# --------------------------------------------------
frame_indices = []
encoded_frames = []

print(f"Alpha mode: binary threshold at {ALPHA_THRESHOLD}")
print("Palette mode: force quantize to 255 visible colors")
print()

for i, img in enumerate(images_rgba):
    indices = frame_to_indices(img, quantized_sheet)
    encoded = rle_encode(indices)

    # Verify
    decoded = rle_decode(encoded)
    assert decoded == indices, f"RLE verify failed at frame {i}"

    frame_indices.append(indices)
    encoded_frames.append(encoded)

    raw_size = len(indices)
    rle_size = len(encoded)
    ratio = rle_size / raw_size * 100.0

    print(
        f"Frame {i}: "
        f"Raw={raw_size} bytes, "
        f"RLE={rle_size} bytes, "
        f"Size={ratio:.1f}%"
    )

# --------------------------------------------------
# STEP 7: Export C header
# --------------------------------------------------
def write_array(f, name, data):
    f.write(f"static const uint8_t {name}[] = {{\n")
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        f.write("    " + ", ".join(f"0x{x:02X}" for x in chunk) + ",\n")
    f.write("};\n\n")

with OUTPUT.open("w", encoding="utf-8") as f:
    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")

    f.write(f"#define MASCOT_WIDTH {WIDTH}\n")
    f.write(f"#define MASCOT_HEIGHT {HEIGHT}\n")
    f.write(f"#define MASCOT_FRAME_COUNT {FRAME_COUNT}\n")
    f.write(f"#define MASCOT_PALETTE_SIZE {len(runtime_palette)}\n\n")

    # Flat RGB666 palette bytes
    palette_bytes = bytes(
        channel
        for color in runtime_palette
        for channel in color
    )

    write_array(f, "mascot_palette", palette_bytes)

    for i, data in enumerate(encoded_frames):
        write_array(f, f"mascot_frame_{i}", data)

    f.write("static const uint8_t *const mascot_frames[] = {\n")
    for i in range(FRAME_COUNT):
        f.write(f"    mascot_frame_{i},\n")
    f.write("};\n\n")

    f.write("static const uint32_t mascot_frame_sizes[] = {\n")
    for data in encoded_frames:
        f.write(f"    {len(data)},\n")
    f.write("};\n")

total_size = sum(len(data) for data in encoded_frames) + len(runtime_palette) * 3

print()
print(f"Runtime palette entries: {len(runtime_palette)}")
print(f"Total resource size (palette + frames): {total_size} bytes ({total_size / 1024:.2f} KiB)")
print(f"Generated: {OUTPUT}")
