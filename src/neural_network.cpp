#include "neural_network.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>
#include <cmath>

using namespace godot;

// ==================== NEURAL NETWORK FEATURE EXTRACTION ====================

float NeuralNet::forward_pass(const std::vector<float> &input_features) {
    // If network is not initialized, return 0
    if (!network_initialized || layer_sizes.empty()) {
        return 0.0f;
    }

    // Validate input size
    if (input_features.size() != static_cast<size_t>(layer_sizes[0])) {
        UtilityFunctions::print("Error: Input size mismatch. Expected ", layer_sizes[0],
                                ", got ", input_features.size());
        return 0.0f;
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

    // Forward pass through each layer
    for (size_t layer = 1; layer < layer_sizes.size(); layer++) {
        int prev_layer_size = layer_sizes[layer - 1];
        int current_layer_size = layer_sizes[layer];

        // Check if weights and biases exist for this layer
        if (layer - 1 >= weights.size() || layer - 1 >= biases.size()) {
            UtilityFunctions::print("Error: Weights not set for layer ", layer - 1);
            return 0.0f;
        }

        // Compute activations for each neuron in current layer
        for (int neuron = 0; neuron < current_layer_size; neuron++) {
            float sum = biases[layer - 1][neuron];

            // Sum weighted inputs
            for (int prev_neuron = 0; prev_neuron < prev_layer_size; prev_neuron++) {
                sum += activations[layer - 1][prev_neuron] * weights[layer - 1][neuron][prev_neuron];
            }

            // Apply activation function based on layer configuration
            int activation_type = (layer - 1 < activation_functions.size()) ?
                                  activation_functions[layer - 1] : 2; // Default to sigmoid
            activations[layer][neuron] = apply_activation(sum, activation_type);
        }
    }

    // Return the output (last layer, first neuron)
    return activations.back()[0];
}

// ==================== ACTIVATION FUNCTIONS ====================

float NeuralNet::linear(float x) const {
    return x;
}

float NeuralNet::relu(float x) const {
    return (x > 0.0f) ? x : 0.0f;
}

float NeuralNet::sigmoid(float x) const {
    return 1.0f / (1.0f + std::exp(-x));
}

float NeuralNet::tanh_activation(float x) const {
    return std::tanh(x);
}

float NeuralNet::apply_activation(float x, int activation_type) const {
    switch (activation_type) {
        case 0: return linear(x);
        case 1: return relu(x);
        case 2: return sigmoid(x);
        case 3: return tanh_activation(x);
        default: return sigmoid(x); // Default to sigmoid
    }
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

void NeuralNet::initialize_neural_network(const Array &layer_sizes_array, int activation /* = 2 */) {
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

    // Validate activation type
    if (activation < 0 || activation > 3) {
        UtilityFunctions::print("Warning: Invalid activation type ", activation,
                                ". Using sigmoid (2) as default.");
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

    // Validate input layer size matches expected feature size
    if (layer_sizes[0] != NN_TOTAL_INPUTS) {
        UtilityFunctions::print("Warning: Input layer size ", layer_sizes[0],
                                " does not match expected feature size ", NN_TOTAL_INPUTS);
        UtilityFunctions::print("Adjusting input layer size to ", NN_TOTAL_INPUTS);
        layer_sizes[0] = NN_TOTAL_INPUTS;
    }

    // Initialize weights and biases with random values (Xavier initialization)
    int num_weight_layers = layer_sizes.size() - 1;
    weights.resize(num_weight_layers);
    biases.resize(num_weight_layers);

    // Initialize activation functions for each layer
    // By default, use specified activation for hidden layers, linear for output
    activation_functions.resize(num_weight_layers);
    for (int i = 0; i < num_weight_layers; i++) {
        if (i == num_weight_layers - 1) {
            // Output layer uses linear activation by default
            activation_functions[i] = 0;
        } else {
            // Hidden layers use specified default activation
            activation_functions[i] = activation;
        }
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

    // Print activation functions
    String activation_names[] = {"linear", "relu", "sigmoid", "tanh"};
    String activations_str = "  Activations: [";
    for (size_t i = 0; i < activation_functions.size(); i++) {
        int act_type = activation_functions[i];
        activations_str += (act_type >= 0 && act_type <= 3) ? activation_names[act_type] : "unknown";
        if (i < activation_functions.size() - 1) activations_str += ", ";
    }
    activations_str += "]";
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

void NeuralNet::set_activation_function(int layer_index, int activation_type) {
    if (!network_initialized) {
        UtilityFunctions::print("Error: Network not initialized. Call initialize_neural_network first.");
        return;
    }

    // Validate activation type
    if (activation_type < 0 || activation_type > 3) {
        UtilityFunctions::print("Error: Invalid activation type ", activation_type,
                                ". Valid range: 0 (linear), 1 (relu), 2 (sigmoid), 3 (tanh)");
        return;
    }

    // Set for all layers if layer_index is -1
    if (layer_index == -1) {
        for (size_t i = 0; i < activation_functions.size(); i++) {
            activation_functions[i] = activation_type;
        }
        UtilityFunctions::print("All layers set to activation type ", activation_type);
        return;
    }

    // Validate layer index
    if (layer_index < 0 || layer_index >= static_cast<int>(activation_functions.size())) {
        UtilityFunctions::print("Error: Invalid layer index ", layer_index,
                                ". Valid range: 0 to ", activation_functions.size() - 1,
                                " (or -1 for all layers)");
        return;
    }

    activation_functions[layer_index] = activation_type;

    String activation_names[] = {"linear", "relu", "sigmoid", "tanh"};
    UtilityFunctions::print("Layer ", layer_index, " activation set to ",
                            activation_names[activation_type]);
}

int NeuralNet::get_activation_function(int layer_index) const {
    if (!network_initialized) {
        UtilityFunctions::print("Error: Network not initialized.");
        return -1;
    }

    if (layer_index < 0 || layer_index >= static_cast<int>(activation_functions.size())) {
        UtilityFunctions::print("Error: Invalid layer index ", layer_index);
        return -1;
    }

    return activation_functions[layer_index];
}

bool NeuralNet::load_network(const String &path) {
    // ==================== PLACEHOLDER FOR ONNX MODEL LOADING ====================
    //
    // TODO: Implement ONNX Runtime model loading:
    //
    // 1. Initialize ONNX Runtime environment (once):
    //    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ChessNN");
    //
    // 2. Create session options:
    //    Ort::SessionOptions session_options;
    //    session_options.SetIntraOpNumThreads(1);
    //
    // 3. Load the model:
    //    std::string model_path = path.utf8().get_data();
    //    ort_session = new Ort::Session(env, model_path.c_str(), session_options);
    //
    // 4. Get input/output names:
    //    Ort::AllocatorWithDefaultOptions allocator;
    //    input_name = ort_session->GetInputName(0, allocator);
    //    output_name = ort_session->GetOutputName(0, allocator);
    //
    // 5. Set use_neural_network = true on success
    //
    // ==================== END PLACEHOLDER ====================
    
    UtilityFunctions::print("NeuralNet::load_network() - PLACEHOLDER: Would load model from ", path);
    UtilityFunctions::print("Neural network not yet implemented. Using material evaluation.");
    
    // Return false since we haven't actually loaded anything
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
    ClassDB::bind_method(D_METHOD("initialize_neural_network", "layer_sizes", "activation"), &NeuralNet::initialize_neural_network, DEFVAL(2));
    ClassDB::bind_method(D_METHOD("load_network", "path"), &NeuralNet::load_network);
    ClassDB::bind_method(D_METHOD("set_layer_weights", "layer_index", "weights", "biases"), &NeuralNet::set_layer_weights);
    ClassDB::bind_method(D_METHOD("set_activation_function", "layer_index", "activation_type"), &NeuralNet::set_activation_function);
    ClassDB::bind_method(D_METHOD("get_activation_function", "layer_index"), &NeuralNet::get_activation_function);
    ClassDB::bind_method(D_METHOD("is_network_initialized"), &NeuralNet::is_network_initialized);
    ClassDB::bind_method(D_METHOD("get_layer_sizes"), &NeuralNet::get_layer_sizes);
    ClassDB::bind_method(D_METHOD("get_num_layers"), &NeuralNet::get_num_layers);
    ClassDB::bind_method(D_METHOD("get_input_size"), &NeuralNet::get_input_size);
}

