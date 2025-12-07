// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miniscript_json.h"
#include "compiler.h"

#include <script/miniscript.h>
#include <sstream>
#include <iomanip>
#include <optional>
#include <map>
#include <functional>

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

/**
 * Map Miniscript Fragment to Rete.js component name for miniscript.fun.
 * Returns empty string for fragments that don't have a direct Rete component.
 */
std::string FragmentToReteComponent(Fragment frag) {
    switch (frag) {
        case Fragment::PK_K:
        case Fragment::PK_H:
            return "Key";
        case Fragment::AFTER:
            return "After";
        case Fragment::OLDER:
            return "Older";
        case Fragment::SHA256:
            return "SHA256";
        case Fragment::HASH256:
            return "Hash256";
        case Fragment::RIPEMD160:
            return "Ripemd160";
        case Fragment::HASH160:
            return "Hash160";
        case Fragment::AND_V:
        case Fragment::AND_B:
            return "And";
        case Fragment::OR_B:
        case Fragment::OR_C:
        case Fragment::OR_D:
        case Fragment::OR_I:
            return "Or";
        case Fragment::THRESH:
            return "Threshold";
        case Fragment::MULTI:
            return "Multi";
        case Fragment::ANDOR:
            // ANDOR is decomposed into Or(And(cond,then),else) in BuildReteNodes
            // This should never be reached, but return empty to be safe
            return "";
        // Wrappers and literals don't have direct components
        case Fragment::WRAP_A:
        case Fragment::WRAP_S:
        case Fragment::WRAP_C:
        case Fragment::WRAP_D:
        case Fragment::WRAP_V:
        case Fragment::WRAP_J:
        case Fragment::WRAP_N:
        case Fragment::JUST_0:
        case Fragment::JUST_1:
            return "";
    }
    return "";
}

/**
 * Get the socket name for a child input based on the parent's fragment.
 * For And/Or: first child connects to "pol1", second to "pol2"
 * For Threshold: all children connect to "policies"
 */
std::string GetChildInputSocket(Fragment parent_frag, size_t child_index) {
    switch (parent_frag) {
        case Fragment::AND_V:
        case Fragment::AND_B:
        case Fragment::OR_B:
        case Fragment::OR_C:
        case Fragment::OR_D:
        case Fragment::OR_I:
            return child_index == 0 ? "pol1" : "pol2";
        case Fragment::ANDOR:
            // AndOr has 3 children: condition, if-true, if-false
            if (child_index == 0) return "cond";
            if (child_index == 1) return "pol1";
            return "pol2";
        case Fragment::THRESH:
            return "policies";
        default:
            return "pol";
    }
}

/**
 * Get the output socket name for a node based on its fragment.
 * Key nodes output "key", policy nodes output "pol"
 */
std::string GetOutputSocket(Fragment frag) {
    switch (frag) {
        case Fragment::PK_K:
        case Fragment::PK_H:
            return "key";
        default:
            return "pol";
    }
}

/**
 * Structure to hold a Rete node during graph construction.
 */
struct ReteNode {
    int id;
    std::string name;
    std::string fragment;       // Original miniscript fragment name
    std::string miniscript_str; // The miniscript string for this node
    
    // Data fields
    std::optional<int64_t> num;      // For After, Older
    std::optional<int64_t> thresh;   // For Threshold
    std::optional<std::string> key;  // For Key
    std::optional<std::string> hash; // For hash functions
    std::vector<std::string> wrappers; // Wrapper chain
    
    // BIP39 specific data fields
    std::optional<std::string> mnemonic;  // For BIP39 nodes
    std::optional<std::string> password;  // For BIP39 nodes
    
    // Address specific data fields
    std::optional<int64_t> idx;  // For Address nodes
    
    // Position
    int pos_x;
    int pos_y;
    
    // Connections (stored as vectors of {node_id, socket_name})
    std::vector<std::pair<int, std::string>> input_connections;  // Who connects to our inputs
    std::vector<std::pair<int, std::string>> output_connections; // Where we connect to
    
    // For And/Or nodes: track which child index each input came from (for pol1/pol2 assignment)
    std::vector<size_t> input_child_indices;
    
    std::string input_socket;  // Socket name for our inputs
    std::string output_socket; // Socket name for our outputs
};

/**
 * Context for Rete graph construction.
 */
struct ReteGraphContext {
    int next_id = 1;
    std::vector<ReteNode> nodes;
    int current_depth = 0;
    int max_depth = 0;
    std::map<int, int> nodes_at_depth; // Count of nodes at each depth
    std::map<int, int> current_y_at_depth; // Current Y position counter at each depth
};

// Layout constants
constexpr int LAYOUT_X_SPACING = 300;  // Horizontal spacing between depth levels
constexpr int LAYOUT_Y_SPACING = 180;  // Vertical spacing between nodes at same depth
constexpr int LAYOUT_BASE_X = 100;     // Starting X position (for leaves)
constexpr int LAYOUT_BASE_Y = 200;     // Starting Y position

/**
 * Calculate the depth of each node (distance from leaves).
 * Leaves have depth 0, their parents have depth 1, etc.
 * This gives us a left-to-right layout with leaves on the left.
 */
int CalculateNodeDepth(const NodeRef<std::string>& node) {
    // Skip wrappers
    if (IsWrapper(node->fragment)) {
        if (!node->subs.empty()) {
            return CalculateNodeDepth(node->subs[0]);
        }
        return 0;
    }
    
    // Leaf nodes (no children) have depth 0
    if (node->subs.empty()) {
        return 0;
    }
    
    // Parent nodes have depth = max(children depths) + 1
    int max_child_depth = 0;
    for (const auto& sub : node->subs) {
        int child_depth = CalculateNodeDepth(sub);
        max_child_depth = std::max(max_child_depth, child_depth);
    }
    return max_child_depth + 1;
}

/**
 * Recursively build Rete nodes from a Miniscript tree.
 * Skips wrapper nodes and collects wrapper chain.
 * 
 * ANDOR Decomposition:
 * miniscript.fun does NOT have an AndOr component. We decompose:
 *   andor(cond, then, else) → or(and(cond, then), else)
 * This creates an Or node with an And node as pol1 and else as pol2.
 */
int BuildReteNodes(const NodeRef<std::string>& node, ReteGraphContext& ctx,
                   std::vector<std::string>& wrapper_chain) {
    
    // Handle wrappers: collect them and recurse to the inner node
    if (IsWrapper(node->fragment)) {
        wrapper_chain.push_back(FragmentName(node->fragment));
        if (!node->subs.empty()) {
            return BuildReteNodes(node->subs[0], ctx, wrapper_chain);
        }
        return -1; // Invalid: wrapper with no child
    }
    
    // Special handling for ANDOR: decompose into Or(And(cond, then), else)
    // miniscript.fun does NOT have an AndOr component!
    if (node->fragment == Fragment::ANDOR && node->subs.size() == 3) {
        // First, recursively build all three children
        std::vector<std::string> cond_wrappers, then_wrappers, else_wrappers;
        int cond_id = BuildReteNodes(node->subs[0], ctx, cond_wrappers);
        int then_id = BuildReteNodes(node->subs[1], ctx, then_wrappers);
        int else_id = BuildReteNodes(node->subs[2], ctx, else_wrappers);
        
        // Create the inner And node: and(cond, then)
        ReteNode and_node;
        and_node.id = ctx.next_id++;
        and_node.name = "And";
        and_node.fragment = "and_synthetic";
        and_node.output_socket = "pol";
        
        // Position And node
        int and_depth = CalculateNodeDepth(node) - 1;
        and_node.pos_x = LAYOUT_BASE_X + and_depth * LAYOUT_X_SPACING;
        int and_y_index = ctx.current_y_at_depth[and_depth]++;
        and_node.pos_y = LAYOUT_BASE_Y + and_y_index * LAYOUT_Y_SPACING;
        
        int and_id = and_node.id;
        ctx.nodes.push_back(std::move(and_node));
        size_t and_index = ctx.nodes.size() - 1;
        
        // Connect cond → And.pol1
        if (cond_id > 0) {
            std::string cond_output = "pol";
            for (auto& cn : ctx.nodes) {
                if (cn.id == cond_id) {
                    cond_output = cn.output_socket;
                    cn.output_connections.push_back({and_id, "pol1"});
                    break;
                }
            }
            ctx.nodes[and_index].input_connections.push_back({cond_id, cond_output});
            ctx.nodes[and_index].input_child_indices.push_back(0); // cond is child 0
        }
        
        // Connect then → And.pol2
        if (then_id > 0) {
            std::string then_output = "pol";
            for (auto& cn : ctx.nodes) {
                if (cn.id == then_id) {
                    then_output = cn.output_socket;
                    cn.output_connections.push_back({and_id, "pol2"});
                    break;
                }
            }
            ctx.nodes[and_index].input_connections.push_back({then_id, then_output});
            ctx.nodes[and_index].input_child_indices.push_back(1); // then is child 1
        }
        
        // Create the outer Or node: or(And_result, else)
        ReteNode or_node;
        or_node.id = ctx.next_id++;
        or_node.name = "Or";
        or_node.fragment = "or_synthetic";
        or_node.output_socket = "pol";
        or_node.wrappers = wrapper_chain;
        
        // Position Or node (at original andor depth)
        int or_depth = CalculateNodeDepth(node);
        ctx.max_depth = std::max(ctx.max_depth, or_depth);
        or_node.pos_x = LAYOUT_BASE_X + or_depth * LAYOUT_X_SPACING;
        int or_y_index = ctx.current_y_at_depth[or_depth]++;
        or_node.pos_y = LAYOUT_BASE_Y + or_y_index * LAYOUT_Y_SPACING;
        
        int or_id = or_node.id;
        ctx.nodes.push_back(std::move(or_node));
        size_t or_index = ctx.nodes.size() - 1;
        
        // Connect And → Or.pol1
        ctx.nodes[and_index].output_connections.push_back({or_id, "pol1"});
        ctx.nodes[or_index].input_connections.push_back({and_id, "pol"});
        ctx.nodes[or_index].input_child_indices.push_back(0); // and is child 0 of or
        
        // Connect else → Or.pol2
        if (else_id > 0) {
            std::string else_output = "pol";
            for (auto& cn : ctx.nodes) {
                if (cn.id == else_id) {
                    else_output = cn.output_socket;
                    cn.output_connections.push_back({or_id, "pol2"});
                    break;
                }
            }
            ctx.nodes[or_index].input_connections.push_back({else_id, else_output});
            ctx.nodes[or_index].input_child_indices.push_back(1); // else is child 1 of or
        }
        
        // Return the Or node's ID as this is the output of the decomposed andor
        return or_id;
    }
    
    // Get the Rete component name
    std::string component_name = FragmentToReteComponent(node->fragment);
    
    // Skip nodes without Rete components (like JUST_0, JUST_1)
    if (component_name.empty()) {
        return -1;
    }
    
    // Special handling for And/Or nodes with JUST_0/JUST_1 children
    // These are typically from wrappers like t: which creates and_v(X, 1)
    // In this case, we skip the And node and just return the real child
    if ((node->fragment == Fragment::AND_V || node->fragment == Fragment::AND_B ||
         node->fragment == Fragment::OR_B || node->fragment == Fragment::OR_C ||
         node->fragment == Fragment::OR_D || node->fragment == Fragment::OR_I) &&
        node->subs.size() == 2) {
        
        bool child0_is_literal = (node->subs[0]->fragment == Fragment::JUST_0 || 
                                   node->subs[0]->fragment == Fragment::JUST_1);
        bool child1_is_literal = (node->subs[1]->fragment == Fragment::JUST_0 || 
                                   node->subs[1]->fragment == Fragment::JUST_1);
        
        // If one child is a literal, return the other child's node (skip this And/Or)
        if (child0_is_literal && !child1_is_literal) {
            return BuildReteNodes(node->subs[1], ctx, wrapper_chain);
        }
        if (child1_is_literal && !child0_is_literal) {
            return BuildReteNodes(node->subs[0], ctx, wrapper_chain);
        }
        // If both are literals, return -1 (shouldn't happen in practice)
        if (child0_is_literal && child1_is_literal) {
            return -1;
        }
    }
    
    // Create the Rete node
    ReteNode rnode;
    rnode.id = ctx.next_id++;
    rnode.name = component_name;
    rnode.fragment = FragmentName(node->fragment);
    rnode.wrappers = wrapper_chain;
    
    // Get miniscript string
    auto ms_str = node->ToString(COMPILER_CTX);
    if (ms_str) {
        rnode.miniscript_str = *ms_str;
    }
    
    // Set output socket based on fragment type
    rnode.output_socket = GetOutputSocket(node->fragment);
    
    // Set data fields based on fragment type
    switch (node->fragment) {
        case Fragment::AFTER:
        case Fragment::OLDER:
            rnode.num = node->k;
            break;
        case Fragment::THRESH:
            rnode.thresh = node->k;
            break;
        case Fragment::PK_K:
        case Fragment::PK_H:
            if (!node->keys.empty()) {
                auto key_str = COMPILER_CTX.ToString(node->keys[0]);
                if (key_str) {
                    rnode.key = *key_str;
                }
            }
            break;
        case Fragment::SHA256:
        case Fragment::HASH256:
        case Fragment::RIPEMD160:
        case Fragment::HASH160:
            if (!node->data.empty()) {
                rnode.hash = HexStr(node->data);
            }
            break;
        case Fragment::MULTI:
            rnode.thresh = node->k; // k of n for multi
            break;
        default:
            break;
    }
    
    // Calculate node depth (distance from leaves) for X position
    // Leaves are on the left, root is on the right
    int node_depth = CalculateNodeDepth(node);
    ctx.max_depth = std::max(ctx.max_depth, node_depth);
    
    // X position based on depth (leaves at left, root at right)
    rnode.pos_x = LAYOUT_BASE_X + node_depth * LAYOUT_X_SPACING;
    
    // Y position: increment counter for this depth level
    int y_index = ctx.current_y_at_depth[node_depth]++;
    rnode.pos_y = LAYOUT_BASE_Y + y_index * LAYOUT_Y_SPACING;
    
    // Store the node
    int my_id = rnode.id;
    ctx.nodes.push_back(std::move(rnode));
    size_t my_index = ctx.nodes.size() - 1;
    
    // Process children (they will be placed at lower depth = further left)
    for (size_t i = 0; i < node->subs.size(); ++i) {
        std::vector<std::string> child_wrappers;
        int child_id = BuildReteNodes(node->subs[i], ctx, child_wrappers);
        
        if (child_id > 0) {
            // Find the child node and set up connections
            std::string input_socket = GetChildInputSocket(node->fragment, i);
            std::string child_output_socket = "pol"; // Default
            
            // Find the child node to get its output socket
            for (auto& cn : ctx.nodes) {
                if (cn.id == child_id) {
                    child_output_socket = cn.output_socket;
                    // Add outgoing connection from child to parent
                    cn.output_connections.push_back({my_id, input_socket});
                    break;
                }
            }
            
            // Add incoming connection to parent from child
            ctx.nodes[my_index].input_connections.push_back({child_id, child_output_socket});
            // Track child index for And/Or socket assignment
            ctx.nodes[my_index].input_child_indices.push_back(i);
        }
    }
    
    return my_id;
}

/**
 * Serialize the Rete graph to JSON.
 */
std::string SerializeReteGraph(const ReteGraphContext& ctx, const std::string& original_ms) {
    std::ostringstream oss;
    
    oss << "{";
    // Use miniscript.fun's expected editor ID for compatibility
    oss << "\"id\":\"demo@0.1.0\"";
    
    // Add network field for miniscript.fun compatibility
    oss << ",\"network\":\"bitcoin\"";
    
    // Nodes object
    oss << ",\"nodes\":{";
    
    for (size_t i = 0; i < ctx.nodes.size(); ++i) {
        const ReteNode& n = ctx.nodes[i];
        
        if (i > 0) oss << ",";
        oss << "\"" << n.id << "\":{";
        
        // id
        oss << "\"id\":" << n.id;
        
        // name (component type)
        oss << ",\"name\":\"" << n.name << "\"";
        
        // data object
        oss << ",\"data\":{";
        bool first_data = true;
        
        if (n.num.has_value()) {
            oss << "\"num\":" << n.num.value();
            first_data = false;
        }
        if (n.thresh.has_value()) {
            if (!first_data) oss << ",";
            oss << "\"thresh\":" << n.thresh.value();
            first_data = false;
        }
        if (n.key.has_value()) {
            if (!first_data) oss << ",";
            oss << "\"key\":\"" << EscapeJsonString(n.key.value()) << "\"";
            first_data = false;
        }
        if (n.hash.has_value()) {
            if (!first_data) oss << ",";
            oss << "\"hash\":\"" << EscapeJsonString(n.hash.value()) << "\"";
            first_data = false;
        }
        if (n.mnemonic.has_value()) {
            if (!first_data) oss << ",";
            oss << "\"mnemonic\":\"" << EscapeJsonString(n.mnemonic.value()) << "\"";
            first_data = false;
        }
        if (n.password.has_value()) {
            if (!first_data) oss << ",";
            oss << "\"password\":\"" << EscapeJsonString(n.password.value()) << "\"";
            first_data = false;
        }
        if (n.idx.has_value()) {
            if (!first_data) oss << ",";
            oss << "\"idx\":" << n.idx.value();
            first_data = false;
        }
        
        // Note: We don't add custom __ms_* fields to stay compatible with miniscript.fun
        // The original miniscript can be recovered from the graph structure
        (void)first_data; // Silence unused warning
        
        oss << "}";
        
        // inputs object - connections coming INTO this node
        oss << ",\"inputs\":{";
        
        // For And/Or nodes, ALWAYS declare both pol1 and pol2 sockets (even if empty)
        // This is required by miniscript.fun - binary operators must have both inputs
        if (n.name == "And" || n.name == "Or") {
            std::map<std::string, std::vector<std::pair<int, std::string>>> grouped;
            // Initialize both sockets
            grouped["pol1"] = {};
            grouped["pol2"] = {};
            
            // Assign connections to pol1/pol2 based on original child index (not connection order)
            for (size_t idx = 0; idx < n.input_connections.size(); ++idx) {
                size_t child_idx = idx < n.input_child_indices.size() ? n.input_child_indices[idx] : idx;
                std::string socket = (child_idx == 0) ? "pol1" : "pol2";
                grouped[socket].push_back(n.input_connections[idx]);
            }
            
            bool first_socket = true;
            for (const auto& [socket_name, conns] : grouped) {
                if (!first_socket) oss << ",";
                first_socket = false;
                
                oss << "\"" << socket_name << "\":{\"connections\":[";
                for (size_t c = 0; c < conns.size(); ++c) {
                    if (c > 0) oss << ",";
                    oss << "{\"node\":" << conns[c].first;
                    oss << ",\"output\":\"" << conns[c].second << "\"";
                    oss << ",\"data\":{}}";
                }
                oss << "]}";
            }
        } else if (!n.input_connections.empty()) {
            // Group connections by input socket name
            std::map<std::string, std::vector<std::pair<int, std::string>>> grouped;
            
            if (n.name == "Threshold" || n.name == "Multi") {
                // All inputs go to "policies"
                for (const auto& conn : n.input_connections) {
                    grouped["policies"].push_back(conn);
                }
            } else if (n.name == "Key" || n.name == "BIP39") {
                // Key and BIP39 nodes receive on "key" input
                for (const auto& conn : n.input_connections) {
                    grouped["key"].push_back(conn);
                }
            } else if (n.name == "Descriptor") {
                // Descriptor receives on "pol" input
                for (const auto& conn : n.input_connections) {
                    grouped["pol"].push_back(conn);
                }
            } else if (n.name == "Address") {
                // Address receives on "desc" input
                for (const auto& conn : n.input_connections) {
                    grouped["desc"].push_back(conn);
                }
            } else {
                // Default: single "pol" input
                for (const auto& conn : n.input_connections) {
                    grouped["pol"].push_back(conn);
                }
            }
            
            bool first_socket = true;
            for (const auto& [socket_name, conns] : grouped) {
                if (!first_socket) oss << ",";
                first_socket = false;
                
                oss << "\"" << socket_name << "\":{\"connections\":[";
                for (size_t c = 0; c < conns.size(); ++c) {
                    if (c > 0) oss << ",";
                    oss << "{\"node\":" << conns[c].first;
                    oss << ",\"output\":\"" << conns[c].second << "\"";
                    oss << ",\"data\":{}}";
                }
                oss << "]}";
            }
        } else {
            // No input connections, but some nodes still need empty sockets declared
            // Key nodes always have a "key" input socket (for optional BIP39 connection)
            if (n.name == "Key") {
                oss << "\"key\":{\"connections\":[]}";
            }
            // Other nodes with no connections can have empty inputs object
        }
        oss << "}";
        
        // outputs object - connections going OUT of this node
        oss << ",\"outputs\":{";
        if (!n.output_connections.empty()) {
            oss << "\"" << n.output_socket << "\":{\"connections\":[";
            for (size_t c = 0; c < n.output_connections.size(); ++c) {
                if (c > 0) oss << ",";
                oss << "{\"node\":" << n.output_connections[c].first;
                oss << ",\"input\":\"" << n.output_connections[c].second << "\"";
                oss << ",\"data\":{}}";
            }
            oss << "]}";
        } else {
            // Empty output with no connections (leaf or root)
            oss << "\"" << n.output_socket << "\":{\"connections\":[]}";
        }
        oss << "}";
        
        // position
        oss << ",\"position\":[" << n.pos_x << "," << n.pos_y << "]";
        
        oss << "}";
    }
    
    oss << "}";
    
    // Comments (required by miniscript.fun)  
    oss << ",\"comments\":[]";
    
    oss << "}";
    
    return oss.str();
}

/**
 * Add dummy BIP39, Descriptor, and Address nodes for complete graph mode.
 * This makes the graph fully functional in miniscript.fun.
 */
void AddDummyNodes(ReteGraphContext& ctx, int root_id) {
    // BIP39 mnemonic: "bacon" repeated 24 times
    const std::string BACON_MNEMONIC = "bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon bacon";
    
    // Find all Key nodes and track them for BIP39 connection
    std::vector<size_t> key_node_indices;
    for (size_t i = 0; i < ctx.nodes.size(); ++i) {
        if (ctx.nodes[i].name == "Key") {
            key_node_indices.push_back(i);
        }
    }
    
    // Calculate positions for the extended graph
    // BIP39 nodes go to the left of keys (at depth -1)
    // Descriptor goes to the right of root (at max_depth + 1)
    // Address goes to the right of descriptor (at max_depth + 2)
    int bip39_x = LAYOUT_BASE_X - LAYOUT_X_SPACING;
    int descriptor_x = LAYOUT_BASE_X + (ctx.max_depth + 1) * LAYOUT_X_SPACING;
    int address_x = LAYOUT_BASE_X + (ctx.max_depth + 2) * LAYOUT_X_SPACING;
    
    // Create BIP39 nodes for each Key
    int key_index = 1;
    for (size_t key_idx : key_node_indices) {
        ReteNode& key_node = ctx.nodes[key_idx];
        
        // Create BIP39 node
        ReteNode bip39;
        bip39.id = ctx.next_id++;
        bip39.name = "BIP39";
        bip39.mnemonic = BACON_MNEMONIC;
        
        // Use the original key name as password (for differentiation)
        // This preserves the key identity while generating different keys
        if (key_node.key && !key_node.key->empty()) {
            bip39.password = *key_node.key;
        } else if (key_index > 1) {
            bip39.password = std::to_string(key_index);
        }
        
        // Position: to the left of the key node, same Y
        bip39.pos_x = bip39_x;
        bip39.pos_y = key_node.pos_y;
        bip39.output_socket = "key";
        
        // Connect BIP39 -> Key
        bip39.output_connections.push_back({key_node.id, "key"});
        
        // Update Key node to receive from BIP39
        key_node.input_connections.push_back({bip39.id, "key"});
        
        // Keep the original key name (don't overwrite with "bacon N")
        // key_node.key already has the original name like "key_remote"
        
        ctx.nodes.push_back(std::move(bip39));
        key_index++;
    }
    
    // Find the root node
    size_t root_idx = 0;
    for (size_t i = 0; i < ctx.nodes.size(); ++i) {
        if (ctx.nodes[i].id == root_id) {
            root_idx = i;
            break;
        }
    }
    ReteNode& root_node = ctx.nodes[root_idx];
    
    // Calculate Y position for descriptor/address (centered around root)
    int center_y = root_node.pos_y;
    
    // Create Descriptor node
    ReteNode descriptor;
    descriptor.id = ctx.next_id++;
    descriptor.name = "Descriptor";
    descriptor.pos_x = descriptor_x;
    descriptor.pos_y = center_y;
    descriptor.input_socket = "pol";
    descriptor.output_socket = "desc";
    
    // Connect root -> Descriptor
    descriptor.input_connections.push_back({root_id, root_node.output_socket});
    root_node.output_connections.push_back({descriptor.id, "pol"});
    
    int descriptor_id = descriptor.id;
    ctx.nodes.push_back(std::move(descriptor));
    
    // Create Address node
    ReteNode address;
    address.id = ctx.next_id++;
    address.name = "Address";
    address.idx = 0;  // Default address index
    address.pos_x = address_x;
    address.pos_y = center_y;
    address.input_socket = "desc";
    address.output_socket = "addr";
    
    // Connect Descriptor -> Address
    address.input_connections.push_back({descriptor_id, "desc"});
    
    // Find descriptor in nodes and add output connection
    for (auto& n : ctx.nodes) {
        if (n.id == descriptor_id) {
            n.output_connections.push_back({address.id, "desc"});
            break;
        }
    }
    
    ctx.nodes.push_back(std::move(address));
}

/**
 * Internal implementation of MiniscriptToReteJson.
 * This is inside the anonymous namespace to access ReteGraphContext, etc.
 * @param include_dummy_data If true, adds BIP39, Descriptor, and Address nodes
 */
std::string MiniscriptToReteJsonImpl(const std::string& miniscript_src, bool include_dummy_data = false) {
    std::ostringstream result;
    
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
            result << "{\"error\":\"Failed to parse miniscript expression\",\"valid\":false}";
            return result.str();
        }
        
        if (!parsed->IsValid()) {
            result << "{\"error\":\"Parsed miniscript is not valid\",\"valid\":false}";
            return result.str();
        }
        
        // Build the Rete graph
        ReteGraphContext ctx;
        std::vector<std::string> root_wrappers;
        int root_id = BuildReteNodes(parsed, ctx, root_wrappers);
        
        if (root_id < 0 || ctx.nodes.empty()) {
            result << "{\"error\":\"Failed to build Rete graph\",\"valid\":false}";
            return result.str();
        }
        
        // Add dummy BIP39, Descriptor, Address nodes if requested
        if (include_dummy_data) {
            AddDummyNodes(ctx, root_id);
        }
        
        // Serialize to JSON
        return SerializeReteGraph(ctx, miniscript_src);
        
    } catch (const std::exception& e) {
        result << "{\"error\":\"Exception: " << EscapeJsonString(e.what()) << "\",\"valid\":false}";
    }
    
    return result.str();
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

std::string MiniscriptToReteJson(const std::string& miniscript_src) {
    return MiniscriptToReteJsonImpl(miniscript_src, false);
}

std::string MiniscriptToCompleteReteJson(const std::string& miniscript_src) {
    return MiniscriptToReteJsonImpl(miniscript_src, true);
}
