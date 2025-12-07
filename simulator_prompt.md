This repo is a fork of sipa/miniscript with extra JSON exports added. Your job is to add a new simulator.html file at the root of this repo (next to index.html) that acts as a Miniscript Visualizer & Simulator, using the existing miniscript WASM bindings from this project.

High-level goal

Create a standalone HTML page (simulator.html) that:

Lets me input a Miniscript string and optional parameter mapping for keys/labels.

Calls the existing WASM/JS binding that converts Miniscript → JSON AST
(something like miniscript_to_json — you should find the exact export name in js_bindings.cpp or related JS glue in this branch).

Converts that JSON AST into an internal Visualization AST suitable for:

Rendering a tree view of the policy.

Running a spending simulator based on:

Which keys I have signatures for.

Current block height.

Confirmation height (for older(...) relative timelocks).

Available hash preimages (for hashlocks).

Displays:

Left: Tree view (policy structure).

Right: Details / Simulator tabs.

Status of whether the script is currently spendable under chosen conditions.

We do not want to implement our own Miniscript compiler or parser anymore — use the C++/WASM miniscript implementation on this branch as the single source of truth.

Repo context (what you should inspect)

Please first inspect the existing files in this branch (read them before coding):

index.html

style.css

Any JS glue that loads the WASM (miniscript.js, main.js, or similar).

js_bindings.cpp (or whatever file defines the Emscripten bindings).

Any new JSON-related functions (likely miniscript_to_json, miniscript_to_rete_json, etc.).

I know the JSON AST output on this branch looks like this (example):

{
  "miniscript": "and_v(v:pk(key_user),or_d(pk(key_service),older(12960)))",
  "valid": true,
  "is_valid_top_level": true,
  "is_sane": true,
  "root": {
    "id": "n0",
    "fragment": "and_v",
    "miniscript": "and_v(v:pk(key_user),or_d(pk(key_service),older(12960)))",
    "args": [],
    "props": {
      "type": "B",
      "modifiers": ["n","f","s","m","x","k"],
      "is_valid": true,
      "is_nonmalleable": true,
      "needs_signature": true,
      "timelock_mix_ok": true,
      "script_size": 77,
      "max_ops": 6,
      "max_stack_size": 3
    },
    "children": [
      {
        "id": "n1",
        "fragment": "v",
        "miniscript": "v:pk(key_user)",
        "args": [],
        "is_wrapper": true,
        "props": {...},
        "children": [
          {
            "id": "n2",
            "fragment": "c",
            "miniscript": "pk(key_user)",
            "args": [],
            "is_wrapper": true,
            "props": {...},
            "children": [
              {
                "id": "n3",
                "fragment": "pk_k",
                "miniscript": "pk_k(key_user)",
                "args": ["key_user"],
                "props": {...},
                "children": []
              }
            ]
          }
        ]
      },
      {
        "id": "n4",
        "fragment": "or_d",
        "miniscript": "or_d(pk(key_service),older(12960))",
        "args": [],
        "props": {...},
        "children": [
          {
            "id": "n5",
            "fragment": "c",
            "miniscript": "pk(key_service)",
            "args": [],
            "is_wrapper": true,
            "props": {...},
            "children": [
              {
                "id": "n6",
                "fragment": "pk_k",
                "miniscript": "pk_k(key_service)",
                "args": ["key_service"],
                "props": {...},
                "children": []
              }
            ]
          },
          {
            "id": "n7",
            "fragment": "older",
            "miniscript": "older(12960)",
            "args": [12960],
            "props": {...},
            "children": []
          }
        ]
      }
    ]
  }
}


Every node has:

id: "n0", "n1", …

fragment: Miniscript fragment id like "and_v", "or_d", "pk_k", "older", etc.

miniscript: Miniscript string for that subtree (including wrappers).

args: array of scalar args (keys, numbers, hashes).

is_wrapper: optional, true for wrapper fragments (v, c, d, etc.).

props: miniscript type data (we’ll show some of this in the details panel).

children: array of child nodes.

We want to build our own Visualization AST on top of this structure.

Visualization AST we want inside simulator.html

Inside simulator.html, create a JS data model like:

const NodeType = {
  AND: "and",
  OR: "or",
  THRESH: "thresh",
  ANDOR: "andor",
  SIGNATURE: "signature",
  MULTISIG: "multisig",
  TIMELOCK_ABSOLUTE: "timelock_absolute",
  TIMELOCK_RELATIVE: "timelock_relative",
  HASHLOCK: "hashlock",
  CONSTANT_TRUE: "constant_true",
  CONSTANT_FALSE: "constant_false",
};

interface VisualizationNode {
  id: string;
  type: string; // one of NodeType
  label: string | null;
  miniscript: string | null;
  children: string[];
  params: Record<string, any>;

  // Simulator UI state
  satisfied?: boolean | null;
  relevant?: boolean | null;
  satisfactionPath?: string | null;
}

interface VisualizationTree {
  nodesById: Record<string, VisualizationNode>;
  rootId: string;
}

Mapping from JSON AST → Visualization AST

Create a function in simulator.html:

function buildVisualizationTree(jsonAst, params, keyLabels) { ... }


jsonAst is the parsed result of miniscript_to_json(...) (the structure above).

params is a user-provided map like { "Alice": "02abc...", "Bob": "03def..." } (names → pubkey hex).

keyLabels is optional map { "02abc...": "Alice (Party A)" } for human-readable labels.

In buildVisualizationTree:

Start from jsonAst.root.

Implement a recursive visit(astNode):

Flatten wrappers:

If astNode.is_wrapper === true and astNode.children[0] exists, walk down until you reach a non-wrapper inner node.

Keep track of the wrapper fragments in an array like wrappers = ["v", "c"] if you go through v then c.

Then look at inner.fragment and map like this:

pk_k, pk_h → NodeType.SIGNATURE

const keyRef = inner.args[0] (e.g. "key_user").

Resolve pubkey = params[keyRef] ?? keyRef;

Resolve label = keyLabels[pubkey] ?? keyRef;

params for the viz node:

{ pubkey, pubkeyLabel: label, wrappers }


multi → NodeType.MULTISIG

inner.args[0] = k (number).

inner.args[1..] = key identifiers.

Resolve each key via params.

params:

{
  k: <number>,
  n: <number of keys>,
  keys: [pubkey1, pubkey2, ...],
  keyLabels: [label1, label2, ...],
}


after → NodeType.TIMELOCK_ABSOLUTE

inner.args[0] = block height.

params:

{ lockType: "absolute", value: height, unit: "height" }


older → NodeType.TIMELOCK_RELATIVE

inner.args[0] = number of blocks.

params:

{ lockType: "relative", value: blocks, unit: "blocks" }


Hashlocks: if there are fragments like hash160, sha256, hash256, etc.:

Map to NodeType.HASHLOCK with:

{
  hashType: inner.fragment, // "hash160" / "sha256" ...
  hash: inner.args[0],
}


Logical combinators:

and_v, and_b, maybe other and_* fragments → NodeType.AND

or_i, or_d, or_c, or_b, maybe other or_* → NodeType.OR

andor → NodeType.ANDOR with exactly 3 children (condition, then, else).

thresh:

inner.args[0] = k threshold.

The actual children are in inner.children (subexpressions).

NodeType.THRESH + params: { k }.

Constant true/false (if present in AST; you’ll see what fragments are used):

Map to NodeType.CONSTANT_TRUE or CONSTANT_FALSE.

Each visit creates a new VisualizationNode with:

id: generate your own (e.g. "node_0", "node_1", etc.); you don’t need to preserve "n0" etc.

type: from the mapping above.

label: human-readable:

"Signature: Alice (Party A)",

"Multisig: 2 of 3",

"After block 800000",

"OR", "AND", "Threshold: 2 of 3", etc.

miniscript: use astNode.miniscript from the wrapper root, so the label includes wrappers (e.g. "v:pk(key_user)").

children: array of child viz-node IDs, created by recursing on astNode.children (respecting wrappers).

params: as described above.

Return:

return { nodesById, rootId };


Also build helper arrays:

knownPubkeys: list of { pubkey, label } discovered from SIGNATURE / MULTISIG nodes (for simulator checkboxes).

hashlocks: list of hashes from HASHLOCK nodes.

You can store these alongside the tree in a currentTree object in JS.

Simulator logic (JS, inside simulator.html)

Implement the simulator using these functions (you can basically copy this and adjust naming):

function evaluateSatisfaction(node, nodesById, simulatorState) {
  const {
    availableSignatures,  // Set<string> of pubkey hex
    currentHeight,        // number
    confirmationHeight,   // number | null
    availablePreimages,   // { [hash: string]: string }
  } = simulatorState;

  node.satisfactionPath = null;

  switch (node.type) {
    case NodeType.AND: {
      let all = true;
      for (const childId of node.children) {
        const child = nodesById[childId];
        if (!evaluateSatisfaction(child, nodesById, simulatorState)) {
          all = false;
        }
      }
      node.satisfied = all;
      return node.satisfied;
    }

    case NodeType.OR: {
      for (const childId of node.children) {
        const child = nodesById[childId];
        if (evaluateSatisfaction(child, nodesById, simulatorState)) {
          node.satisfied = true;
          node.satisfactionPath = childId;
          return true;
        }
      }
      node.satisfied = false;
      return false;
    }

    case NodeType.THRESH: {
      const k = node.params.k;
      let count = 0;
      for (const childId of node.children) {
        const child = nodesById[childId];
        if (evaluateSatisfaction(child, nodesById, simulatorState)) {
          count++;
        }
      }
      node.satisfied = count >= k;
      return node.satisfied;
    }

    case NodeType.ANDOR: {
      const [condId, thenId, elseId] = node.children;
      const cond = nodesById[condId];
      const thenNode = nodesById[thenId];
      const elseNode = nodesById[elseId];

      const condSatisfied = evaluateSatisfaction(cond, nodesById, simulatorState);
      if (condSatisfied) {
        const thenSatisfied = evaluateSatisfaction(thenNode, nodesById, simulatorState);
        node.satisfied = condSatisfied && thenSatisfied;
        node.satisfactionPath = thenSatisfied ? thenId : null;
      } else {
        const elseSatisfied = evaluateSatisfaction(elseNode, nodesById, simulatorState);
        node.satisfied = !condSatisfied && elseSatisfied;
        node.satisfactionPath = elseSatisfied ? elseId : null;
      }
      return node.satisfied;
    }

    case NodeType.SIGNATURE: {
      const pubkey = node.params.pubkey;
      node.satisfied = availableSignatures.has(pubkey);
      return node.satisfied;
    }

    case NodeType.MULTISIG: {
      const requiredK = node.params.k;
      const keys = node.params.keys || [];
      let availableCount = 0;
      for (const k of keys) {
        if (availableSignatures.has(k)) availableCount++;
      }
      node.satisfied = availableCount >= requiredK;
      return node.satisfied;
    }

    case NodeType.TIMELOCK_ABSOLUTE: {
      const locktime = node.params.value;
      node.satisfied =
        typeof currentHeight === "number" &&
        !Number.isNaN(currentHeight) &&
        currentHeight >= locktime;
      return node.satisfied;
    }

    case NodeType.TIMELOCK_RELATIVE: {
      const blocks = node.params.value;
      if (
        confirmationHeight === null ||
        confirmationHeight === undefined ||
        Number.isNaN(confirmationHeight)
      ) {
        node.satisfied = false;
      } else {
        node.satisfied = currentHeight - confirmationHeight >= blocks;
      }
      return node.satisfied;
    }

    case NodeType.HASHLOCK: {
      const hash = node.params.hash;
      node.satisfied =
        !!availablePreimages &&
        typeof availablePreimages[hash] === "string" &&
        availablePreimages[hash].length > 0;
      return node.satisfied;
    }

    case NodeType.CONSTANT_TRUE:
      node.satisfied = true;
      return true;

    case NodeType.CONSTANT_FALSE:
      node.satisfied = false;
      return false;

    default:
      node.satisfied = false;
      return false;
  }
}

function resetSatisfaction(nodesById) {
  for (const id in nodesById) {
    const node = nodesById[id];
    node.satisfied = null;
    node.relevant = null;
    node.satisfactionPath = null;
  }
}

function markRelevanceSatisfied(node, nodesById) {
  node.relevant = true;

  if (node.type === NodeType.OR && node.satisfactionPath) {
    for (const childId of node.children) {
      const child = nodesById[childId];
      if (!child) continue;
      if (childId === node.satisfactionPath) {
        markRelevanceSatisfied(child, nodesById);
      } else {
        child.relevant = false;
      }
    }
  } else if (node.type === NodeType.AND) {
    for (const childId of node.children) {
      const child = nodesById[childId];
      if (!child) continue;
      markRelevanceSatisfied(child, nodesById);
    }
  } else if (node.type === NodeType.THRESH) {
    for (const childId of node.children) {
      const child = nodesById[childId];
      if (!child) continue;
      if (child.satisfied) {
        markRelevanceSatisfied(child, nodesById);
      } else {
        child.relevant = false;
      }
    }
  } else if (node.type === NodeType.ANDOR) {
    const [condId, thenId, elseId] = node.children;
    const cond = nodesById[condId];
    const thenNode = nodesById[thenId];
    const elseNode = nodesById[elseId];
    if (cond && cond.satisfied && thenNode && thenNode.satisfied) {
      markRelevanceSatisfied(cond, nodesById);
      markRelevanceSatisfied(thenNode, nodesById);
      if (elseNode) elseNode.relevant = false;
    } else if (!cond.satisfied && elseNode && elseNode.satisfied) {
      cond.relevant = true;
      markRelevanceSatisfied(elseNode, nodesById);
      if (thenNode) thenNode.relevant = false;
    }
  }
}


Also implement a simple describeSatisfactionPath(rootNode, nodesById) that returns a human-readable summary (optional but nice).

UI layout for simulator.html

Follow the general look & feel of index.html / style.css in this repo. It does not have to be pixel perfect, but implement this approximate layout:

A top section with:

A Miniscript input (<textarea>).

A JSON config input (<textarea>) for:

{
  "params": {
    "Alice": "02abc123...",
    "Bob": "03def456..."
  },
  "keyLabels": {
    "02abc123...": "Alice (Party A)",
    "03def456...": "Bob (Party B)"
  }
}


A “Load policy” button:

Calls the WASM miniscript_to_json binding with the Miniscript string.

Parses the JSON result.

Calls buildVisualizationTree.

Renders the tree and simulator inputs.

Two-column main layout:

Left: Policy Tree

Render the tree using nested <details>/<summary> or simple nested <div> with indent.

Each node row shows:

Icon based on type:

🔑 signature

⏰ timelock

🔢 multisig

🔒 hashlock

📦 combinators

Node label.

Color coding based on simulation state:

Green: satisfied & relevant (node.satisfied === true && node.relevant === true).

Red: required but not satisfied (node.relevant === true && node.satisfied === false).

Gray: not relevant (node.relevant === false).

Default styling when no simulation has run.

Clicking a node should select it and populate the Details tab.

Right: Tabbed panel

Tabs:

Details

Simulator

Details tab:

Shows for selected node:

Type.

Label.

Miniscript fragment.

params pretty-printed as JSON.

Plain-language explanation (see next section).

Simulator tab:

Inputs:

Current block height (number).

Confirmation height (number, optional).

Signature checkboxes for each discovered pubkey:

Label as pubkeyLabel (pubkeyHex).

Hash preimage inputs:

For each hashlock, an input where any non-empty value counts as “preimage available”.

A “Run simulation” button that:

Calls resetSatisfaction.

Runs evaluateSatisfaction starting from root.

If root is satisfied:

Call markRelevanceSatisfied on root.

Update a status element with:

✅ Spendable.

Optional description of the spend path.

If root is not satisfied:

Status shows ❌ Not yet spendable.

After simulation, re-render the tree with colors.

Reuse existing CSS from style.css where convenient, but you can inline some styling in simulator.html if needed.

Plain-language explanations

Implement a simple function to explain each node in plain language in the Details tab, something like:

function explainNode(node) {
  switch (node.type) {
    case NodeType.SIGNATURE:
      return `Requires a valid signature from ${node.params.pubkeyLabel || "the specified public key"}.`;
    case NodeType.MULTISIG:
      return `Requires at least ${node.params.k} signatures out of ${node.params.n} possible keys.`;
    case NodeType.TIMELOCK_ABSOLUTE:
      return `Can only be spent once the blockchain has reached block height ${node.params.value} or higher.`;
    case NodeType.TIMELOCK_RELATIVE:
      return `Can only be spent after at least ${node.params.value} blocks have elapsed since confirmation.`;
    case NodeType.HASHLOCK:
      return `Requires revealing a secret whose ${node.params.hashType || "hash"} matches the stored hash.`;
    case NodeType.AND:
      return "All child conditions in this AND node must be satisfied.";
    case NodeType.OR:
      return "At least one of the child conditions in this OR node must be satisfied.";
    case NodeType.THRESH:
      return `At least ${node.params.k} of this node's children must be satisfied.`;
    case NodeType.ANDOR:
      return "If the first condition holds, the 'then' branch must be satisfied; otherwise the 'else' branch must be satisfied.";
    case NodeType.CONSTANT_TRUE:
      return "This condition is always true.";
    case NodeType.CONSTANT_FALSE:
      return "This condition is always false and blocks spending along this branch.";
    default:
      return "Part of the Miniscript policy. Its children and parameters determine how it contributes to spending rules.";
  }
}

Integration with existing WASM / JS

Very important: do not add a new Miniscript parser/compiler in JS. Use the existing C++/WASM implementation that this repo already uses.

In simulator.html:

Include the same JS/WASM loader as index.html uses. For example (adapt to whatever is actually in the repo):

<script src="miniscript.js"></script>
<script>
  // wait for Module to be ready if needed
</script>


Find the name/signature of the JSON export in the JS/WASM glue, e.g. in js_bindings.cpp you might see something like:

EM_JS(char*, miniscript_to_json, (const char* ms), {
    // ...
});


or equivalent. Mirror that from JS with Module.cwrap if necessary:

const miniscriptToJson = Module.cwrap('miniscript_to_json', 'string', ['string']);


When the user clicks “Load policy”:

function loadPolicy() {
  const miniscriptStr = miniscriptTextarea.value.trim();
  if (!miniscriptStr) { /* show error */ return; }

  const configStr = configTextarea.value.trim();
  let params = {}, keyLabels = {};
  if (configStr) {
    try {
      const parsed = JSON.parse(configStr);
      params = parsed.params || {};
      keyLabels = parsed.keyLabels || {};
    } catch (e) {
      // show JSON parse error
      return;
    }
  }

  const jsonStr = miniscriptToJson(miniscriptStr);  // from WASM
  const ast = JSON.parse(jsonStr);

  const tree = buildVisualizationTree(ast, params, keyLabels);
  // also build knownPubkeys/hashlocks from tree
  currentTree = tree;
  // render tree & simulator inputs
}


If the export name or signature is slightly different in this branch, adapt accordingly — the important part is: call into the Miniscript WASM to get the AST JSON, then build your own Visualization AST from that.

What I want from you, concretely

Create a new file simulator.html at the repo root.

Make it fully functional using the existing WASM JS bindings on this branch:

Load miniscript → call miniscript_to_json (or whatever it’s called here).

Build visualization tree.

Render tree UI.

Provide simulator UI (block heights, signatures, hash preimages).

Run the evaluation and color the tree.

Reuse styling from style.css where appropriate. It doesn’t have to be perfect, but should look reasonably clean and consistent with the project.

Test with sample Miniscript expressions, for example:

or_i(pk(Alice), pk(Bob))

and_v(after(800000), pk(Alice))

multi(2, Alice, Bob, Carol)

or_i(pk(Alice), and_v(after(800000), multi(2, Bob, Carol)))

thresh(2, pk(Alice), pk(Bob), pk(Carol))

When you’re done, show me the full contents of simulator.html and briefly explain:

How to open it in the browser.

How it hooks into the existing WASM.

How the simulator logic maps to Miniscript fragments.

Please now inspect the repo, locate the JSON export function in the JS/WASM bindings, and then implement simulator.html as described above.
