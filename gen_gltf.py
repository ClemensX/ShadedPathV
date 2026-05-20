"""
Blender Python script to generate GLTF test files with textures for parser validation
Compatible with Blender 3.0+ and 4.0+
Run in Blender: Open Blender > Scripting tab > Paste this script > Run Script
Or run from command line: blender --background --python generate_gltf_test_files.py
"""

import bpy
import os
import math

# Configuration - set this to your test data folder
OUTPUT_DIR = r"C:\dev\cpp\ShadedPathV\test_samples\mesh"
TEXTURE_DIR = os.path.join(OUTPUT_DIR, "textures")

def clear_scene():
    """Remove all objects, meshes, materials, images from scene"""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    
    # Clear orphaned data
    for block in bpy.data.meshes:
        bpy.data.meshes.remove(block)
    for block in bpy.data.materials:
        bpy.data.materials.remove(block)
    for block in bpy.data.images:
        if block.name not in ['Render Result', 'Viewer Node']:
            bpy.data.images.remove(block)

def create_checker_texture(name, color1, color2, resolution=512):
    """Create a simple checker pattern texture"""
    # Create new image
    img = bpy.data.images.new(name=name, width=resolution, height=resolution, alpha=False)
    
    # Generate checker pattern
    pixels = [None] * resolution * resolution * 4
    checker_size = resolution // 8
    
    for y in range(resolution):
        for x in range(resolution):
            checker_x = (x // checker_size) % 2
            checker_y = (y // checker_size) % 2
            is_white = (checker_x + checker_y) % 2
            
            color = color1 if is_white else color2
            idx = (y * resolution + x) * 4
            pixels[idx:idx+3] = color
            pixels[idx+3] = 1.0  # Alpha
    
    img.pixels = pixels
    
    # Save to file
    img.filepath_raw = os.path.join(TEXTURE_DIR, f"{name}.png")
    img.file_format = 'PNG'
    img.save()
    
    return img

def create_solid_texture(name, color, resolution=512):
    """Create a solid color texture"""
    img = bpy.data.images.new(name=name, width=resolution, height=resolution, alpha=False)
    
    pixels = [None] * resolution * resolution * 4
    for i in range(0, len(pixels), 4):
        pixels[i:i+3] = color
        pixels[i+3] = 1.0
    
    img.pixels = pixels
    
    # Save to file
    img.filepath_raw = os.path.join(TEXTURE_DIR, f"{name}.png")
    img.file_format = 'PNG'
    img.save()
    
    return img

def create_normal_map(name, resolution=512):
    """Create a simple normal map (bluish, flat)"""
    img = bpy.data.images.new(name=name, width=resolution, height=resolution, alpha=False)
    
    # Normal map for flat surface points up: RGB(0.5, 0.5, 1.0)
    pixels = [0.5, 0.5, 1.0, 1.0] * (resolution * resolution)
    
    img.pixels = pixels
    
    # Save to file
    img.filepath_raw = os.path.join(TEXTURE_DIR, f"{name}.png")
    img.file_format = 'PNG'
    img.save()
    
    return img

def create_roughness_metallic_texture(name, roughness=0.5, metallic=0.0, resolution=512):
    """Create a roughness/metallic texture (combined in one texture)"""
    img = bpy.data.images.new(name=name, width=resolution, height=resolution, alpha=False)
    
    # For glTF: Green channel = Roughness, Blue channel = Metallic
    pixels = [0.0, roughness, metallic, 1.0] * (resolution * resolution)
    
    img.pixels = pixels
    
    # Save to file
    img.filepath_raw = os.path.join(TEXTURE_DIR, f"{name}.png")
    img.file_format = 'PNG'
    img.save()
    
    return img

def get_separate_color_node_type():
    """Get the correct node type for separating colors (version-dependent)"""
    # Blender 3.0+ uses ShaderNodeSeparateColor, older versions use ShaderNodeSeparateRGB
    try:
        # Try the new node type first
        return 'ShaderNodeSeparateColor'
    except:
        return 'ShaderNodeSeparateRGB'

def create_pbr_material(name, base_color, use_textures=True, texture_name=None):
    """Create a PBR material with textures"""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    
    # Clear default nodes
    nodes.clear()
    
    # Add Principled BSDF
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    
    # Add Material Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (300, 0)
    
    # Link BSDF to output
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])
    
    if use_textures and texture_name:
        # Base Color Texture
        tex_image = nodes.new(type='ShaderNodeTexImage')
        tex_image.location = (-400, 300)
        tex_image.image = create_checker_texture(
            f"{texture_name}_BaseColor",
            base_color,
            tuple(c * 0.5 for c in base_color)  # Darker version
        )
        links.new(tex_image.outputs['Color'], bsdf.inputs['Base Color'])
        
        # Normal Map
        normal_tex = nodes.new(type='ShaderNodeTexImage')
        normal_tex.location = (-400, 0)
        normal_tex.image = create_normal_map(f"{texture_name}_Normal")
        normal_tex.image.colorspace_settings.name = 'Non-Color'
        
        normal_map = nodes.new(type='ShaderNodeNormalMap')
        normal_map.location = (-100, 0)
        links.new(normal_tex.outputs['Color'], normal_map.inputs['Color'])
        links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])
        
        # Metallic/Roughness Texture
        mr_tex = nodes.new(type='ShaderNodeTexImage')
        mr_tex.location = (-400, -300)
        mr_tex.image = create_roughness_metallic_texture(f"{texture_name}_MetallicRoughness")
        mr_tex.image.colorspace_settings.name = 'Non-Color'
        
        # Separate color channels - use version-appropriate node
        # Try new node type first (Blender 3.0+)
        try:
            separate_node = nodes.new(type='ShaderNodeSeparateColor')
            separate_node.location = (-100, -300)
            links.new(mr_tex.outputs['Color'], separate_node.inputs['Color'])
            # ShaderNodeSeparateColor outputs: Red, Green, Blue
            links.new(separate_node.outputs['Green'], bsdf.inputs['Roughness'])
            links.new(separate_node.outputs['Blue'], bsdf.inputs['Metallic'])
        except:
            # Fallback to old node type (Blender 2.x)
            separate_node = nodes.new(type='ShaderNodeSeparateRGB')
            separate_node.location = (-100, -300)
            links.new(mr_tex.outputs['Color'], separate_node.inputs['Image'])
            links.new(separate_node.outputs['G'], bsdf.inputs['Roughness'])
            links.new(separate_node.outputs['B'], bsdf.inputs['Metallic'])
    else:
        # Just use solid colors
        bsdf.inputs['Base Color'].default_value = (*base_color, 1.0)
        bsdf.inputs['Metallic'].default_value = 0.0
        bsdf.inputs['Roughness'].default_value = 0.5
    
    return mat

def uv_unwrap_object(obj):
    """UV unwrap object using smart UV project"""
    # Select object
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    
    # Enter edit mode
    bpy.ops.object.mode_set(mode='EDIT')
    
    # Select all faces
    bpy.ops.mesh.select_all(action='SELECT')
    
    # Smart UV Project
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
    
    # Return to object mode
    bpy.ops.object.mode_set(mode='OBJECT')

def export_gltf(filepath, use_selection=False):
    """Export scene to GLTF with textures - Multi-version compatible"""
    blender_version = bpy.app.version
    
    # Base parameters that work across all versions
    export_params = {
        'filepath': filepath,
        'export_format': 'GLTF_SEPARATE',
        'use_selection': use_selection,
        'export_materials': 'EXPORT',
        'export_image_format': 'AUTO',
        'export_texture_dir': 'textures',
        'export_normals': True,
        'export_texcoords': True,  # This works in all versions
    }
    
    # Version-specific parameters
    if blender_version >= (5, 0, 0):
        # Blender 5.0+ - export_colors removed, export_attributes added
        export_params.update({
            'export_attributes': True,  # New in 5.0
        })
    elif blender_version >= (3, 0, 0):
        # Blender 3.x and 4.x
        export_params.update({
            'export_colors': True,
            'export_yup': True,
        })
    
    bpy.ops.export_scene.gltf(**export_params)
    print(f"Exported: {filepath}")

def create_pbr_material(name, base_color, use_textures=True, texture_name=None):
    """Create a PBR material with textures"""
    mat = bpy.data.materials.new(name=name)
    
    # Suppress deprecation warning for Blender 5.0+
    if not mat.use_nodes:
        mat.use_nodes = True
    
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    
    # Clear default nodes
    nodes.clear()
    
    # Add Principled BSDF
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    
    # Add Material Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (300, 0)
    
    # Link BSDF to output
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])
    
    if use_textures and texture_name:
        # Base Color Texture
        tex_image = nodes.new(type='ShaderNodeTexImage')
        tex_image.location = (-400, 300)
        tex_image.image = create_checker_texture(
            f"{texture_name}_BaseColor",
            base_color,
            tuple(c * 0.5 for c in base_color)  # Darker version
        )
        links.new(tex_image.outputs['Color'], bsdf.inputs['Base Color'])
        
        # Normal Map
        normal_tex = nodes.new(type='ShaderNodeTexImage')
        normal_tex.location = (-400, 0)
        normal_tex.image = create_normal_map(f"{texture_name}_Normal")
        normal_tex.image.colorspace_settings.name = 'Non-Color'
        
        normal_map = nodes.new(type='ShaderNodeNormalMap')
        normal_map.location = (-100, 0)
        links.new(normal_tex.outputs['Color'], normal_map.inputs['Color'])
        links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])
        
        # Metallic/Roughness Texture
        mr_tex = nodes.new(type='ShaderNodeTexImage')
        mr_tex.location = (-400, -300)
        mr_tex.image = create_roughness_metallic_texture(f"{texture_name}_MetallicRoughness")
        mr_tex.image.colorspace_settings.name = 'Non-Color'
        
        # Separate color channels - use version-appropriate node
        try:
            separate_node = nodes.new(type='ShaderNodeSeparateColor')
            separate_node.location = (-100, -300)
            links.new(mr_tex.outputs['Color'], separate_node.inputs['Color'])
            links.new(separate_node.outputs['Green'], bsdf.inputs['Roughness'])
            links.new(separate_node.outputs['Blue'], bsdf.inputs['Metallic'])
        except:
            separate_node = nodes.new(type='ShaderNodeSeparateRGB')
            separate_node.location = (-100, -300)
            links.new(mr_tex.outputs['Color'], separate_node.inputs['Image'])
            links.new(separate_node.outputs['G'], bsdf.inputs['Roughness'])
            links.new(separate_node.outputs['B'], bsdf.inputs['Metallic'])
    else:
        # Just use solid colors
        bsdf.inputs['Base Color'].default_value = (*base_color, 1.0)
        bsdf.inputs['Metallic'].default_value = 0.0
        bsdf.inputs['Roughness'].default_value = 0.5
    
    return mat

# =============================================================================
# TEST FILE 1: Simple single mesh (cube) with texture
# =============================================================================
def generate_cube_single():
    """Generate cube_single.gltf - simple cube with textured material"""
    print("\n=== Generating cube_single.gltf ===")
    clear_scene()
    
    # Create cube
    bpy.ops.mesh.primitive_cube_add(size=2.0, location=(0, 0, 0))
    cube = bpy.context.active_object
    cube.name = "SimpleCube"
    
    # UV unwrap
    uv_unwrap_object(cube)
    
    # Apply textured material
    mat = create_pbr_material("CubeMaterial", (0.8, 0.3, 0.3), True, "Cube")
    cube.data.materials.append(mat)
    
    # Export
    export_gltf(os.path.join(OUTPUT_DIR, "cube_single.gltf"))

# =============================================================================
# TEST FILE 2: 10 LOD levels with textures
# =============================================================================
def generate_mesh_with_lods():
    """Generate mesh_with_lods.gltf - 10 spheres with textures"""
    print("\n=== Generating mesh_with_lods.gltf ===")
    clear_scene()
    
    mat = create_pbr_material("LODMaterial", (0.3, 0.8, 0.3), True, "LOD")
    
    # Create 10 LOD levels
    lod_subdivisions = [4, 4, 3, 3, 2, 2, 1, 1, 0, 0]
    
    for lod_level in range(10):
        subdivs = lod_subdivisions[lod_level]
        
        # Create sphere
        bpy.ops.mesh.primitive_uv_sphere_add(
            segments=max(8, 32 - lod_level * 3),
            ring_count=max(6, 16 - lod_level * 1),
            radius=1.0,
            location=(lod_level * 2.5, 0, 0)
        )
        
        sphere = bpy.context.active_object
        sphere.name = f"mesh_lod_{lod_level}"
        
        # UV unwrap
        uv_unwrap_object(sphere)
        
        # Apply material
        sphere.data.materials.append(mat)
        
        # Optional subdivision
        if subdivs > 0:
            mod = sphere.modifiers.new(name="Subsurf", type='SUBSURF')
            mod.levels = subdivs
            mod.render_levels = subdivs
            bpy.ops.object.modifier_apply(modifier="Subsurf")
    
    # Export all
    export_gltf(os.path.join(OUTPUT_DIR, "mesh_with_lods.gltf"))

# =============================================================================
# TEST FILE 3: Single mesh with 2 primitives (tree with textures)
# =============================================================================
def generate_tree_primitives():
    """Generate tree_primitives.gltf - textured trunk and foliage"""
    print("\n=== Generating tree_primitives.gltf ===")
    clear_scene()
    
    # Create trunk
    bpy.ops.mesh.primitive_cylinder_add(
        radius=0.3,
        depth=3.0,
        location=(0, 0, 1.5)
    )
    trunk = bpy.context.active_object
    trunk.name = "Tree"
    
    # UV unwrap trunk
    uv_unwrap_object(trunk)
    
    # Trunk material with texture
    mat_trunk = create_pbr_material("TrunkMaterial", (0.4, 0.2, 0.1), True, "Trunk")
    trunk.data.materials.append(mat_trunk)
    
    # Create foliage
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=2,
        radius=1.5,
        location=(0, 0, 3.5)
    )
    foliage = bpy.context.active_object
    foliage.name = "Tree_Foliage"
    
    # UV unwrap foliage
    uv_unwrap_object(foliage)
    
    # Foliage material with texture
    mat_foliage = create_pbr_material("FoliageMaterial", (0.1, 0.6, 0.1), True, "Foliage")
    foliage.data.materials.append(mat_foliage)
    
    # Join objects to create multi-primitive mesh
    bpy.ops.object.select_all(action='DESELECT')
    trunk.select_set(True)
    foliage.select_set(True)
    bpy.context.view_layer.objects.active = trunk
    bpy.ops.object.join()
    trunk.name = "Tree"
    
    # Export
    export_gltf(os.path.join(OUTPUT_DIR, "tree_primitives.gltf"))

# =============================================================================
# TEST FILE 4: Complex - 10 LODs with 2 primitives each, all textured
# =============================================================================
def generate_tree_lod_primitives():
    """Generate tree_lod_primitives.gltf - 10 textured LOD trees"""
    print("\n=== Generating tree_lod_primitives.gltf ===")
    clear_scene()
    
    mat_trunk = create_pbr_material("TrunkMaterial", (0.4, 0.2, 0.1), True, "TreeLODTrunk")
    mat_foliage = create_pbr_material("FoliageMaterial", (0.1, 0.6, 0.1), True, "TreeLODFoliage")
    
    for lod_level in range(10):
        x_offset = lod_level * 5.0
        trunk_segments = max(8, 32 - lod_level * 2)
        
        # Create trunk
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=trunk_segments,
            radius=0.3,
            depth=3.0,
            location=(x_offset, 0, 1.5)
        )
        trunk = bpy.context.active_object
        trunk.name = f"tree_lod_{lod_level}"
        
        # UV unwrap trunk
        uv_unwrap_object(trunk)
        trunk.data.materials.append(mat_trunk)
        
        # Create foliage
        foliage_subdivs = max(1, 3 - lod_level // 3)
        bpy.ops.mesh.primitive_ico_sphere_add(
            subdivisions=foliage_subdivs,
            radius=1.5,
            location=(x_offset, 0, 3.5)
        )
        foliage = bpy.context.active_object
        foliage.name = f"tree_lod_{lod_level}_foliage"
        
        # UV unwrap foliage
        uv_unwrap_object(foliage)
        foliage.data.materials.append(mat_foliage)
        
        # Join to create multi-primitive mesh
        bpy.ops.object.select_all(action='DESELECT')
        trunk.select_set(True)
        foliage.select_set(True)
        bpy.context.view_layer.objects.active = trunk
        bpy.ops.object.join()
        trunk.name = f"tree_lod_{lod_level}"
    
    # Export all
    export_gltf(os.path.join(OUTPUT_DIR, "tree_lod_primitives.gltf"))

# =============================================================================
# TEST FILE 5: Multiple separate meshes with different textures
# =============================================================================
def generate_multiple_objects():
    """Generate multiple_objects.gltf - 3 textured objects"""
    print("\n=== Generating multiple_objects.gltf ===")
    clear_scene()
    
    # Cube
    bpy.ops.mesh.primitive_cube_add(size=1.5, location=(-3, 0, 0))
    cube = bpy.context.active_object
    cube.name = "Cube"
    uv_unwrap_object(cube)
    mat_cube = create_pbr_material("CubeMaterial", (0.8, 0.2, 0.2), True, "MultiCube")
    cube.data.materials.append(mat_cube)
    
    # Sphere
    bpy.ops.mesh.primitive_uv_sphere_add(radius=1.0, location=(0, 0, 0))
    sphere = bpy.context.active_object
    sphere.name = "Sphere"
    uv_unwrap_object(sphere)
    mat_sphere = create_pbr_material("SphereMaterial", (0.2, 0.8, 0.2), True, "MultiSphere")
    sphere.data.materials.append(mat_sphere)
    
    # Cylinder
    bpy.ops.mesh.primitive_cylinder_add(radius=0.7, depth=2.0, location=(3, 0, 0))
    cylinder = bpy.context.active_object
    cylinder.name = "Cylinder"
    uv_unwrap_object(cylinder)
    mat_cylinder = create_pbr_material("CylinderMaterial", (0.2, 0.2, 0.8), True, "MultiCylinder")
    cylinder.data.materials.append(mat_cylinder)
    
    # Export all
    export_gltf(os.path.join(OUTPUT_DIR, "multiple_objects.gltf"))

# =============================================================================
# MAIN EXECUTION
# =============================================================================
def main():
    """Generate all test GLTF files with textures"""
    
    # Create directories
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(TEXTURE_DIR, exist_ok=True)
    
    print(f"Blender version: {bpy.app.version_string}")
    print(f"Output directory: {OUTPUT_DIR}")
    print(f"Texture directory: {TEXTURE_DIR}")
    print("=" * 60)
    
    # Generate all test files
    try:
        generate_cube_single()
        generate_mesh_with_lods()
        generate_tree_primitives()
        generate_tree_lod_primitives()
        generate_multiple_objects()
        
        print("=" * 60)
        print("✓ All test files with textures generated successfully!")
        print(f"GLTF files: {OUTPUT_DIR}")
        print(f"Texture files: {TEXTURE_DIR}")
    except Exception as e:
        print(f"✗ Error generating files: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()