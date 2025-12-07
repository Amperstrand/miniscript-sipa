# GitHub Pages Setup

The code is now pushed to the `json` branch and ready for GitHub Pages deployment.

## How to Enable GitHub Pages

1. Go to your repository: https://github.com/Amperstrand/miniscript-sipa
2. Click **Settings** (gear icon)
3. Scroll down to the **Pages** section (left sidebar)
4. Under "Build and deployment", select:
   - **Source**: Deploy from a branch
   - **Branch**: Select `json` 
   - **Folder**: `/root` (or leave as default for the root directory)
5. Click **Save**

GitHub will automatically deploy the site. It will be available at:
https://Amperstrand.github.io/miniscript-sipa/

## What's Included

The GitHub Pages site includes:

- **index.html** - Main compiler and analyzer interface
  - Compile policies to miniscript
  - Generate JSON/AST representation
  - Generate simple Rete.js JSON (policy only)
  - Generate complete Rete.js JSON (with BIP39 keys, Descriptor, Address)
  - View graphs directly in miniscript.fun

- **test_rete.html** - Dedicated Rete.js testing page
  - Test Rete JSON generation with examples
  - Compare simple vs. complete output
  - Copy/paste JSON or open in miniscript.fun

- **unit_test.html** - Unit tests for JSON/AST output

- **miniscript.js + miniscript.wasm** - WebAssembly module with all functions exported

## Features

✅ Miniscript compilation from policies  
✅ JSON/AST output for all miniscripts  
✅ Simple Rete.js graph (policy structure only)  
✅ Complete Rete.js graph (with dummy BIP39 keys)  
✅ Direct integration with miniscript.fun  
✅ Automatic node layout based on tree depth  
✅ Full connection serialization for miniscript.fun rendering  

## Key Functions

- `miniscript_compile()` - Compile a spending policy
- `miniscript_analyze()` - Analyze an existing miniscript
- `miniscript_to_json()` - Convert to JSON/AST
- `miniscript_to_rete_json()` - Convert to simple Rete.js JSON
- `miniscript_to_complete_rete_json()` - Convert to complete Rete.js JSON with dummy nodes

All functions are exported via Emscripten and available to JavaScript.
