#ifndef CHESS_AGENT_H
#define CHESS_AGENT_H

// Godot node base class and Array type.
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

// Local dependency: the neural network used to evaluate positions.
#include "neural_net.h"
#include <vector>

namespace godot {

// C++ chess agent node that uses NeuralNet to score and pick moves.
class ChessAgent : public Node {
    GDCLASS(ChessAgent, Node)

private:
    // Owned neural network instance used for evaluation.
    NeuralNet *neural_net;

    // Neural Net configuration:
    // - 768 input nodes: 64 squares * 12 piece channels.
    // - Hidden and output sizes are fixed here for simplicity.
    const int INPUT_NODES = 768; // 64 squares * 12 types of pieces
    const int HIDDEN_NODES = 128;
    const int OUTPUT_NODES = 1;

    // Convert a 8x8 board Array (of Dictionaries) into 768 input features for the net.
    Array encode_board_to_inputs(const Array &board_state_2d);

protected:
    static void _bind_methods();

public:
    ChessAgent();
    ~ChessAgent();

    // Standard Godot entry point when node is added to the scene tree.
    void _ready() override;

    // Select the best move from possible_moves using the neural net evaluation.
    // Now only takes the list of moves because each move contains its future board state.
    Dictionary select_best_move(const Array &possible_moves);
};

} // namespace godot

#endif
