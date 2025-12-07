Go to [the Miniscript website](http://bitcoin.sipa.be/miniscript/).

This repository contains a C++ implementation of Miniscript and a number of
related things:
* The core Miniscript module ([cpp](bitcoin/script/miniscript.cpp), [h](bitcoin/script/miniscript.h)) together with a number of [dependencies](bitcoin/) based on
  the Bitcoin Core source code.
* A policy to Miniscript compiler ([cpp](compiler.cpp), [h](compiler.h)).
* **NEW:** Miniscript to JSON/AST converter ([cpp](miniscript_json.cpp), [h](miniscript_json.h)).
* Javascript wrappers for the website ([cpp](js_bindings.cpp)).
* The project website ([.html](index.html)).

## JSON/AST Output

This fork adds a `miniscript_to_json` function that converts a Miniscript expression into a structured JSON/AST representation. This is useful for:
- Building visualization tools
- Programmatic analysis of Miniscript structure
- Debugging and educational purposes

### Usage from JavaScript (via WebAssembly)

```javascript
// Using cwrap (recommended)
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

### Example JSON Output

For the input `or_i(pk(A),pk(B))`:

```json
{
  "miniscript": "or_i(pk(A),pk(B))",
  "valid": true,
  "is_valid_top_level": true,
  "is_sane": true,
  "root": {
    "id": "n0",
    "fragment": "or_i",
    "miniscript": "or_i(pk(A),pk(B))",
    "args": [],
    "props": {
      "type": "B",
      "modifiers": ["d", "u", "s"],
      "is_valid": true,
      "is_nonmalleable": true,
      "needs_signature": true,
      "timelock_mix_ok": true,
      "script_size": 73,
      "max_ops": 3,
      "max_stack_size": 3
    },
    "children": [
      {
        "id": "n1",
        "fragment": "c",
        "miniscript": "pk(A)",
        "args": [],
        "is_wrapper": true,
        "props": { ... },
        "children": [
          {
            "id": "n2",
            "fragment": "pk_k",
            "miniscript": "pk(A)",
            "args": ["A"],
            "props": { ... },
            "children": []
          }
        ]
      },
      {
        "id": "n3",
        "fragment": "c",
        "miniscript": "pk(B)",
        ...
      }
    ]
  }
}
```

### JSON Schema

Each node in the AST contains:
- `id`: Unique identifier (e.g., "n0", "n1", ...)
- `fragment`: The fragment type (e.g., "pk_k", "and_v", "or_c", "older", etc.)
- `miniscript`: The miniscript string for this subtree
- `args`: Scalar arguments (keys, hashes, numbers)
- `is_wrapper`: Boolean, present if this is a wrapper (a:, s:, c:, d:, v:, j:, n:)
- `props`: Type properties object:
  - `type`: Basic type (B, V, K, W)
  - `modifiers`: Array of modifier flags (z, o, n, d, u, e, f, s, m, x, k)
  - `is_valid`, `is_nonmalleable`, `needs_signature`, `timelock_mix_ok`: Boolean flags
  - `script_size`, `max_ops`, `max_stack_size`: Numeric properties
- `children`: Array of child nodes

### Test Page

Open `test_json.html` in a browser (served via HTTP) to see the JSON output for various test cases.

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

