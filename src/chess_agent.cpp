#include "chess_agent.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void ChessAgent::_bind_methods() {
    ClassDB::bind_method(D_METHOD("select_best_move", "board_state", "possible_moves"), &ChessAgent::select_best_move);
}

ChessAgent::ChessAgent() {
    neural_net = nullptr;
}

ChessAgent::~ChessAgent() {
    // NeuralNet is a child node, Godot handles destruction usually, 
    // but if we new() it without add_child, we must delete.
    // Here we will add it as a child in _ready.
}

void ChessAgent::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) return;

    // Instantiate and configure the Neural Network
    neural_net = memnew(NeuralNet);
    add_child(neural_net);

    Array layers;
    layers.push_back(INPUT_NODES);
    layers.push_back(HIDDEN_NODES);
    layers.push_back(OUTPUT_NODES);
    
    neural_net->set_layer_sizes(layers);
    UtilityFunctions::print("C++ ChessAgent initialized with NeuralNet.");
}

Dictionary ChessAgent::select_best_move(const Array& board_state, const Array& possible_moves) {
    if (possible_moves.size() == 0) return Dictionary();

    // Convert Godot Array to C++ vector for faster access
    std::vector<int> current_board;
    current_board.resize(64);
    for(int i = 0; i < 64; i++) {
        current_board[i] = (int)board_state[i];
    }

    Dictionary best_move;
    double best_score = -100.0; // Lower than any sigmoid output (0.0 to 1.0)
    bool move_found = false;

    // Iterate through all moves
    for (int i = 0; i < possible_moves.size(); i++) {
        Dictionary move = possible_moves[i];
        Vector2 start = move["start"];
        Vector2 end = move["end"];

        int start_idx = (int)start.y * 8 + (int)start.x;
        int end_idx = (int)end.y * 8 + (int)end.x;

        if (start_idx < 0 || start_idx >= 64 || end_idx < 0 || end_idx >= 64) continue;

        // --- SIMULATION STEP ---
        // 1. Copy board
        std::vector<int> temp_board = current_board;
        
        // 2. Execute move (Overwrite destination with source, clear source)
        int piece = temp_board[start_idx];
        temp_board[end_idx] = piece;
        temp_board[start_idx] = 0;

        // 3. Encode to Inputs
        Array inputs = encode_board_to_inputs(temp_board);

        // 4. Run Neural Net
        neural_net->set_inputs(inputs);
        neural_net->compute();
        Array outputs = neural_net->get_outputs();
        
        double score = (double)outputs[0];

        // 5. Check if best
        if (score > best_score) {
            best_score = score;
            best_move = move;
            move_found = true;
        }
    }

    if (move_found) {
        UtilityFunctions::print("ChessAgent selected move with score: ", best_score);
        return best_move;
    }
    
    return possible_moves[0]; // Fallback
}

Array ChessAgent::encode_board_to_inputs(const std::vector<int>& board_state) {
    Array inputs;
    // We need 768 inputs (64 squares * 12 piece types)
    // Map: 1=WP, 2=WN, 3=WB, 4=WR, 5=WQ, 6=WK
    //      7=BP, 8=BN, 9=BB, 10=BR, 11=BQ, 12=BK
    
    // We will push 768 zeros or ones
    // Optimization: Pre-allocate resize if possible in GDScript, but here we push_back
    
    for (int i = 0; i < 64; i++) {
        int piece_val = board_state[i];
        
        // There are 12 channels per square.
        // Channel 0-5: White P, N, B, R, Q, K
        // Channel 6-11: Black P, N, B, R, Q, K
        
        for (int channel = 0; channel < 12; channel++) {
            double val = 0.0;
            
            if (piece_val != 0) {
                // Map piece_val to channel index
                // piece_val 1 -> channel 0
                // piece_val 7 -> channel 6
                int target_channel = piece_val - 1; 
                
                if (channel == target_channel) {
                    val = 1.0;
                }
            }
            inputs.push_back(val);
        }
    }
    return inputs;
}
