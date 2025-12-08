#include "neural_net.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <math.h>
#include <time.h>

using namespace godot;

void NeuralNet::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_layer_sizes", "sizes"), &NeuralNet::set_layer_sizes);
    ClassDB::bind_method(D_METHOD("get_layer_sizes"), &NeuralNet::get_layer_sizes);
    ClassDB::bind_method(D_METHOD("set_inputs", "inputs"), &NeuralNet::set_inputs);
    ClassDB::bind_method(D_METHOD("get_outputs"), &NeuralNet::get_outputs);
    ClassDB::bind_method(D_METHOD("compute"), &NeuralNet::compute);
    
    ClassDB::bind_method(D_METHOD("train", "inputs", "expected_outputs"), &NeuralNet::train);
    ClassDB::bind_method(D_METHOD("get_cost", "inputs", "expected_outputs"), &NeuralNet::get_cost);
    ClassDB::bind_method(D_METHOD("set_learning_rate", "rate"), &NeuralNet::set_learning_rate);
    ClassDB::bind_method(D_METHOD("get_learning_rate"), &NeuralNet::get_learning_rate);

    ClassDB::add_property("NeuralNet", PropertyInfo(Variant::ARRAY, "layer_sizes"), "set_layer_sizes", "get_layer_sizes");
    ClassDB::add_property("NeuralNet", PropertyInfo(Variant::FLOAT, "learning_rate"), "set_learning_rate", "get_learning_rate");
}

NeuralNet::NeuralNet() {
    network_initialized = false;
    learning_rate = 0.1;
    srand(time(nullptr));
}

NeuralNet::~NeuralNet() {}

void NeuralNet::_ready() {
    if (!Engine::get_singleton()->is_editor_hint()) {
        if (network_initialized && input_values.size() > 0) {
            compute();
        }
    }
}

double NeuralNet::sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double NeuralNet::sigmoid_derivative(double activated_value) {
    return activated_value * (1.0 - activated_value);
}

void NeuralNet::initialize_network() {
    if (layer_sizes.size() < 2) {
        UtilityFunctions::print("Error: Need at least 2 layers");
        return;
    }

    weights.clear();
    biases.clear();
    activations.clear();

    activations.resize(layer_sizes.size());
    for (size_t i = 0; i < layer_sizes.size(); i++) {
        activations[i].resize(layer_sizes[i], 0.0);
    }

    for (size_t layer = 1; layer < layer_sizes.size(); layer++) {
        int current_layer_size = layer_sizes[layer];
        int previous_layer_size = layer_sizes[layer - 1];

        std::vector<std::vector<double>> layer_weights;
        for (int neuron = 0; neuron < current_layer_size; neuron++) {
            std::vector<double> neuron_weights;
            for (int weight = 0; weight < previous_layer_size; weight++) {
                double random_weight = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                neuron_weights.push_back(random_weight);
            }
            layer_weights.push_back(neuron_weights);
        }
        weights.push_back(layer_weights);

        std::vector<double> layer_biases;
        for (int neuron = 0; neuron < current_layer_size; neuron++) {
            double random_bias = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            layer_biases.push_back(random_bias);
        }
        biases.push_back(layer_biases);
    }

    network_initialized = true;
}

void NeuralNet::forward_propagation() {
    if (!network_initialized) return;

    for (size_t i = 0; i < input_values.size() && i < activations[0].size(); i++) {
        activations[0][i] = input_values[i];
    }

    for (size_t layer = 1; layer < layer_sizes.size(); layer++) {
        for (int neuron = 0; neuron < layer_sizes[layer]; neuron++) {
            double sum = biases[layer - 1][neuron];
            for (int prev_neuron = 0; prev_neuron < layer_sizes[layer - 1]; prev_neuron++) {
                sum += activations[layer - 1][prev_neuron] * weights[layer - 1][neuron][prev_neuron];
            }
            activations[layer][neuron] = sigmoid(sum);
        }
    }
    output_values = activations[layer_sizes.size() - 1];
}

void NeuralNet::train(const Array &inputs, const Array &expected_outputs) {
    if (!network_initialized) return;

    set_inputs(inputs);
    forward_propagation();

    if (output_values.size() != (size_t)expected_outputs.size()) {
        UtilityFunctions::print("Error: Output size mismatch");
        return;
    }
    
    std::vector<double> next_layer_deltas; 

    for (size_t i = 0; i < output_values.size(); i++) {
        double output = output_values[i];
        double target = (double)expected_outputs[i];
        
        double error_term = (target - output);
        double delta = error_term * sigmoid_derivative(output);
        
        next_layer_deltas.push_back(delta);
    }

    for (int i = weights.size() - 1; i >= 0; i--) {
        int current_layer_idx = i;
        int next_layer_idx = i + 1;
        int current_layer_size = layer_sizes[current_layer_idx];
        int next_layer_size = layer_sizes[next_layer_idx];

        std::vector<double> current_layer_deltas;
        
        if (i > 0) {
            current_layer_deltas.resize(current_layer_size, 0.0);
            for (int k = 0; k < current_layer_size; k++) {
                double error_sum = 0.0;
                for (int j = 0; j < next_layer_size; j++) {
                    error_sum += next_layer_deltas[j] * weights[i][j][k];
                }
                current_layer_deltas[k] = error_sum * sigmoid_derivative(activations[current_layer_idx][k]);
            }
        }

        for (int j = 0; j < next_layer_size; j++) {
            double delta = next_layer_deltas[j];
            biases[i][j] += learning_rate * delta;
            
            for (int k = 0; k < current_layer_size; k++) {
                weights[i][j][k] += learning_rate * delta * activations[current_layer_idx][k];
            }
        }
        next_layer_deltas = current_layer_deltas;
    }
}

double NeuralNet::get_cost(const Array &inputs, const Array &expected_outputs) {
    if (!network_initialized) return 0.0;
    
    set_inputs(inputs);
    forward_propagation();

    double total_squared_error = 0.0;
    for (size_t i = 0; i < output_values.size(); i++) {
        double diff = (double)expected_outputs[i] - output_values[i];
        total_squared_error += diff * diff;
    }
    
    return 0.5 * total_squared_error;
}

void NeuralNet::set_learning_rate(double rate) { learning_rate = rate; }
double NeuralNet::get_learning_rate() const { return learning_rate; }

void NeuralNet::set_layer_sizes(const Array &sizes) {
    layer_sizes.clear();
    for (int i = 0; i < sizes.size(); i++) layer_sizes.push_back((int)sizes[i]);
    initialize_network();
}
Array NeuralNet::get_layer_sizes() const {
    Array result;
    for (size_t i = 0; i < layer_sizes.size(); i++) result.append(layer_sizes[i]);
    return result;
}
void NeuralNet::set_inputs(const Array &inputs) {
    input_values.clear();
    for (int i = 0; i < inputs.size(); i++) input_values.push_back((double)inputs[i]);
}
Array NeuralNet::get_outputs() const {
    Array result;
    for (size_t i = 0; i < output_values.size(); i++) result.append(output_values[i]);
    return result;
}
void NeuralNet::compute() { forward_propagation(); }
