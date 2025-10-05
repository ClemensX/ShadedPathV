#!/usr/bin/env bash

# cube maps: pz front, nz back, nx left, px right, ny bottom, py top
#        top 
# left   front    right   back
#        bottom

#        py 
# nx     pz       px      nz
#        ny

# Windows: run in git bash from /c/dev/cpp/ShadedPathV
# path to ktx tools: C:\tools\vcpkg\packages\ktx_x64-windows\tools\ktx\ktx
# if newer version is needed you might need to manually install locally, as vcpkg does not seem to get the right version:
# set PATH=c:\tools\ktx\bin;%PATH%
#export PATH="$PATH:/c/tools/vcpkg/packages/ktx_x64-windows/tools/ktx"
export PATH="/c/tools/ktx/bin:$PATH"

# create debug ktx texture with single color mipmaps: 
#../libraries/ktx/bin/toktx --mipmap ../data/texture/debug.ktx ../datainput/texture/debug0.png ../datainput/texture/debug1.png ../datainput/texture/debug2.png ../datainput/texture/debug3.png ../datainput/texture/debug4.png ../datainput/texture/debug5.png ../datainput/texture/debug6.png ../datainput/texture/debug7.png ../datainput/texture/debug8.png ../datainput/texture/debug9.png
#../libraries/ktx/bin/toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 ../data/texture/eucalyptus.ktx2 ../datainput/texture/eucalyptus.png
#gltf-transform uastc simple_grass_chunks.glb grass.glb --level 2 --zstd 1 --verbose
#gltf-transform metalrough .\simple_grass_chunks.glb grass_pbr.glb
#gltf-transform uastc .\grass_pbr.glb grass.glb --level 2 --zstd 1 --verbose
#toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 ../data/texture/shadedpath_logo.ktx2 ../datainput/texture/shadedpath_logo2.png
#toktx --uastc 3 --zcmp 18 --verbose --t2 ../data/texture/shadedpath_logo.ktx2 ../datainput/texture/shadedpath_logo2.png
#ktx create --format R32_SFLOAT --raw --width 3 --height 3 ../data/texture/heightmap_9_points.raw ../data/texture/height.ktx2
#ktx create --format R32_SFLOAT --raw --width 1025 --height 1025 ../data/texture/heightmap_2mill_points.raw ../data/texture/heightbig.ktx2
# gltf-transform inspect ../data/mesh/t01.glb
# recreate raw heightmap: C:\tools\vcpkg\packages\ktx_x64-windows\tools\ktx\ktx extract --raw .\height.ktx2 - >out.raw
# gltf-transform metalrough ../data/mesh/terrain/Terrain_Mesh_0_0.gltf ../data/mesh/terrain/terrain.gltf
#gltf-transform metalrough ../data/mesh/terrain/Terrain_Mesh_0_0.gltf ../data/mesh/terrain/terrain.gltf
#gltf-transform uastc ../data/mesh/t01.glb ../data/mesh/t01_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/terrain.glb ../data/mesh/terrain_cmp.glb --level 4 --zstd 18 --verbose
# cmp means compressed :-)
#gltf-transform uastc ../data/mesh/box1.glb ../data/mesh/box1_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/box10.glb ../data/mesh/box10_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/box100.glb ../data/mesh/box100_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/cyberpunk_pistol.glb ../data/mesh/cyberpunk_pistol_cmp.glb --level 4 --zstd 18 --verbose
#cd ../data/raw
#toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 --cubemap cube_sky.ktx2 px.png nx.png py.png ny.png pz.png nz.png
#ktx create --format R32_SFLOAT --raw --width 2048 --height 2048 "../data/texture/valley03_Height Map_2048x2048_0_0.raw" ../data/texture/valley_height.ktx2
#ktx create --format R32_SFLOAT --raw --width 2048 --height 2048 "../WorldCreator/flat_Height Map_2048x2048_0_0.raw" ../data/texture/flat.ktx2
#gltf-transform uastc ../data/rocks.glb ../data/mesh/rocks_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/rocks.gltf ../data/mesh/rocks_cmp.gltf --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/rocks.gltf ./test/rocks_cmp.gltf --level 1 --zstd 10 --verbose
#gltf-transform uastc ../data/rocks_multi.glb ../data/mesh/rocks_multi_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/loadingbox.glb ../data/mesh/loadingbox_cmp.glb --level 4 --zstd 18 --verbose
#toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 --cubemap cube.ktx2 px.png nx.png py.png ny.png pz.png nz.png
#gltf-transform uastc ../data/mesh/DamagedHelmet.glb ../data/mesh/DamagedHelmet_cmp.glb --level 4 --zstd 18 --verbose
#toktx --genmipmap --uastc 3 --zcmp 18 --verbose --t2 --cubemap ../data/texture/irr.ktx2 ./build/src/app/irradiance.ktx 
#gltf-transform uastc ../data/mesh/SimpleMaterial.gltf ../data/mesh/SimpleMaterial_cmp.gltf --level 4 --zstd 18 --verbose
#gltf-transform metalrough ../data/mesh/Material_MetallicRoughness/Material_MetallicRoughness_04.gltf ../data/mesh/mirror.glb
#gltf-transform uastc ../data/mesh/mirror.glb ../data/mesh/mirror_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/WaterBottle.glb ../data/mesh/WaterBottle_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/raw/desert.glb ../data/mesh/desert_cmp.glb --level 1 --zstd 10 --verbose
#gltf-transform uastc ../data/raw/desert3.glb ../data/mesh/desert3_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/raw/Terrain.glb ../data/mesh/Terrain_cmp.glb --level 4 --zstd 18 --verbose
#gltf-transform uastc ../data/mesh/cc_facial_exp.glb ../data/mesh/cc_facial_exp_cmp.glb --level 4 --zstd 18 --verbose

# run gltf-tranform scripts:
# npm install @gltf-transform/core
#node set-single-sided.mjs ../data/granite_rock_double_sided.glb ../data/granite_rock.glb
#gltf-transform uastc ../data/granite_rock_lod.glb ../data/mesh/granite_rock_lod_cmp.glb --level 4 --zstd 18 --verbose
gltf-transform uastc ../data/granite_rock_lod.glb ../data/mesh/granite_rock_lod_cmp.glb --verbose
