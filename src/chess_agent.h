#ifndef CHESS_AGENT_H
#define CHESS_AGENT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "neural_net.h"

namespace godot {

class ChessAgent : public Node {
    GDCLASS(ChessAgent, Node)

private:
    NeuralNet* neural_net;
    
    // Neural Net Configuration
    const int INPUT_NODES = 768; // 64 squares * 12 channels
    const int HIDDEN_NODES = 128;
    const int OUTPUT_NODES = 1;

    // Helper to convert internal integer board to NN inputs
    Array encode_board_to_inputs(const std::vector<int>& board_state);

protected:
    static void _bind_methods();

public:
    ChessAgent();
    ~ChessAgent();

    void _ready() override;

    // The main function called from GDScript
    // board_state: A flat Array of 64 integers representing the board
    // possible_moves: An Array of Dictionaries { "start": Vector2, "end": Vector2 }
    // Returns: The best Dictionary from possible_moves
    Dictionary select_best_move(const Array& board_state, const Array& possible_moves);
};

}

#endif
