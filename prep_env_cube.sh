#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
  echo "Usage: $0 <input_pano_2to1_image> <output_ktx2> [rotate_degrees_right: 0|90|180|270]"
  exit 1
fi

INPUT_PANO="$1"
OUTPUT_KTX2="$2"
ROT_DEG="${3:-180}"

if ! [[ "$ROT_DEG" =~ ^-?[0-9]+$ ]]; then
  echo "Error: rotation must be an integer"
  exit 1
fi
if (( ROT_DEG % 90 != 0 )); then
  echo "Error: rotation must be a multiple of 90"
  exit 1
fi
TURNS=$(( ((ROT_DEG / 90) % 4 + 4) % 4 )) # 0..3, right turns around Y

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
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

ATLAS="$TMPDIR/cubemap_3x2.png"
FACE_DIR="$TMPDIR/faces"
ROT_DIR="$TMPDIR/rot"
mkdir -p "$FACE_DIR" "$ROT_DIR"

# 1) Equirectangular panorama -> 3x2 cubemap atlas
"$FFMPEG_BIN" -y -i "$INPUT_PANO" \
  -vf "v360=input=equirect:output=c3x2" \
  "$ATLAS"

# 2) Split atlas
"$MAGICK_BIN" "$ATLAS" -crop 3x2@ +repage +adjoin "$FACE_DIR/face_%d.png"

# 3) Base mapping to px nx py ny pz nz (with horizontal flip convention)
"$MAGICK_BIN" "$FACE_DIR/face_0.png" -flop "$FACE_DIR/nx.png"
"$MAGICK_BIN" "$FACE_DIR/face_1.png" -flop "$FACE_DIR/px.png"
"$MAGICK_BIN" "$FACE_DIR/face_2.png" -flop "$FACE_DIR/py.png"
"$MAGICK_BIN" "$FACE_DIR/face_3.png" -flop "$FACE_DIR/ny.png"
"$MAGICK_BIN" "$FACE_DIR/face_4.png" -flop "$FACE_DIR/pz.png"
"$MAGICK_BIN" "$FACE_DIR/face_5.png" -flop "$FACE_DIR/nz.png"

# helper: rotate a single face
rot_face() {
  local src="$1" dst="$2" rot="$3"
  case "$rot" in
    0)   cp "$src" "$dst" ;;
    90)  "$MAGICK_BIN" "$src" -rotate 90  "$dst" ;;   # adjust sign if your IM build differs
    180) "$MAGICK_BIN" "$src" -rotate 180 "$dst" ;;
    270) "$MAGICK_BIN" "$src" -rotate 270 "$dst" ;;
  esac
}

# 4) Optional cubemap yaw rotation right by 90° steps:
# side remap per step: +X<- -Z, -X<- +Z, +Z<- +X, -Z<- -X
# top/bottom in-face rotation per step: +Y CCW, -Y CW
if (( TURNS == 0 )); then
  cp "$FACE_DIR/px.png" "$ROT_DIR/px.png"
  cp "$FACE_DIR/nx.png" "$ROT_DIR/nx.png"
  cp "$FACE_DIR/py.png" "$ROT_DIR/py.png"
  cp "$FACE_DIR/ny.png" "$ROT_DIR/ny.png"
  cp "$FACE_DIR/pz.png" "$ROT_DIR/pz.png"
  cp "$FACE_DIR/nz.png" "$ROT_DIR/nz.png"
elif (( TURNS == 1 )); then
  cp "$FACE_DIR/nz.png" "$ROT_DIR/px.png"
  cp "$FACE_DIR/pz.png" "$ROT_DIR/nx.png"
  rot_face "$FACE_DIR/py.png" "$ROT_DIR/py.png" 270
  rot_face "$FACE_DIR/ny.png" "$ROT_DIR/ny.png" 90
  cp "$FACE_DIR/px.png" "$ROT_DIR/pz.png"
  cp "$FACE_DIR/nx.png" "$ROT_DIR/nz.png"
elif (( TURNS == 2 )); then
  cp "$FACE_DIR/nx.png" "$ROT_DIR/px.png"
  cp "$FACE_DIR/px.png" "$ROT_DIR/nx.png"
  rot_face "$FACE_DIR/py.png" "$ROT_DIR/py.png" 180
  rot_face "$FACE_DIR/ny.png" "$ROT_DIR/ny.png" 180
  cp "$FACE_DIR/nz.png" "$ROT_DIR/pz.png"
  cp "$FACE_DIR/pz.png" "$ROT_DIR/nz.png"
else # TURNS == 3
  cp "$FACE_DIR/pz.png" "$ROT_DIR/px.png"
  cp "$FACE_DIR/nz.png" "$ROT_DIR/nx.png"
  rot_face "$FACE_DIR/py.png" "$ROT_DIR/py.png" 90
  rot_face "$FACE_DIR/ny.png" "$ROT_DIR/ny.png" 270
  cp "$FACE_DIR/nx.png" "$ROT_DIR/pz.png"
  cp "$FACE_DIR/px.png" "$ROT_DIR/nz.png"
fi

# 5) Pack into KTX2 cubemap
"$TOKTX_BIN" --genmipmap --uastc 3 --zcmp 18 --verbose --t2 --cubemap \
  "$OUTPUT_KTX2" \
  "$ROT_DIR/px.png" "$ROT_DIR/nx.png" "$ROT_DIR/py.png" \
  "$ROT_DIR/ny.png" "$ROT_DIR/pz.png" "$ROT_DIR/nz.png"

echo "Wrote: $OUTPUT_KTX2 (rotated right by $ROT_DEG deg)"
