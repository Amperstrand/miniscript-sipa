Go to [the Miniscript website](http://bitcoin.sipa.be/miniscript/).

This repository contains a C++ implementation of Miniscript and a number of
related things:
* The core Miniscript module ([cpp](bitcoin/script/miniscript.cpp), [h](bitcoin/script/miniscript.h)) together with a number of [dependencies](bitcoin/) based on
  the Bitcoin Core source code.
* A policy to Miniscript compiler ([cpp](compiler.cpp), [h](compiler.h)).
* **NEW:** Miniscript to JSON/AST converter ([cpp](miniscript_json.cpp), [h](miniscript_json.h)).
* **NEW:** Miniscript to Rete.js graph format (compatible with [miniscript.fun](https://miniscript.fun/)).
* Javascript wrappers for the website ([cpp](js_bindings.cpp)).
* The project website ([.html](index.html)).

## JSON Output Formats

This fork provides **two JSON output functions**:

| Function | Purpose | Use Case |
|----------|---------|----------|
| `miniscript_to_json` | Full AST with type information | Analysis, debugging, visualization |
| `miniscript_to_rete_json` | Rete.js-compatible graph | Import into miniscript.fun |

Both functions are available:
- In the web interface (index.html) with **copy buttons** 📋
- Via JavaScript using `Module.cwrap()`
- In C++ directly

---

## 1. JSON/AST Output (`miniscript_to_json`)

Converts a Miniscript expression into a structured JSON/AST representation. Useful for:
- Building visualization tools
- Programmatic analysis of Miniscript structure
- Debugging and educational purposes

### Usage from JavaScript

```javascript
const miniscript_to_json = Module.cwrap('miniscript_to_json', 'string', ['string']);
const jsonStr = miniscript_to_json("and_v(pk(A),after(100))");
const ast = JSON.parse(jsonStr);
console.log(ast);
```

### Usage from C++

```cpp
#include "miniscript_json.h"
std::string json = MiniscriptToJson("and_v(pk(A),after(100))");
```

### JSON Schema

Each node in the AST contains:
- `id`: Unique identifier (e.g., "n0", "n1", ...)
- `fragment`: The fragment type (e.g., "pk_k", "and_v", "or_c", "older", etc.)
- `miniscript`: The miniscript string for this subtree
- `args`: Scalar arguments (keys, hashes, numbers)
- `is_wrapper`: Boolean, present if this is a wrapper (a:, s:, c:, d:, v:, j:, n:)
- `props`: Type properties object with `type`, `modifiers`, validity flags, and size metrics
- `children`: Array of child nodes

---

## 2. Rete.js Graph Output (`miniscript_to_rete_json`)

Converts a Miniscript expression into a Rete.js-compatible graph JSON that can be loaded directly into [miniscript.fun](https://miniscript.fun/) via `editor.fromJSON()`.

### Usage from JavaScript

```javascript
const miniscript_to_rete_json = Module.cwrap('miniscript_to_rete_json', 'string', ['string']);
const reteStr = miniscript_to_rete_json("and_v(pk(A),after(100))");
const graph = JSON.parse(reteStr);

// In miniscript.fun:
// editor.fromJSON(graph);
```

### Usage from C++

```cpp
#include "miniscript_json.h"
std::string reteJson = MiniscriptToReteJson("and_v(pk(A),after(100))");
```

### Rete.js JSON Format

```json
{
  "id": "miniscript-sipa@rete-0.1.0",
  "nodes": {
    "1": {
      "id": 1,
      "name": "And",
      "data": {
        "__ms_fragment": "and_v",
        "__ms_str": "and_v(pk(A),after(100))"
      },
      "inputs": {
        "pol1": {"connections": [{"node": 2, "output": "key", "data": {}}]},
        "pol2": {"connections": [{"node": 3, "output": "pol", "data": {}}]}
      },
      "outputs": {"pol": {"connections": []}},
      "position": [100, 100]
    },
    "2": {
      "id": 2,
      "name": "Key",
      "data": {"key": "A", "__ms_fragment": "pk_k"},
      "inputs": {},
      "outputs": {"key": {"connections": [{"node": 1, "input": "pol1", "data": {}}]}},
      "position": [400, 100]
    },
    "3": {
      "id": 3,
      "name": "After",
      "data": {"num": 100, "__ms_fragment": "after"},
      "inputs": {},
      "outputs": {"pol": {"connections": [{"node": 1, "input": "pol2", "data": {}}]}},
      "position": [400, 300]
    }
  },
  "groups": {},
  "comments": [],
  "__ms_source": "and_v(pk(A),after(100))"
}
```

### Component Mapping

| Miniscript Fragment | Rete Component | Data Fields |
|---------------------|----------------|-------------|
| `pk_k`, `pk_h` | Key | `key` |
| `after` | After | `num` |
| `older` | Older | `num` |
| `and_v`, `and_b` | And | - |
| `or_b`, `or_c`, `or_d`, `or_i` | Or | - |
| `andor` | AndOr | - |
| `thresh` | Threshold | `thresh` |
| `multi` | Multi | `thresh` |
| `sha256`, `hash256`, `ripemd160`, `hash160` | SHA256, Hash256, etc. | `hash` |

Wrapper fragments (a:, s:, c:, d:, v:, j:, n:) are stored in `data.__ms_wrappers` array.

---

## Web Interface

The `index.html` page provides:
- **Compile**: Convert policy to miniscript with JSON output
- **Analyze**: Analyze miniscript with JSON output
- **Copy buttons** 📋: One-click copy for both JSON/AST and Rete.js formats

After compiling or analyzing, expand the JSON sections and click the copy buttons to get the output in your clipboard.

## Building

To build the WebAssembly version:

```bash
# Install Emscripten if needed
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build
cd /path/to/miniscript
make miniscript.js
```

This produces:
- `miniscript.js` - JavaScript glue code
- `miniscript.wasm` - WebAssembly binary

To test locally:
```bash
python3 -m http.server 8080
# Open http://localhost:8080 in your browser
```

