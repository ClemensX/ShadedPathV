#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
  echo "Usage: $0 <input_pano_2to1_image> <output_ktx2>"
  exit 1
fi

INPUT_PANO="$1"
OUTPUT_KTX2="$2"

FFMPEG_BIN="${FFMPEG_BIN:-ffmpeg}"
MAGICK_BIN="${MAGICK_BIN:-magick}"
TOKTX_BIN="${TOKTX_BIN:-toktx}"

for cmd in "$FFMPEG_BIN" "$MAGICK_BIN" "$TOKTX_BIN"; do
  command -v "$cmd" >/dev/null 2>&1 || {
    echo "Error: '$cmd' not found in PATH"
    exit 1
  }
done

TMPDIR="$(mktemp -d)"
cleanup() {
  rmr -rf "$TMPDIR"
}
trap cleanup EXIT

echo "$TMPDIR"

ATLAS="$TMPDIR/cubemap_3x2.png"
FACE_DIR="$TMPDIR/faces"
mkdir -p "$FACE_DIR"

# 1) Equirectangular panorama -> 3x2 cubemap atlas
# This is the standard conversion step for a 2:1 panorama.
"$FFMPEG_BIN" -y -i "$INPUT_PANO" \
  -vf "v360=input=equirect:output=c3x2" \
  "$ATLAS"

# 2) Split atlas into 6 face images
# NOTE: Face order from v360/c3x2 can vary by version/config.
# If the resulting cubemap is rotated or mirrored, adjust this mapping once.
"$MAGICK_BIN" "$ATLAS" -crop 3x2@ +repage +adjoin "$FACE_DIR/face_%d.png"

# 3) Flip each face horizontally to match the engine's cubemap convention
# and remap to the expected packing order:
#   px nx py ny pz nz
"$MAGICK_BIN" "$FACE_DIR/face_0.png" -flop "$FACE_DIR/nx.png"
"$MAGICK_BIN" "$FACE_DIR/face_1.png" -flop "$FACE_DIR/px.png"
"$MAGICK_BIN" "$FACE_DIR/face_2.png" -flop "$FACE_DIR/py.png"
"$MAGICK_BIN" "$FACE_DIR/face_3.png" -flop "$FACE_DIR/ny.png"
"$MAGICK_BIN" "$FACE_DIR/face_4.png" -flop "$FACE_DIR/pz.png"
"$MAGICK_BIN" "$FACE_DIR/face_5.png" -flop "$FACE_DIR/nz.png"

echo "Flipped cubemap faces horizontally and prepared face order for KTX2 packing."

# 4) Pack into KTX2 cubemap
"$TOKTX_BIN" --genmipmap --uastc 3 --zcmp 18 --verbose --t2 --cubemap \
  "$OUTPUT_KTX2" \
  "$FACE_DIR/px.png" "$FACE_DIR/nx.png" "$FACE_DIR/py.png" \
  "$FACE_DIR/ny.png" "$FACE_DIR/pz.png" "$FACE_DIR/nz.png"

echo "Wrote: $OUTPUT_KTX2"
echo "Check your texture in engine. If not ok you might have to change the face order or flipping. Disable TMPDIR cleanup to inspect intermediate files: $TMPDIR"
