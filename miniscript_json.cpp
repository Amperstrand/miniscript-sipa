// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miniscript_json.h"
#include "compiler.h"

#include <script/miniscript.h>
#include <sstream>
#include <iomanip>

namespace {

using miniscript::operator"" _mst;
using miniscript::Fragment;
using miniscript::NodeRef;
using miniscript::Type;

/**
 * Simple JSON string escaper.
 * Escapes special characters for JSON string values.
 */
std::string EscapeJsonString(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

/**
 * Get the fragment name as a string.
 * Maps the Fragment enum to human-readable fragment names.
 */
std::string FragmentName(Fragment frag) {
    switch (frag) {
        case Fragment::JUST_0: return "0";
        case Fragment::JUST_1: return "1";
        case Fragment::PK_K: return "pk_k";
        case Fragment::PK_H: return "pk_h";
        case Fragment::OLDER: return "older";
        case Fragment::AFTER: return "after";
        case Fragment::SHA256: return "sha256";
        case Fragment::HASH256: return "hash256";
        case Fragment::RIPEMD160: return "ripemd160";
        case Fragment::HASH160: return "hash160";
        case Fragment::WRAP_A: return "a";
        case Fragment::WRAP_S: return "s";
        case Fragment::WRAP_C: return "c";
        case Fragment::WRAP_D: return "d";
        case Fragment::WRAP_V: return "v";
        case Fragment::WRAP_J: return "j";
        case Fragment::WRAP_N: return "n";
        case Fragment::AND_V: return "and_v";
        case Fragment::AND_B: return "and_b";
        case Fragment::OR_B: return "or_b";
        case Fragment::OR_C: return "or_c";
        case Fragment::OR_D: return "or_d";
        case Fragment::OR_I: return "or_i";
        case Fragment::ANDOR: return "andor";
        case Fragment::THRESH: return "thresh";
        case Fragment::MULTI: return "multi";
    }
    return "unknown";
}

/**
 * Check if a fragment is a wrapper type (a:, s:, c:, d:, v:, j:, n:).
 */
bool IsWrapper(Fragment frag) {
    switch (frag) {
        case Fragment::WRAP_A:
        case Fragment::WRAP_S:
        case Fragment::WRAP_C:
        case Fragment::WRAP_D:
        case Fragment::WRAP_V:
        case Fragment::WRAP_J:
        case Fragment::WRAP_N:
            return true;
        default:
            return false;
    }
}

/**
 * Get the basic type character (B, V, K, W) from the Type object.
 */
std::string GetBasicType(Type t) {
    if (t << "B"_mst) return "B";
    if (t << "V"_mst) return "V";
    if (t << "K"_mst) return "K";
    if (t << "W"_mst) return "W";
    return "";
}

/**
 * Get the modifier flags as a JSON array string.
 */
std::string GetModifiersJson(Type t) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    auto add = [&](const char* m) {
        if (!first) oss << ",";
        oss << "\"" << m << "\"";
        first = false;
    };
    if (t << "z"_mst) add("z");
    if (t << "o"_mst) add("o");
    if (t << "n"_mst) add("n");
    if (t << "d"_mst) add("d");
    if (t << "u"_mst) add("u");
    if (t << "e"_mst) add("e");
    if (t << "f"_mst) add("f");
    if (t << "s"_mst) add("s");
    if (t << "m"_mst) add("m");
    if (t << "x"_mst) add("x");
    if (t << "k"_mst) add("k");
    oss << "]";
    return oss.str();
}

/**
 * State passed down during tree traversal.
 * Tracks the current node ID counter.
 */
struct JsonState {
    int next_id;
};

/**
 * Result returned during tree traversal.
 * Contains the JSON representation of a node.
 */
struct JsonResult {
    std::string json;
};

/**
 * Recursively build JSON for a miniscript node.
 * This function uses the TreeEval algorithm from the miniscript library.
 * 
 * @param node The miniscript node to convert
 * @return A JSON string representing this node and all its children
 */
std::string NodeToJson(const NodeRef<std::string>& node) {
    // Use the non-stateful TreeEval approach with an upward function.
    // We need to track node IDs, so we'll use a counter that increments.
    // Since TreeEval doesn't easily support stateful ID generation across nodes,
    // we'll use a simpler recursive approach here.
    
    int id_counter = 0;
    
    // Define a recursive lambda for tree walking
    std::function<std::string(const NodeRef<std::string>&, int&)> walk;
    walk = [&walk](const NodeRef<std::string>& n, int& id_counter) -> std::string {
        std::ostringstream oss;
        int my_id = id_counter++;
        
        oss << "{";
        
        // id
        oss << "\"id\":\"n" << my_id << "\"";
        
        // fragment
        oss << ",\"fragment\":\"" << FragmentName(n->fragment) << "\"";
        
        // miniscript string for this node
        auto ms_str = n->ToString(COMPILER_CTX);
        if (ms_str) {
            oss << ",\"miniscript\":\"" << EscapeJsonString(*ms_str) << "\"";
        } else {
            oss << ",\"miniscript\":null";
        }
        
        // args - scalar arguments (keys, hashes, numbers)
        oss << ",\"args\":[";
        bool first_arg = true;
        
        // Add keys as args
        for (const auto& key : n->keys) {
            if (!first_arg) oss << ",";
            auto key_str = COMPILER_CTX.ToString(key);
            if (key_str) {
                oss << "\"" << EscapeJsonString(*key_str) << "\"";
            } else {
                oss << "null";
            }
            first_arg = false;
        }
        
        // Add k value for OLDER, AFTER, MULTI, THRESH
        if (n->fragment == Fragment::OLDER || n->fragment == Fragment::AFTER ||
            n->fragment == Fragment::MULTI || n->fragment == Fragment::THRESH) {
            if (!first_arg) oss << ",";
            oss << n->k;
            first_arg = false;
        }
        
        // Add data (hashes) for hash functions
        if (!n->data.empty()) {
            if (!first_arg) oss << ",";
            oss << "\"" << HexStr(n->data) << "\"";
            first_arg = false;
        }
        
        oss << "]";
        
        // wrappers - if this is a wrapper, note it; collect wrapper chain
        // For simplicity, we just note if this node itself is a wrapper
        if (IsWrapper(n->fragment)) {
            oss << ",\"is_wrapper\":true";
        }
        
        // props - type properties
        oss << ",\"props\":{";
        Type t = n->GetType();
        oss << "\"type\":\"" << GetBasicType(t) << "\"";
        oss << ",\"modifiers\":" << GetModifiersJson(t);
        oss << ",\"is_valid\":" << (n->IsValid() ? "true" : "false");
        oss << ",\"is_nonmalleable\":" << (n->IsNonMalleable() ? "true" : "false");
        oss << ",\"needs_signature\":" << (n->NeedsSignature() ? "true" : "false");
        oss << ",\"timelock_mix_ok\":" << (n->CheckTimeLocksMix() ? "true" : "false");
        oss << ",\"script_size\":" << n->ScriptSize();
        oss << ",\"max_ops\":" << n->GetOps();
        oss << ",\"max_stack_size\":" << n->GetStackSize();
        oss << "}";
        
        // children - sub-expressions
        oss << ",\"children\":[";
        for (size_t i = 0; i < n->subs.size(); ++i) {
            if (i > 0) oss << ",";
            oss << walk(n->subs[i], id_counter);
        }
        oss << "]";
        
        oss << "}";
        return oss.str();
    };
    
    return walk(node, id_counter);
}

} // anonymous namespace

std::string MiniscriptToJson(const std::string& miniscript_src) {
    std::ostringstream result;
    result << "{";
    result << "\"miniscript\":\"" << EscapeJsonString(miniscript_src) << "\"";
    
    try {
        // Trim whitespace from input
        std::string str = miniscript_src;
        if (!str.empty()) {
            size_t end = str.find_last_not_of(" \n\r\t");
            if (end != std::string::npos) {
                str = str.substr(0, end + 1);
            }
        }
        
        // Expand abbreviations (pk -> c:pk_k, pkh -> c:pk_h, etc.)
        std::string expanded = Expand(str);
        
        // Parse the miniscript using the existing parser
        NodeRef<std::string> parsed = miniscript::FromString(expanded, COMPILER_CTX);
        
        if (!parsed) {
            result << ",\"valid\":false";
            result << ",\"error\":\"Failed to parse miniscript expression\"";
            result << "}";
            return result.str();
        }
        
        if (!parsed->IsValid()) {
            result << ",\"valid\":false";
            result << ",\"error\":\"Parsed miniscript is not valid\"";
            result << ",\"root\":" << NodeToJson(parsed);
            result << "}";
            return result.str();
        }
        
        // Successfully parsed and valid
        result << ",\"valid\":true";
        result << ",\"is_valid_top_level\":" << (parsed->IsValidTopLevel() ? "true" : "false");
        result << ",\"is_sane\":" << (parsed->IsSane() ? "true" : "false");
        result << ",\"root\":" << NodeToJson(parsed);
        result << "}";
        
    } catch (const std::exception& e) {
        result << ",\"valid\":false";
        result << ",\"error\":\"Exception: " << EscapeJsonString(e.what()) << "\"";
        result << "}";
    }
    
    return result.str();
}
