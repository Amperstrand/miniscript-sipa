# Expert Prompt: miniscript.fun JSON Format Analysis

## Context

I am building a miniscript compiler that outputs Rete.js JSON compatible with [miniscript.fun](https://miniscript.fun). The goal is to generate JSON that can be imported into miniscript.fun's visual editor and display correctly with all node connections visible.

## Current Understanding

Based on previous analysis, we've identified these socket patterns:

| Component | Input Sockets | Output Socket |
|-----------|---------------|---------------|
| Key | `key` | `key` |
| BIP39 | none | `key` |
| Descriptor | `pol` | `desc` |
| Address | `desc` | `addr` |
| And | `pol1`, `pol2` | `pol` |
| Or | `pol1`, `pol2` | `pol` |
| AndOr | `cond`, `pol1`, `pol2` | `pol` |
| Threshold | `policies` (multiple) | `pol` |
| Multi | `policies` (multiple) | `pol` |
| Older | none | `pol` |
| After | none | `pol` |
| SHA256 | none | `pol` |
| Hash160 | none | `pol` |
| Hash256 | none | `pol` |
| Ripemd160 | none | `pol` |

## Questions for Expert

1. **Hash Preimage Nodes**: Do SHA256, Hash160, Hash256, and Ripemd160 have any input sockets (e.g., for connecting a preimage value), or are they purely leaf nodes with only a `hash` data field?

2. **Multi Node**: Does the Multi component work exactly like Threshold, using a single `policies` socket for all key connections? Or does it have a different pattern for key inputs?

3. **Wrapper Nodes**: Does miniscript.fun visualize wrapper operations (a:, s:, c:, d:, v:, j:, n:) as separate nodes, or are they applied as modifiers to their child nodes?

4. **Just_0 and Just_1**: How are the literal 0 and 1 values represented in miniscript.fun? Are they nodes? What sockets do they have?

5. **pk_k vs pk_h**: Are these both represented as "Key" components, or is there a separate component for pk_h (pubkey hash)?

6. **Data Fields**: Can you provide the complete list of `data` fields for each component type? For example:
   - Key: `{ key: string }`
   - Threshold: `{ thresh: number }`
   - After/Older: `{ num: number }`
   - Hash nodes: `{ hash: string }`
   - Any others?

7. **Position Fields**: Are there required constraints on `position` array values? We're using `[x, y]` with integers. Are there minimum/maximum values or spacing requirements?

8. **Connection Format**: Can you confirm the complete connection format? We're using:
   ```json
   {
     "node": <integer node id>,
     "input": "<socket name>",  // or "output" for inputs
     "data": {}
   }
   ```

9. **Optional Components**: Are there any other components in miniscript.fun that we might be missing? For example:
   - SortedMulti?
   - Taproot-specific components?
   - Raw script components?

10. **Schema Version**: Is there a version field or schema identifier that should be included in the JSON for future compatibility?

## Example Output We Generate

For `and_v(pk(A),pk(B))`:

```json
{
  "id": "demo@0.1.0",
  "nodes": {
    "1": {
      "id": 1,
      "name": "Key",
      "data": { "key": "A" },
      "inputs": {},
      "outputs": {
        "key": {
          "connections": [
            { "node": 3, "input": "pol1", "data": {} }
          ]
        }
      },
      "position": [0, 0]
    },
    "2": {
      "id": 2,
      "name": "Key", 
      "data": { "key": "B" },
      "inputs": {},
      "outputs": {
        "key": {
          "connections": [
            { "node": 3, "input": "pol2", "data": {} }
          ]
        }
      },
      "position": [0, 120]
    },
    "3": {
      "id": 3,
      "name": "And",
      "data": {},
      "inputs": {
        "pol1": {
          "connections": [
            { "node": 1, "output": "key", "data": {} }
          ]
        },
        "pol2": {
          "connections": [
            { "node": 2, "output": "key", "data": {} }
          ]
        }
      },
      "outputs": {
        "pol": {
          "connections": []
        }
      },
      "position": [250, 60]
    }
  }
}
```

## Verification Method

The best way to verify is to:
1. Open miniscript.fun
2. Build a similar graph visually  
3. Export the JSON using the export feature
4. Compare socket names and connection formats

If you can provide sample exports from miniscript.fun for these test cases, that would be extremely helpful:
- `thresh(2,pk(A),pk(B),pk(C))` - threshold with multiple keys
- `multi(2,A,B,C)` - multi signature
- `or_d(pk(A),and_v(pk(B),older(100)))` - nested structure with timelock
- Complete graph with BIP39 → Key → Descriptor → Address chain

## Thank You

This analysis will help ensure our compiler generates correct JSON that works seamlessly with miniscript.fun's visual editor.
