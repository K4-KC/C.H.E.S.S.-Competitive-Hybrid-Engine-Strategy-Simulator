#include "NNNode.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace godot;

void NNNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_layer_sizes", "sizes"), &NNNode::set_layer_sizes);
    ClassDB::bind_method(D_METHOD("get_layer_sizes"), &NNNode::get_layer_sizes);
    
    ClassDB::bind_method(D_METHOD("set_inputs", "inputs"), &NNNode::set_inputs);
    ClassDB::bind_method(D_METHOD("get_outputs"), &NNNode::get_outputs);
    
    ClassDB::bind_method(D_METHOD("compute"), &NNNode::compute);
    
    ClassDB::add_property("NNNode", 
        PropertyInfo(Variant::ARRAY, "layer_sizes"), 
        "set_layer_sizes", 
        "get_layer_sizes");
}

NNNode::NNNode() {
    network_initialized = false;
    srand(time(nullptr));
}

NNNode::~NNNode() {
}

void NNNode::_ready() {
    if (!Engine::get_singleton()->is_editor_hint()) {
        if (network_initialized && input_values.size() > 0) {
            compute();
            UtilityFunctions::print("Neural Network Output:");
            for (size_t i = 0; i < output_values.size(); i++) {
                UtilityFunctions::print("  Output[", (int)i, "] = ", output_values[i]);
            }
        }
    }
}

double NNNode::sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

void NNNode::initialize_network() {
    if (layer_sizes.size() < 2) {
        UtilityFunctions::print("Error: Need at least 2 layers (output and input)");
        return;
    }

    weights.clear();
    biases.clear();
    activations.clear();
    computed.clear();

    // Initialize activations and computed flags for all layers
    activations.resize(layer_sizes.size());
    computed.resize(layer_sizes.size());
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        activations[i].resize(layer_sizes[i], 0.0);
        computed[i].resize(layer_sizes[i], false);
    }

    // Initialize biases from output to second-to-last layer (not for input layer)
    for (int layer = 0; layer < (int)layer_sizes.size() - 1; layer++) {
        std::vector<double> layer_biases;
        for (int neuron = 0; neuron < layer_sizes[layer]; neuron++) {
            double random_bias = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            layer_biases.push_back(random_bias);
        }
        biases.push_back(layer_biases);
    }

    // Initialize weights from output to input: weights[layer] connects layer[layer+1] to layer[layer]
    for (int layer = 0; layer < (int)layer_sizes.size() - 1; layer++) {
        int current_layer_size = layer_sizes[layer];
        int next_layer_size = layer_sizes[layer + 1];

        std::vector<std::vector<double>> layer_weights;
        for (int neuron = 0; neuron < current_layer_size; neuron++) {
            std::vector<double> neuron_weights;
            for (int next_neuron = 0; next_neuron < next_layer_size; next_neuron++) {
                double random_weight = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                neuron_weights.push_back(random_weight);
            }
            layer_weights.push_back(neuron_weights);
        }
        weights.push_back(layer_weights);
    }

    network_initialized = true;
    UtilityFunctions::print("Neural Network initialized (Neuron-level recursion):");
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        if (i == 0) {
            UtilityFunctions::print("  Layer ", (int)i, " (OUTPUT): ", layer_sizes[i], " neurons");
        } else if (i == layer_sizes.size() - 1) {
            UtilityFunctions::print("  Layer ", (int)i, " (INPUT): ", layer_sizes[i], " neurons");
        } else {
            UtilityFunctions::print("  Layer ", (int)i, " (HIDDEN): ", layer_sizes[i], " neurons");
        }
    }
}

double NNNode::compute_neuron_recursive(int layer, int neuron) {
    int input_layer_index = layer_sizes.size() - 1;
    
    // Base case: if at input layer, return the input value directly
    if (layer == input_layer_index) {
        activations[layer][neuron] = input_values[neuron];
        computed[layer][neuron] = true;
        return activations[layer][neuron];
    }
    
    // If this neuron is already computed, return cached value
    if (computed[layer][neuron]) {
        return activations[layer][neuron];
    }
    
    // Recursive case: compute weighted sum by recursively computing all neurons in next layer
    double sum = biases[layer][neuron];
    int next_layer = layer + 1;
    
    // For each neuron in the next layer, recursively compute its activation
    for (int next_neuron = 0; next_neuron < layer_sizes[next_layer]; next_neuron++) {
        // Recursive call: compute the next neuron's activation
        double next_activation = compute_neuron_recursive(next_layer, next_neuron);
        
        // Add weighted contribution to current neuron's sum
        sum += next_activation * weights[layer][neuron][next_neuron];
    }
    
    // Apply sigmoid activation function
    activations[layer][neuron] = sigmoid(sum);
    computed[layer][neuron] = true;
    
    return activations[layer][neuron];
}

void NNNode::forward_propagation() {
    if (!network_initialized) {
        UtilityFunctions::print("Error: Network not initialized");
        return;
    }

    // Reset computed flags for new computation
    for (size_t i = 0; i < computed.size(); i++) {
        for (size_t j = 0; j < computed[i].size(); j++) {
            computed[i][j] = false;
        }
    }

    // Compute each output neuron (layer 0) recursively
    // Each output neuron will trigger recursive computation of all neurons it depends on
    output_values.clear();
    for (int neuron = 0; neuron < layer_sizes[0]; neuron++) {
        double output = compute_neuron_recursive(0, neuron);
        output_values.push_back(output);
    }
}

void NNNode::set_layer_sizes(const Array& sizes) {
    layer_sizes.clear();
    for (int i = 0; i < sizes.size(); i++) {
        layer_sizes.push_back((int)sizes[i]);
    }
    initialize_network();
}

Array NNNode::get_layer_sizes() const {
    Array result;
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        result.append(layer_sizes[i]);
    }
    return result;
}

void NNNode::set_inputs(const Array& inputs) {
    input_values.clear();
    for (int i = 0; i < inputs.size(); i++) {
        input_values.push_back((double)inputs[i]);
    }
    
    int input_layer_index = layer_sizes.size() - 1;
    if (network_initialized && input_values.size() != (size_t)layer_sizes[input_layer_index]) {
        UtilityFunctions::print("Warning: Input size (", (int)input_values.size(), 
                                ") doesn't match input layer size (", layer_sizes[input_layer_index], ")");
    }
}

Array NNNode::get_outputs() const {
    Array result;
    for (size_t i = 0; i < output_values.size(); i++) {
        result.append(output_values[i]);
    }
    return result;
}

void NNNode::compute() {
    forward_propagation();
}
