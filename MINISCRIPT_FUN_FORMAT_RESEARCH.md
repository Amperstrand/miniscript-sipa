# Miniscript.fun Rete.js JSON Format - Complete Reference

This document contains the authoritative reference for the miniscript.fun Rete.js JSON format,
based on analysis of the Rete.js v1 docs, miniscript-builder repo, and decoded examples.

## 1. Top-Level JSON Shape

```json
{
  "id": "demo@0.1.0",
  "nodes": { ... },
  "comments": [],
  "network": "bitcoin"
}
```

### Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Must be `"demo@0.1.0"` to match miniscript.fun's editor |
| `nodes` | object | Yes | Map of node ID (string) → node object |
| `comments` | array | Yes | Comments from Rete's comments plugin, can be `[]` |
| `network` | string | Optional | One of: `"bitcoin"`, `"testnet"`, `"regtest"`, `"signet"` |

## 2. Node Shape

```json
{
  "id": 3,
  "name": "Key",
  "data": { "key": "03abc..." },
  "inputs": {
    "key": {
      "connections": [
        { "node": 2, "output": "key", "data": {} }
      ]
    }
  },
  "outputs": {
    "key": {
      "connections": [
        { "node": 5, "input": "policies", "data": {} }
      ]
    }
  },
  "position": [80, 400]
}
```

### Node Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | number | Integer ID, must match the string key in `nodes` object |
| `name` | string | Component name (e.g., "Key", "BIP39", "Threshold") |
| `data` | object | Component-specific configuration |
| `inputs` | object | Input sockets, keyed by socket name |
| `outputs` | object | Output sockets, keyed by socket name |
| `position` | [x, y] | Canvas position in pixels |

## 3. Connection Format

### On an input socket (who connects TO this node):
```json
"inputs": {
  "key": {
    "connections": [
      {
        "node": 2,          // source node id (number)
        "output": "key",    // source socket name
        "data": {}
      }
    ]
  }
}
```

### On an output socket (where this node connects TO):
```json
"outputs": {
  "pol": {
    "connections": [
      {
        "node": 6,         // target node id (number)
        "input": "pol",    // target socket name
        "data": {}
      }
    ]
  }
}
```

### Critical Rule: Double-Entry Bookkeeping

For each edge `A.outSocket → B.inSocket`, you **must** update BOTH:
1. `nodes[A].outputs[outSocket].connections.push({ node: B.id, input: inSocket, data: {} })`
2. `nodes[B].inputs[inSocket].connections.push({ node: A.id, output: outSocket, data: {} })`

## 4. Confirmed Components

### 4.1 BIP39 (Seed Node)

| Property | Value |
|----------|-------|
| **name** | `"BIP39"` |
| **inputs** | none |
| **outputs** | `key` |
| **data.mnemonic** | string - 12/24 word seed phrase |
| **data.password** | string - optional passphrase |
| **data.derivation** | string - derivation path (e.g., `"m/48h/0h/0h/2h"`) |

### 4.2 Key

| Property | Value |
|----------|-------|
| **name** | `"Key"` |
| **inputs** | `key` (optional, from BIP39) |
| **outputs** | `key` |
| **data.key** | string (optional) - literal pubkey/xpub when not derived |

**Two forms:**
- **Derived**: `inputs.key` connected to BIP39, `data` is `{}`
- **Literal**: `inputs.key.connections` is `[]`, `data.key` contains the key string

### 4.3 Older (CSV Timelock)

| Property | Value |
|----------|-------|
| **name** | `"Older"` |
| **inputs** | none |
| **outputs** | `pol` |
| **data.num** | number - blocks for CSV timelock |

### 4.4 After (CLTV Timelock)

| Property | Value |
|----------|-------|
| **name** | `"After"` |
| **inputs** | none |
| **outputs** | `pol` |
| **data.num** | number - block height/time for CLTV |

### 4.5 Threshold

| Property | Value |
|----------|-------|
| **name** | `"Threshold"` |
| **inputs** | `policies` (single socket, multiple connections) |
| **outputs** | `pol` |
| **data.thresh** | number - threshold k |

**Important:** All N children connect to the SAME `policies` socket as an array of connections.

### 4.6 And

| Property | Value |
|----------|-------|
| **name** | `"And"` |
| **inputs** | `pol1`, `pol2` |
| **outputs** | `pol` |
| **data** | `{}` |

### 4.7 Or

| Property | Value |
|----------|-------|
| **name** | `"Or"` |
| **inputs** | `pol1`, `pol2` |
| **outputs** | `pol` |
| **data** | `{}` |

### 4.8 AndOr - NOT SUPPORTED

**IMPORTANT:** miniscript.fun does NOT have an AndOr component!

When generating JSON from Miniscript that contains `andor(cond, then, else)`, we decompose it:
- `andor(A, B, C)` → `or(and(A, B), C)`

This creates:
1. An **And** node with inputs from A (cond) and B (then)
2. An **Or** node with inputs from the And result and C (else)

### 4.9 Hash Functions (SHA256, Hash256, RIPEMD160, Hash160)

| Property | Value |
|----------|-------|
| **name** | `"SHA256"`, `"Hash256"`, `"RIPEMD160"`, `"Hash160"` |
| **inputs** | none |
| **outputs** | `pol` |
| **data.hash** | string - hex-encoded hash preimage |

### 4.10 Descriptor

| Property | Value |
|----------|-------|
| **name** | `"Descriptor"` |
| **inputs** | `pol` |
| **outputs** | `desc` |
| **data** | `{}` (type selection may be in UI) |

### 4.11 Address

| Property | Value |
|----------|-------|
| **name** | `"Address"` |
| **inputs** | `desc` |
| **outputs** | `addr` |
| **data.idx** | number - derivation index |

## 5. Socket Name Reference

| Component | Input Sockets | Output Socket |
|-----------|--------------|---------------|
| BIP39 | (none) | `key` |
| Key | `key` | `key` |
| And | `pol1`, `pol2` | `pol` |
| Or | `pol1`, `pol2` | `pol` |
| Threshold | `policies` | `pol` |
| Multi | `policies` (likely) | `pol` |
| Older | (none) | `pol` |
| After | (none) | `pol` |
| SHA256 | (none) | `pol` |
| Hash256 | (none) | `pol` |
| RIPEMD160 | (none) | `pol` |
| Hash160 | (none) | `pol` |
| Descriptor | `pol` | `desc` |
| Address | `desc` | `addr` |

## 6. Wrappers (a:, s:, c:, d:, v:, j:, n:)

**Important:** Miniscript wrappers are NOT represented in the JSON.

- The graph represents **policy language**, not raw Miniscript
- Wrappers are derived internally by rust-miniscript's policy compiler
- They do not appear as separate nodes or data fields
- When generating from Miniscript, you should map to policy-level nodes

## 7. Validation Rules

1. **Node ID consistency**: Key `"5"` must contain `{ "id": 5, ... }`
2. **Component names**: Must match exactly (case-sensitive)
3. **Socket names**: Must match exactly (e.g., `pol` not `policy`)
4. **Empty sockets**: Must still include the socket with empty connections array
5. **Bidirectional connections**: Both input and output sides must be populated
6. **Top-level ID**: Must be `"demo@0.1.0"`

## 8. Position & Layout

- Positions are cosmetic only
- miniscript.fun has "Auto arrange" button that recomputes layout
- Any initial positions work; simple left→right layout recommended
- Suggested spacing: 200-300px between nodes

## 9. Example: Complete Graph

```json
{
  "id": "demo@0.1.0",
  "nodes": {
    "1": {
      "id": 1,
      "name": "BIP39",
      "data": {
        "mnemonic": "bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon",
        "password": "",
        "derivation": "m/48h/0h/0h/2h"
      },
      "inputs": {},
      "outputs": {
        "key": {
          "connections": [
            { "node": 2, "input": "key", "data": {} }
          ]
        }
      },
      "position": [-200, 200]
    },
    "2": {
      "id": 2,
      "name": "Key",
      "data": {},
      "inputs": {
        "key": {
          "connections": [
            { "node": 1, "output": "key", "data": {} }
          ]
        }
      },
      "outputs": {
        "key": {
          "connections": [
            { "node": 3, "input": "pol", "data": {} }
          ]
        }
      },
      "position": [100, 200]
    },
    "3": {
      "id": 3,
      "name": "Descriptor",
      "data": {},
      "inputs": {
        "pol": {
          "connections": [
            { "node": 2, "output": "key", "data": {} }
          ]
        }
      },
      "outputs": {
        "desc": {
          "connections": [
            { "node": 4, "input": "desc", "data": {} }
          ]
        }
      },
      "position": [400, 200]
    },
    "4": {
      "id": 4,
      "name": "Address",
      "data": { "idx": 0 },
      "inputs": {
        "desc": {
          "connections": [
            { "node": 3, "output": "desc", "data": {} }
          ]
        }
      },
      "outputs": {
        "addr": {
          "connections": []
        }
      },
      "position": [700, 200]
    }
  },
  "comments": [],
  "network": "bitcoin"
}
```

## 10. Implementation Checklist

- [ ] Use `"demo@0.1.0"` as graph ID
- [ ] Include `"comments": []`
- [ ] Optionally include `"network": "bitcoin"`
- [ ] Node IDs are integers, keys are stringified integers
- [ ] All connections have both input and output entries
- [ ] Socket names match exactly (case-sensitive)
- [ ] Empty sockets still have `"connections": []`
- [ ] Threshold uses single `policies` socket for all children
- [ ] Key nodes without BIP39 have `data.key`, Key nodes with BIP39 have `data: {}`
- [ ] Map miniscript wrappers to underlying policy nodes
