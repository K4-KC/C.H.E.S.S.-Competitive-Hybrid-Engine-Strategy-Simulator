#include "chess_agent.h"

// Godot includes for binding, engine checks, and logging.
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// Bind methods exposed to GDScript.
void ChessAgent::_bind_methods() {
    ClassDB::bind_method(
        D_METHOD("select_best_move", "possible_moves"),
        &ChessAgent::select_best_move
    );
}

// Constructor: just initialize pointer; actual net is created in _ready.
ChessAgent::ChessAgent() {
    neural_net = nullptr;
}

ChessAgent::~ChessAgent() {}

// Set up the neural network when the game runs (skip in editor).
void ChessAgent::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    // Instantiate and configure the Neural Network.
    neural_net = memnew(NeuralNet);
    add_child(neural_net);

    Array layers;
    layers.push_back(INPUT_NODES);
    layers.push_back(HIDDEN_NODES);
    layers.push_back(OUTPUT_NODES);
    neural_net->set_layer_sizes(layers);

    UtilityFunctions::print("C++ ChessAgent initialized with NeuralNet.");
}

// Evaluate each move with the neural net and return the highest-scoring move.
Dictionary ChessAgent::select_best_move(const Array &possible_moves) {
    if (possible_moves.size() == 0) {
        return Dictionary();
    }

    // Default to first move as fallback.
    Dictionary best_move = possible_moves[0]; // Fallback to 0th move
    double best_score = -1.0; // Initialize lower than lowest possible sigmoid (0.0)

    // Iterate through all candidate moves.
    // Each move already contains the "future state" of the board in the "board" key.
    for (int i = 0; i < possible_moves.size(); i++) {
        Dictionary move = possible_moves[i];

        if (!move.has("board")) {
            continue;
        }
        
        // 1. Get the future board state directly from the move data.
        Array future_board_state = move["board"];

        // 2. Encode board into neural net input vector.
        Array inputs = encode_board_to_inputs(future_board_state);

        // 3. Run Neural Net to get a scalar evaluation.
        neural_net->set_inputs(inputs);
        neural_net->compute();
        Array outputs = neural_net->get_outputs();
        double score = (double)outputs[0];

        // 4. Track best-scoring move.
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    UtilityFunctions::print("ChessAgent selected move with score: ", best_score);
    return best_move;
}

// Encode an 8x8 board of Dictionaries into a 768-length one-hot input vector for the NN.
Array ChessAgent::encode_board_to_inputs(const Array &board_state_2d) {
    Array inputs;

    // We need 768 inputs (64 squares * 12 piece types).
    // Mapping matches the previous logic: 
    // White: P=0, N=1, B=2, R=3, Q=4, K=5
    // Black: P=6, N=7, B=8, R=9, Q=10, K=11
    
    // PieceType enum from BoardRules (presumed):
    // PAWN=0, ROOK=1, KNIGHT=2, BISHOP=3, QUEEN=4, KING=5
    
    // Re-mapping needed to match the 0-11 channel structure:
    // White (color=0): Pawn(0)->0, Knight(2)->1, Bishop(3)->2, Rook(1)->3, Queen(4)->4, King(5)->5
    // Black (color=1): Pawn(0)->6, Knight(2)->7, Bishop(3)->8, Rook(1)->9, Queen(4)->10, King(5)->11
    
    // Standard map to convert BoardRules PieceType to "Network Order" (P, N, B, R, Q, K)
    // BoardRules: P=0, R=1, N=2, B=3, Q=4, K=5
    // Network:    P=0, N=1, B=2, R=3, Q=4, K=5
    int type_map[6] = {0, 3, 1, 2, 4, 5}; 

    // Iterate 8x8 grid
    for (int y = 0; y < 8; y++) {
        Array row = board_state_2d[y];
        for (int x = 0; x < 8; x++) {
            
            Dictionary piece_data = row[x];
            int active_channel = -1;

            if (piece_data.has("active") && (bool)piece_data["active"]) {
                int type = (int)piece_data["type"];   // 0-5
                int color = (int)piece_data["color"]; // 0=White, 1=Black
                
                if (type >= 0 && type <= 5) {
                    int net_type = type_map[type];
                    active_channel = net_type + (color * 6);
                }
            }

            // Fill 12 channels for this square
            for (int channel = 0; channel < 12; channel++) {
                if (channel == active_channel) {
                    inputs.push_back(1.0);
                } else {
                    inputs.push_back(0.0);
                }
            }
        }
    }

    return inputs;
}
