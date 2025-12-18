#include "neural_network.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>
#include <cmath>

using namespace godot;

// ==================== NEURAL NETWORK FORWARD PASS ====================

void NeuralNet::forward_pass_linear(size_t layer_idx) {
    int prev_layer_size = layer_sizes[layer_idx - 1];
    int current_layer_size = layer_sizes[layer_idx];
    size_t weight_idx = layer_idx - 1;

    for (int neuron = 0; neuron < current_layer_size; neuron++) {
        float sum = biases[weight_idx][neuron];
        for (int prev_neuron = 0; prev_neuron < prev_layer_size; prev_neuron++) {
            sum += activations[layer_idx - 1][prev_neuron] * weights[weight_idx][neuron][prev_neuron];
        }
        activations[layer_idx][neuron] = sum;  // Linear activation (no transformation)
    }
}

void NeuralNet::forward_pass_relu(size_t layer_idx) {
    int prev_layer_size = layer_sizes[layer_idx - 1];
    int current_layer_size = layer_sizes[layer_idx];
    size_t weight_idx = layer_idx - 1;

    for (int neuron = 0; neuron < current_layer_size; neuron++) {
        float sum = biases[weight_idx][neuron];
        for (int prev_neuron = 0; prev_neuron < prev_layer_size; prev_neuron++) {
            sum += activations[layer_idx - 1][prev_neuron] * weights[weight_idx][neuron][prev_neuron];
        }
        activations[layer_idx][neuron] = relu(sum);
    }
}

void NeuralNet::forward_pass_sigmoid(size_t layer_idx) {
    int prev_layer_size = layer_sizes[layer_idx - 1];
    int current_layer_size = layer_sizes[layer_idx];
    size_t weight_idx = layer_idx - 1;

    for (int neuron = 0; neuron < current_layer_size; neuron++) {
        float sum = biases[weight_idx][neuron];
        for (int prev_neuron = 0; prev_neuron < prev_layer_size; prev_neuron++) {
            sum += activations[layer_idx - 1][prev_neuron] * weights[weight_idx][neuron][prev_neuron];
        }
        activations[layer_idx][neuron] = sigmoid(sum);
    }
}

void NeuralNet::forward_pass_tanh(size_t layer_idx) {
    int prev_layer_size = layer_sizes[layer_idx - 1];
    int current_layer_size = layer_sizes[layer_idx];
    size_t weight_idx = layer_idx - 1;

    for (int neuron = 0; neuron < current_layer_size; neuron++) {
        float sum = biases[weight_idx][neuron];
        for (int prev_neuron = 0; prev_neuron < prev_layer_size; prev_neuron++) {
            sum += activations[layer_idx - 1][prev_neuron] * weights[weight_idx][neuron][prev_neuron];
        }
        activations[layer_idx][neuron] = tanh_activation(sum);
    }
}

float NeuralNet::forward_pass(const std::vector<float> &input_features) {
    // If network is not initialized, return 0.5 (neutral)
    if (!network_initialized || layer_sizes.empty()) {
        return 0.5f;
    }

    // Validate input size
    if (input_features.size() != static_cast<size_t>(layer_sizes[0])) {
        UtilityFunctions::print("Error: Input size mismatch. Expected ", layer_sizes[0],
                                ", got ", input_features.size());
        return 0.5f;
    }

    // Check if weights are set
    if (weights.empty() || biases.empty()) {
        UtilityFunctions::print("Error: Weights not set");
        return 0.5f;
    }

    // Ensure activations storage is properly sized
    if (activations.size() != layer_sizes.size()) {
        activations.resize(layer_sizes.size());
        for (size_t i = 0; i < layer_sizes.size(); i++) {
            activations[i].resize(layer_sizes[i]);
        }
    }

    // Set input layer activations
    for (size_t i = 0; i < input_features.size(); i++) {
        activations[0][i] = input_features[i];
    }

    // Forward pass through hidden layers
    size_t num_layers = layer_sizes.size();
    for (size_t layer = 1; layer < num_layers - 1; layer++) {
        // Get activation type for this layer
        int activation_type = (layer - 1 < activation_functions.size()) ?
                              activation_functions[layer - 1] : 2;  // Default to sigmoid

        // Call specialized function based on activation type
        switch (activation_type) {
            case 0: forward_pass_linear(layer); break;
            case 1: forward_pass_relu(layer); break;
            case 2: forward_pass_sigmoid(layer); break;
            case 3: forward_pass_tanh(layer); break;
            default: forward_pass_sigmoid(layer); break;
        }
    }

    // Output layer (always uses sigmoid to keep output between 0 and 1)
    if (num_layers > 1) {
        size_t output_layer = num_layers - 1;
        int prev_layer_size = layer_sizes[output_layer - 1];
        size_t weight_idx = output_layer - 1;

        // Compute output (single neuron with sigmoid)
        float sum = biases[weight_idx][0];
        for (int prev_neuron = 0; prev_neuron < prev_layer_size; prev_neuron++) {
            sum += activations[output_layer - 1][prev_neuron] * weights[weight_idx][0][prev_neuron];
        }

        // Apply sigmoid to keep output between 0 and 1
        float output = sigmoid(sum);
        activations[output_layer][0] = output;

        return output;
    }

    return 0.5f;  // Should never reach here
}

// ==================== NEURAL NETWORK INFERENCE ====================

float NeuralNet::predict(const Array &input_array) {
    // Convert Array to std::vector<float>
    std::vector<float> input_vec;
    input_vec.reserve(input_array.size());

    for (int i = 0; i < input_array.size(); i++) {
        input_vec.push_back(input_array[i]);
    }

    return forward_pass(input_vec);
}

// ==================== NEURAL NETWORK UTILITIES ====================

int NeuralNet::activation_string_to_int(const String &activation_str) const {
    String lower = activation_str.to_lower();
    if (lower == "linear") return 0;
    if (lower == "relu") return 1;
    if (lower == "sigmoid") return 2;
    if (lower == "tanh") return 3;
    return -1;  // Invalid
}

String NeuralNet::activation_int_to_string(int activation_type) const {
    switch (activation_type) {
        case 0: return "linear";
        case 1: return "relu";
        case 2: return "sigmoid";
        case 3: return "tanh";
        default: return "";
    }
}

void NeuralNet::initialize_neural_network(const Array &layer_sizes_array, const String &default_activation /* = "sigmoid" */) {
    // Clear existing network
    layer_sizes.clear();
    weights.clear();
    biases.clear();
    activations.clear();
    activation_functions.clear();
    network_initialized = false;

    // Validate input
    if (layer_sizes_array.size() < 2) {
        UtilityFunctions::print("Error: Neural network must have at least 2 layers (input and output)");
        return;
    }

    // Convert activation string to int
    int activation = activation_string_to_int(default_activation);
    if (activation < 0) {
        UtilityFunctions::print("Warning: Invalid activation type '", default_activation,
                                "'. Valid options: linear, relu, sigmoid, tanh. Using sigmoid as default.");
        activation = 2;
    }

    // Parse layer sizes
    for (int i = 0; i < layer_sizes_array.size(); i++) {
        int size = layer_sizes_array[i];
        if (size <= 0) {
            UtilityFunctions::print("Error: Layer size must be positive, got ", size, " at index ", i);
            return;
        }
        layer_sizes.push_back(size);
    }

    // Initialize weights and biases with random values (Xavier initialization)
    int num_weight_layers = layer_sizes.size() - 1;
    weights.resize(num_weight_layers);
    biases.resize(num_weight_layers);

    // Initialize activation functions for each layer
    // Note: Output layer always uses sigmoid (enforced in forward_pass)
    // So we only store activations for hidden layers
    activation_functions.resize(num_weight_layers - 1);  // Exclude output layer
    for (int i = 0; i < num_weight_layers - 1; i++) {
        activation_functions[i] = activation;
    }

    for (int layer = 0; layer < num_weight_layers; layer++) {
        int input_size = layer_sizes[layer];
        int output_size = layer_sizes[layer + 1];

        // Initialize weights for this layer
        weights[layer].resize(output_size);
        biases[layer].resize(output_size);

        // Xavier initialization factor
        float xavier_factor = std::sqrt(2.0f / (input_size + output_size));

        for (int neuron = 0; neuron < output_size; neuron++) {
            weights[layer][neuron].resize(input_size);

            // Initialize weights with small random values
            for (int input = 0; input < input_size; input++) {
                // Simple random initialization (in practice, you'd load these from a file)
                weights[layer][neuron][input] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * xavier_factor;
            }

            // Initialize bias to zero
            biases[layer][neuron] = 0.0f;
        }
    }

    // Initialize activation storage
    activations.resize(layer_sizes.size());
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        activations[i].resize(layer_sizes[i]);
    }

    network_initialized = true;

    // Print network architecture with activation functions
    UtilityFunctions::print("Neural network initialized with architecture:");
    String arch = "  [";
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        arch += String::num_int64(layer_sizes[i]);
        if (i < layer_sizes.size() - 1) arch += ", ";
    }
    arch += "]";
    UtilityFunctions::print(arch);

    // Print activation functions (hidden layers + output)
    String activation_names[] = {"linear", "relu", "sigmoid", "tanh"};
    String activations_str = "  Activations: [";
    for (size_t i = 0; i < activation_functions.size(); i++) {
        int act_type = activation_functions[i];
        activations_str += (act_type >= 0 && act_type <= 3) ? activation_names[act_type] : "unknown";
        activations_str += ", ";
    }
    activations_str += "sigmoid]";  // Output layer always uses sigmoid
    UtilityFunctions::print(activations_str);
}

void NeuralNet::set_layer_weights(int layer_index, const Array &weights_array, const Array &biases_array) {
    if (!network_initialized) {
        UtilityFunctions::print("Error: Network not initialized. Call initialize_neural_network first.");
        return;
    }

    if (layer_index < 0 || layer_index >= static_cast<int>(weights.size())) {
        UtilityFunctions::print("Error: Invalid layer index ", layer_index,
                                ". Valid range: 0 to ", weights.size() - 1);
        return;
    }

    int output_size = layer_sizes[layer_index + 1];
    int input_size = layer_sizes[layer_index];

    // Validate weights array dimensions
    if (weights_array.size() != output_size) {
        UtilityFunctions::print("Error: Expected ", output_size, " neurons, got ", weights_array.size());
        return;
    }

    // Validate biases array
    if (biases_array.size() != output_size) {
        UtilityFunctions::print("Error: Expected ", output_size, " biases, got ", biases_array.size());
        return;
    }

    // Set weights
    for (int neuron = 0; neuron < output_size; neuron++) {
        Array neuron_weights = weights_array[neuron];

        if (neuron_weights.size() != input_size) {
            UtilityFunctions::print("Error: Neuron ", neuron, " expected ", input_size,
                                    " weights, got ", neuron_weights.size());
            return;
        }

        for (int input = 0; input < input_size; input++) {
            weights[layer_index][neuron][input] = neuron_weights[input];
        }

        biases[layer_index][neuron] = biases_array[neuron];
    }

    UtilityFunctions::print("Layer ", layer_index, " weights and biases set successfully");
}

Array NeuralNet::get_layer_sizes() const {
    Array result;
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        result.append(layer_sizes[i]);
    }
    return result;
}

void NeuralNet::set_activation_function(int layer_index, const String &activation_type) {
    if (!network_initialized) {
        UtilityFunctions::print("Error: Network not initialized. Call initialize_neural_network first.");
        return;
    }

    // Convert activation string to int
    int activation = activation_string_to_int(activation_type);
    if (activation < 0) {
        UtilityFunctions::print("Error: Invalid activation type '", activation_type,
                                "'. Valid options: linear, relu, sigmoid, tanh");
        return;
    }

    // Set for all hidden layers if layer_index is -1
    if (layer_index == -1) {
        for (size_t i = 0; i < activation_functions.size(); i++) {
            activation_functions[i] = activation;
        }
        UtilityFunctions::print("All hidden layers set to activation '", activation_type, "'");
        UtilityFunctions::print("Note: Output layer always uses sigmoid");
        return;
    }

    // Validate layer index (can only set hidden layers, not output)
    if (layer_index < 0 || layer_index >= static_cast<int>(activation_functions.size())) {
        UtilityFunctions::print("Error: Invalid layer index ", layer_index,
                                ". Valid range: 0 to ", activation_functions.size() - 1,
                                " (or -1 for all hidden layers)");
        UtilityFunctions::print("Note: Output layer activation cannot be changed (always sigmoid)");
        return;
    }

    activation_functions[layer_index] = activation;
    UtilityFunctions::print("Hidden layer ", layer_index, " activation set to '", activation_type, "'");
}

String NeuralNet::get_activation_function(int layer_index) const {
    if (!network_initialized) {
        UtilityFunctions::print("Error: Network not initialized.");
        return "";
    }

    if (layer_index < 0 || layer_index >= static_cast<int>(activation_functions.size())) {
        UtilityFunctions::print("Error: Invalid layer index ", layer_index);
        return "";
    }

    return activation_int_to_string(activation_functions[layer_index]);
}

bool NeuralNet::load_network(const String &path) {
    // ==================== PLACEHOLDER FOR MODEL LOADING ====================
    //
    // TODO: Implement model loading from file:
    //
    // 1. Load network architecture (layer sizes, activation functions)
    // 2. Load weights and biases for each layer
    // 3. Initialize the network with loaded parameters
    // 4. Return true on success
    //
    // ==================== END PLACEHOLDER ====================

    UtilityFunctions::print("NeuralNet::load_network() - PLACEHOLDER: Would load model from ", path);
    UtilityFunctions::print("Model loading not yet implemented.");

    return false;
}

// ==================== CONSTRUCTOR/DESTRUCTOR ====================

NeuralNet::NeuralNet() {
    network_initialized = false;
}

NeuralNet::~NeuralNet() {
}

void NeuralNet::_ready() {
}

// ==================== GODOT BINDINGS ====================

void NeuralNet::_bind_methods() {
    // Neural network inference
    ClassDB::bind_method(D_METHOD("predict", "input_array"), &NeuralNet::predict);

    // Neural network utilities
    ClassDB::bind_method(D_METHOD("initialize_neural_network", "layer_sizes", "activation"), &NeuralNet::initialize_neural_network, DEFVAL("sigmoid"));
    ClassDB::bind_method(D_METHOD("load_network", "path"), &NeuralNet::load_network);
    ClassDB::bind_method(D_METHOD("set_layer_weights", "layer_index", "weights", "biases"), &NeuralNet::set_layer_weights);
    ClassDB::bind_method(D_METHOD("set_activation_function", "layer_index", "activation_type"), &NeuralNet::set_activation_function);
    ClassDB::bind_method(D_METHOD("get_activation_function", "layer_index"), &NeuralNet::get_activation_function);
    ClassDB::bind_method(D_METHOD("is_network_initialized"), &NeuralNet::is_network_initialized);
    ClassDB::bind_method(D_METHOD("get_layer_sizes"), &NeuralNet::get_layer_sizes);
    ClassDB::bind_method(D_METHOD("get_num_layers"), &NeuralNet::get_num_layers);
    ClassDB::bind_method(D_METHOD("get_input_size"), &NeuralNet::get_input_size);
}

