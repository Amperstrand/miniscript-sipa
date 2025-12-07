// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _MINISCRIPT_JSON_H_
#define _MINISCRIPT_JSON_H_

/**
 * Miniscript to JSON/AST Converter
 * ================================
 * 
 * This module provides functionality to convert a parsed Miniscript expression
 * into a structured JSON representation (AST - Abstract Syntax Tree).
 * 
 * Usage from C++:
 *     std::string json = MiniscriptToJson("and_v(pk(A),after(100))");
 * 
 * Usage from JavaScript (via Emscripten):
 *     const jsonStr = Module.miniscript_to_json("and_v(pk(A),after(100))");
 *     const ast = JSON.parse(jsonStr);
 * 
 * JSON Schema (approximate):
 * {
 *   "miniscript": "<original input string>",
 *   "valid": true|false,
 *   "error": "<error message if not valid>",
 *   "root": {
 *     "id": "n0",
 *     "fragment": "<fragment name: pk_k, and_v, or_c, etc.>",
 *     "miniscript": "<this node's miniscript representation>",
 *     "args": [<scalar arguments like keys, hashes, numbers>],
 *     "wrappers": ["a", "s", "c", ...],  // if this is a wrapper chain
 *     "props": {
 *       "type": "B"|"V"|"K"|"W",
 *       "modifiers": ["z", "o", "n", "d", "u", "e", "f", "s", "m", "x", "k"],
 *       "is_valid": true|false,
 *       "is_nonmalleable": true|false,
 *       "needs_signature": true|false,
 *       "timelock_mix_ok": true|false,
 *       "script_size": <number>,
 *       "max_ops": <number>,
 *       "max_stack_size": <number>
 *     },
 *     "children": [<child nodes>]
 *   }
 * }
 */

#include <string>

/**
 * Convert a Miniscript string to a JSON AST representation.
 * 
 * @param miniscript_src The miniscript expression to parse and convert
 * @return A JSON string representing the AST, or an error JSON if parsing fails
 * 
 * On success, returns a JSON object with:
 *   - "miniscript": the original input
 *   - "valid": true
 *   - "root": the AST root node
 * 
 * On failure, returns a JSON object with:
 *   - "miniscript": the original input  
 *   - "valid": false
 *   - "error": description of the error
 */
std::string MiniscriptToJson(const std::string& miniscript_src);

/**
 * Convert a Miniscript string to Rete.js-compatible graph JSON.
 * 
 * This function produces JSON that can be loaded directly into miniscript.fun
 * (or any Rete.js-based editor) via `editor.fromJSON()`.
 * 
 * @param miniscript_src The miniscript expression to parse and convert
 * @return A Rete.js-compatible JSON string, or an error JSON if parsing fails
 * 
 * The output follows the Rete.js graph format with:
 *   - "id": graph identifier ("miniscript-sipa@rete-0.1.0")
 *   - "nodes": object mapping node IDs to node objects
 *   - "groups": empty object (for future use)
 *   - "comments": empty array (for future use)
 *   - "__ms_source": the original miniscript input
 * 
 * Each node contains:
 *   - "id": numeric node ID
 *   - "name": Rete component name (Key, And, Or, Threshold, etc.)
 *   - "data": node-specific data (num, thresh, key, hash) + metadata
 *   - "inputs": input socket connections
 *   - "outputs": output socket connections
 *   - "position": [x, y] coordinates for layout
 * 
 * Component mapping from Miniscript fragments:
 *   - pk_k, pk_h → "Key"
 *   - after → "After"
 *   - older → "Older"
 *   - and_v, and_b → "And"
 *   - or_b, or_c, or_d, or_i → "Or"
 *   - andor → "AndOr"
 *   - thresh → "Threshold"
 *   - multi → "Multi"
 *   - sha256, hash256, ripemd160, hash160 → "SHA256", "Hash256", etc.
 * 
 * On failure, returns a JSON object with:
 *   - "error": description of the error
 *   - "valid": false
 */
std::string MiniscriptToReteJson(const std::string& miniscript_src);

/**
 * Convert a miniscript expression to a complete Rete.js-compatible JSON graph.
 * 
 * Similar to MiniscriptToReteJson, but adds additional nodes for use in
 * miniscript.fun:
 *   - BIP39 nodes: One for each Key node, providing dummy seed phrases
 *     ("bacon" repeated 24 times, with password "N" for Nth key)
 *   - Descriptor node: Connected to the root policy node
 *   - Address node: Connected to the Descriptor, showing derived address
 * 
 * This produces a fully functional graph that can generate addresses in
 * miniscript.fun.
 */
std::string MiniscriptToCompleteReteJson(const std::string& miniscript_src);

#endif // _MINISCRIPT_JSON_H_
