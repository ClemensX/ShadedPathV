#!/usr/bin/env bash

# run from /c/dev/vulkan/ShadedPathV
# create debug ktx texture with single color mipmaps: 
#../libraries/ktx/bin/toktx --mipmap ../data/texture/debug.ktx ../datainput/texture/debug0.png ../datainput/texture/debug1.png ../datainput/texture/debug2.png ../datainput/texture/debug3.png ../datainput/texture/debug4.png ../datainput/texture/debug5.png ../datainput/texture/debug6.png ../datainput/texture/debug7.png ../datainput/texture/debug8.png ../datainput/texture/debug9.png
#../libraries/ktx/bin/toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 ../data/texture/eucalyptus.ktx2 ../datainput/texture/eucalyptus.png
#gltf-transform uastc simple_grass_chunks.glb grass.glb --level 2 --zstd 1 --verbose
#gltf-transform metalrough .\simple_grass_chunks.glb grass_pbr.glb
#gltf-transform uastc .\grass_pbr.glb grass.glb --level 2 --zstd 1 --verbose
toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 ../data/texture/shadedpath_logo.ktx2 ../datainput/texture/shadedpath_logo2.png