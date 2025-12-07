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

#endif // _MINISCRIPT_JSON_H_
