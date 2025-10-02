import { NodeIO } from '@gltf-transform/core';
import path from 'path';
import { fileURLToPath } from 'url';

// Fix for __dirname in ES modules
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Get input/output filenames from command-line arguments
const [,, inputFile, outputFile] = process.argv;

if (!inputFile || !outputFile) {
    console.error('Usage: node set-single-sided.mjs <input.glb> <output.glb>');
    process.exit(1);
}

const io = new NodeIO();
const document = await io.read(inputFile);  // ✅ Use await here!

const root = document.getRoot();

for (const material of root.listMaterials()) {
    material.setDoubleSided(false);
}

await io.write(outputFile, document);
console.log(`✅ Converted ${path.basename(inputFile)} to single-sided and saved as ${path.basename(outputFile)}`);
