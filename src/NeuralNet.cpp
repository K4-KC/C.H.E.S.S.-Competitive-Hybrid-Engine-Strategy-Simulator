#include "NeuralNet.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace godot;

void NeuralNet::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_layer_sizes", "sizes"), &NeuralNet::set_layer_sizes);
    ClassDB::bind_method(D_METHOD("get_layer_sizes"), &NeuralNet::get_layer_sizes);

    ClassDB::bind_method(D_METHOD("set_inputs", "inputs"), &NeuralNet::set_inputs);
    ClassDB::bind_method(D_METHOD("get_outputs"), &NeuralNet::get_outputs);

    ClassDB::bind_method(D_METHOD("compute"), &NeuralNet::compute);

    ClassDB::add_property("NeuralNet",
                          PropertyInfo(Variant::ARRAY, "layer_sizes"),
                          "set_layer_sizes",
                          "get_layer_sizes");
}

NeuralNet::NeuralNet()
{
    network_initialized = false;
    srand(time(nullptr));
}

NeuralNet::~NeuralNet()
{
}

void NeuralNet::_ready()
{
    if (!Engine::get_singleton()->is_editor_hint())
    {
        if (network_initialized && input_values.size() > 0)
        {
            compute();
            UtilityFunctions::print("Neural Network Output:");
            for (size_t i = 0; i < output_values.size(); i++)
            {
                UtilityFunctions::print("  Output[", (int)i, "] = ", output_values[i]);
            }
        }
    }
}

double NeuralNet::sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

void NeuralNet::initialize_network()
{
    if (layer_sizes.size() < 2)
    {
        UtilityFunctions::print("Error: Need at least 2 layers");
        return;
    }

    weights.clear();
    biases.clear();
    activations.clear();

    // Initialize activations for all layers
    activations.resize(layer_sizes.size());
    for (size_t i = 0; i < layer_sizes.size(); i++)
    {
        activations[i].resize(layer_sizes[i], 0.0);
    }

    // Initialize weights and biases for hidden and output layers
    // No weights for input layer
    for (size_t layer = 1; layer < layer_sizes.size(); layer++)
    {
        int current_layer_size = layer_sizes[layer];
        int previous_layer_size = layer_sizes[layer - 1];

        // Initialize weights for this layer
        std::vector<std::vector<double>> layer_weights;
        for (int neuron = 0; neuron < current_layer_size; neuron++)
        {
            std::vector<double> neuron_weights;
            for (int weight = 0; weight < previous_layer_size; weight++)
            {
                // Random weight between -1 and 1
                double random_weight = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                neuron_weights.push_back(random_weight);
            }
            layer_weights.push_back(neuron_weights);
        }
        weights.push_back(layer_weights);

        // Initialize biases for this layer
        std::vector<double> layer_biases;
        for (int neuron = 0; neuron < current_layer_size; neuron++)
        {
            double random_bias = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            layer_biases.push_back(random_bias);
        }
        biases.push_back(layer_biases);
    }

    network_initialized = true;
    UtilityFunctions::print("Neural Network initialized with architecture:");
    for (size_t i = 0; i < layer_sizes.size(); i++)
    {
        UtilityFunctions::print("  Layer ", (int)i, ": ", layer_sizes[i], " neurons");
    }
}

void NeuralNet::forward_propagation()
{
    if (!network_initialized)
    {
        UtilityFunctions::print("Error: Network not initialized");
        return;
    }

    // Set input layer activations
    for (size_t i = 0; i < input_values.size() && i < activations[0].size(); i++)
    {
        activations[0][i] = input_values[i];
    }

    // Propagate through each layer
    for (size_t layer = 1; layer < layer_sizes.size(); layer++)
    {
        for (int neuron = 0; neuron < layer_sizes[layer]; neuron++)
        {
            double sum = biases[layer - 1][neuron];

            // Sum weighted inputs from previous layer
            for (int prev_neuron = 0; prev_neuron < layer_sizes[layer - 1]; prev_neuron++)
            {
                sum += activations[layer - 1][prev_neuron] *
                       weights[layer - 1][neuron][prev_neuron];
            }

            // Apply sigmoid activation function
            activations[layer][neuron] = sigmoid(sum);
        }
    }

    // Store output layer activations
    output_values = activations[layer_sizes.size() - 1];
}

void NeuralNet::set_layer_sizes(const Array &sizes)
{
    layer_sizes.clear();
    for (int i = 0; i < sizes.size(); i++)
    {
        layer_sizes.push_back((int)sizes[i]);
    }
    initialize_network();
}

Array NeuralNet::get_layer_sizes() const
{
    Array result;
    for (size_t i = 0; i < layer_sizes.size(); i++)
    {
        result.append(layer_sizes[i]);
    }
    return result;
}

void NeuralNet::set_inputs(const Array &inputs)
{
    input_values.clear();
    for (int i = 0; i < inputs.size(); i++)
    {
        input_values.push_back((double)inputs[i]);
    }

    if (network_initialized && input_values.size() != (size_t)layer_sizes[0])
    {
        UtilityFunctions::print("Warning: Input size (", (int)input_values.size(),
                                ") doesn't match input layer size (", layer_sizes[0], ")");
    }
}

Array NeuralNet::get_outputs() const
{
    Array result;
    for (size_t i = 0; i < output_values.size(); i++)
    {
        result.append(output_values[i]);
    }
    return result;
}

void NeuralNet::compute()
{
    forward_propagation();
}
