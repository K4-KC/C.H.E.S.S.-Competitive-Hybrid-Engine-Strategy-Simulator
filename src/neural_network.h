#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "board.h"
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace godot;

// ==================== NEURAL NETWORK INPUT CONFIGURATION ====================

// Input layer size: 12 piece types × 64 squares + extras
// Piece planes: P, N, B, R, Q, K (white), p, n, b, r, q, k (black)
#define NN_PIECE_PLANES     12
#define NN_SQUARES          64
#define NN_PIECE_INPUTS     (NN_PIECE_PLANES * NN_SQUARES)  // 768

// Additional inputs
#define NN_CASTLING_INPUTS  4   // WK, WQ, BK, BQ
#define NN_TURN_INPUT       1   // Side to move (1.0 = white, 0.0 = black)
#define NN_EP_INPUTS        8   // En passant file (one-hot, or 0 if none)

// Total input size
#define NN_TOTAL_INPUTS     (NN_PIECE_INPUTS + NN_CASTLING_INPUTS + NN_TURN_INPUT + NN_EP_INPUTS)  // 781

// ==================== EVALUATION CONSTANTS ====================

#define CHECKMATE_SCORE 100000
#define STALEMATE_SCORE 0

// Piece values for fallback/material evaluation (centipawns)
#define PAWN_VALUE   100
#define KNIGHT_VALUE 320
#define BISHOP_VALUE 330
#define ROOK_VALUE   500
#define QUEEN_VALUE  900

// ==================== NEURAL NETWORK CLASS (AI AGENT) ====================

class NeuralNet : public Node2D {
    GDCLASS(NeuralNet, Node2D)

protected:
    // ==================== NEURAL NETWORK FRAMEWORK ====================

    // Network architecture
    std::vector<int> layer_sizes;  // Size of each layer (input, hidden layers, output)
    std::vector<std::vector<std::vector<float>>> weights;  // weights[layer][neuron][input]
    std::vector<std::vector<float>> biases;  // biases[layer][neuron]
    std::vector<std::vector<float>> activations;  // activations[layer][neuron] (for forward pass)

    // Activation function per layer (for layers 1 to n, layer 0 is input)
    // 0=linear, 1=relu, 2=sigmoid, 3=tanh
    std::vector<int> activation_functions;

    // Network initialized flag
    bool network_initialized;

    // Forward pass through neural network with provided input features
    // Returns the network output value
    float forward_pass(const std::vector<float> &input_features);

    // Activation functions
    float relu(float x) const;
    float sigmoid(float x) const;
    float tanh_activation(float x) const;
    float linear(float x) const;

    // Apply activation function by type
    float apply_activation(float x, int activation_type) const;

protected:
    static void _bind_methods();

public:
    NeuralNet();
    ~NeuralNet();

    void _ready();

    // ==================== NEURAL NETWORK INFERENCE ====================

    // Run inference on provided input features
    // Returns the network output (centipawns for chess evaluation)
    float predict(const Array &input_array);

    // ==================== NEURAL NETWORK UTILITIES ====================

    // Initialize neural network with custom architecture
    // layer_sizes: Array of layer sizes [input_size, hidden1_size, hidden2_size, ..., output_size]
    // default_activation: Default activation function (0=linear, 1=relu, 2=sigmoid, 3=tanh). Default is sigmoid (2)
    // Example: [781, 256, 128, 64, 1] creates a network with 781 inputs, 3 hidden layers, and 1 output
    void initialize_neural_network(const Array &layer_sizes_array, int default_activation = 2);

    // Load neural network weights from file
    // PLACEHOLDER: Will load ONNX model
    bool load_network(const String &path);

    // Set weights and biases for a specific layer
    // layer_index: Which layer to set (0 = first hidden layer, etc.)
    // weights_array: 2D array of weights [neuron_index][input_index]
    // biases_array: 1D array of biases [neuron_index]
    void set_layer_weights(int layer_index, const Array &weights_array, const Array &biases_array);

    // Set activation function for a specific layer or all layers
    // layer_index: Which layer to set (-1 for all layers, 0 for first hidden layer, etc.)
    // activation_type: 0=linear, 1=relu, 2=sigmoid, 3=tanh
    void set_activation_function(int layer_index, int activation_type);

    // Get activation function for a specific layer
    int get_activation_function(int layer_index) const;

    // Get network architecture info
    bool is_network_initialized() const { return network_initialized; }
    Array get_layer_sizes() const;
    int get_num_layers() const { return layer_sizes.empty() ? 0 : static_cast<int>(layer_sizes.size() - 1); }

    // Get the expected input size for this network
    int get_input_size() const { return layer_sizes.empty() ? NN_TOTAL_INPUTS : layer_sizes[0]; }
};

#endif // NEURAL_NETWORK_H
