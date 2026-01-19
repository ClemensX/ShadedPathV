# GPU Structure Logger

## Overview

The `logGPUStructuresMarkdown()` utility method generates comprehensive markdown-formatted documentation of GPU structures in your ShadedPathV engine. This is particularly useful for debugging, optimization, and understanding how your meshes and objects are organized on the GPU.

## Usage

### Basic Usage (Log to Console)

```cpp
// Log to console
engine.util.logGPUStructuresMarkdown();
```

### Save to File

```cpp
// Save to markdown file
engine.util.logGPUStructuresMarkdown("gpu_structures_report.md");
```

## Report Sections

The generated report includes the following sections:

### 1. MeshStore Overview
- Total number of meshes loaded
- Maximum mesh capacity
- GPU mesh indices and info buffer sizes

### 2. GPU Base Addresses
A table showing the base addresses of major GPU structures:
- Global GPU memory chunk
- Push constants for indices
- Push constants for mesh info

### 3. Mesh Collections
Details about all loaded mesh collections:
- Collection ID and filename
- Availability status
- Number of meshes in collection
- Flags (PBR, SKINNED, LOD, etc.)

### 4. Detailed Mesh Information
For each mesh:
- Mesh ID and number
- Collection index
- Availability status
- Vertex and index counts
- Meshlet count
- LOD level

### 5. GPU Mesh Storage Addresses
Memory addresses on GPU for each mesh:
- Base address
- Vertex buffer offset
- Global index offset
- Local index offset
- Meshlet descriptor offset

### 6. GPU Mesh Index Table
Shows the LOD group organization (10 LOD levels per group)

### 7. GPU Mesh Info Details
Detailed information about GPU mesh info structures:
- Memory offsets for each buffer type
- Meshlet counts

### 8. Meshlet Statistics
Statistical information about meshlets:
- Total meshlet count per mesh
- Average vertices per meshlet
- Average primitives per meshlet
- Whether meshlet storage file was found

### 9. Dynamic Model UBO
Information about the dynamic uniform buffer:
- Aligned size
- Next free index
- Structure size

### 10. Texture Usage by Mesh
Which textures are used by each mesh:
- Base color texture
- Metallic roughness texture
- Normal map
- Occlusion map
- Emissive texture

### 11. Bounding Boxes
Bounding box information for each mesh:
- Min/Max coordinates
- Size in each dimension

## Example Output

```markdown
# GPU Structures Report

Generated: 133776288000000000

---

## MeshStore Overview

- **Total Meshes:** 45
- **Max Meshes:** 1000
- **GPU Mesh Indices Size:** 100
- **GPU Mesh Infos Size:** 1000

## GPU Base Addresses

| Structure | Base Address (hex) | Base Address (dec) |
|-----------|-------------------|-------------------|
| Global GPU Memory Chunk | 0x7f8a4c000000 | 140236300058624 |
| Push Constants - Indices | 0x7f8a4c000000 | 140236300058624 |
| Push Constants - Infos | 0x7f8a4c000800 | 140236300060672 |

... (additional sections) ...
```

## Best Practices

1. **Generate after initialization**: Call this method after all meshes have been loaded and uploaded to GPU
2. **Use for debugging**: Particularly useful when troubleshooting GPU memory issues or LOD problems
3. **Compare reports**: Generate reports at different stages to track changes
4. **Archive reports**: Keep reports as documentation for your project's memory layout

## Implementation Details

The logger accesses the following structures:
- `MeshStore` - mesh and collection management
- `PBRShader` - dynamic uniform buffer information
- `GlobalRendering` - GPU memory chunk information
- `MeshInfo` - individual mesh details
- `GPUMeshIndex` / `GPUMeshInfo` - GPU-side index structures

## Notes

- The report uses markdown tables for easy viewing in markdown viewers
- All GPU addresses are shown in both hexadecimal and decimal format
- The method is safe to call multiple times (non-destructive)
- Output is UTF-8 encoded with unicode checkmarks (✓) and crosses (✗)
