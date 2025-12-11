#include "neural_net.h"

// Godot includes for registration, engine state, and logging.
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Standard headers for math, randomness, and time seeding.
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace godot;

// Expose methods and properties to GDScript.
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

	// Expose layer sizes and learning rate as editable properties.
	ClassDB::add_property(
		"NeuralNet",
		PropertyInfo(Variant::ARRAY, "layer_sizes"),
		"set_layer_sizes",
		"get_layer_sizes"
	);
	ClassDB::add_property(
		"NeuralNet",
		PropertyInfo(Variant::FLOAT, "learning_rate"),
		"set_learning_rate",
		"get_learning_rate"
	);
}

// Initialize default state, but do not build the network yet.
NeuralNet::NeuralNet() {
	network_initialized = false;
	learning_rate = 0.1;
	srand(time(nullptr)); // Seed RNG for random weights/biases.
}

NeuralNet::~NeuralNet() {}

// Optionally auto-compute once when running the game (not in editor).
void NeuralNet::_ready() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		if (network_initialized && input_values.size() > 0) {
			compute();
		}
	}
}

// Standard sigmoid activation.
double NeuralNet::sigmoid(double x) {
	return 1.0 / (1.0 + std::exp(-x));
}

// Derivative of sigmoid, given already-activated value.
double NeuralNet::sigmoid_derivative(double activated_value) {
	return activated_value * (1.0 - activated_value);
}

// Allocate and randomize weights/biases based on layer_sizes.
void NeuralNet::initialize_network() {
	if (layer_sizes.size() < 2) {
		UtilityFunctions::print("Error: Need at least 2 layers");
		return;
	}

	weights.clear();
	biases.clear();
	activations.clear();

	// Prepare activations for each layer (all zeros initially).
	activations.resize(layer_sizes.size());
	for (size_t i = 0; i < layer_sizes.size(); i++) {
		activations[i].resize(layer_sizes[i], 0.0);
	}

	// For each pair of consecutive layers, create a weight matrix and bias vector.
	for (size_t layer = 1; layer < layer_sizes.size(); layer++) {
		int current_layer_size = layer_sizes[layer];
		int previous_layer_size = layer_sizes[layer - 1];

		std::vector<std::vector<double>> layer_weights;
		for (int neuron = 0; neuron < current_layer_size; neuron++) {
			std::vector<double> neuron_weights;
			for (int weight = 0; weight < previous_layer_size; weight++) {
				// Random weight in range [-1, 1].
				double random_weight = ((double)std::rand() / RAND_MAX) * 2.0 - 1.0;
				neuron_weights.push_back(random_weight);
			}
			layer_weights.push_back(neuron_weights);
		}
		weights.push_back(layer_weights);

		std::vector<double> layer_biases;
		for (int neuron = 0; neuron < current_layer_size; neuron++) {
			// Random bias in range [-1, 1].
			double random_bias = ((double)std::rand() / RAND_MAX) * 2.0 - 1.0;
			layer_biases.push_back(random_bias);
		}
		biases.push_back(layer_biases);
	}

	network_initialized = true;
}

// Forward pass: write inputs into layer 0 and propagate through all layers.
void NeuralNet::forward_propagation() {
	if (!network_initialized) {
		return;
	}

	// Load input values into first layer's activations (truncate if oversized).
	for (size_t i = 0; i < input_values.size() && i < activations[0].size(); i++) {
		activations[0][i] = input_values[i];
	}

	// Compute activations layer by layer using weights, biases, and sigmoid.
	for (size_t layer = 1; layer < layer_sizes.size(); layer++) {
		for (int neuron = 0; neuron < layer_sizes[layer]; neuron++) {
			double sum = biases[layer - 1][neuron];

			for (int prev_neuron = 0; prev_neuron < layer_sizes[layer - 1]; prev_neuron++) {
				sum += activations[layer - 1][prev_neuron] *
					weights[layer - 1][neuron][prev_neuron];
			}

			activations[layer][neuron] = sigmoid(sum);
		}
	}

	// Last layer activations are the output values.
	output_values = activations[layer_sizes.size() - 1];
}

// Single-sample training using backpropagation and gradient descent.
void NeuralNet::train(const Array &inputs, const Array &expected_outputs) {
	if (!network_initialized) {
		return;
	}

	set_inputs(inputs);
	forward_propagation();

	if (output_values.size() != (size_t)expected_outputs.size()) {
		UtilityFunctions::print("Error: Output size mismatch");
		return;
	}

	// Compute output layer deltas from error and activation derivative.
	std::vector<double> next_layer_deltas;
	for (size_t i = 0; i < output_values.size(); i++) {
		double output = output_values[i];
		double target = (double)expected_outputs[i];
		double error_term = (target - output);
		double delta = error_term * sigmoid_derivative(output);
		next_layer_deltas.push_back(delta);
	}

	// Backpropagate through all weight layers from last to first.
	for (int i = (int)weights.size() - 1; i >= 0; i--) {
		int current_layer_idx = i;
		int next_layer_idx = i + 1;

		int current_layer_size = layer_sizes[current_layer_idx];
		int next_layer_size = layer_sizes[next_layer_idx];

		std::vector<double> current_layer_deltas;

		// Compute deltas for the current layer (except for input layer).
		if (i > 0) {
			current_layer_deltas.resize(current_layer_size, 0.0);

			for (int k = 0; k < current_layer_size; k++) {
				double error_sum = 0.0;
				for (int j = 0; j < next_layer_size; j++) {
					error_sum += next_layer_deltas[j] * weights[i][j][k];
				}
				current_layer_deltas[k] =
					error_sum * sigmoid_derivative(activations[current_layer_idx][k]);
			}
		}

		// Gradient descent update for biases and weights for this weight layer.
		for (int j = 0; j < next_layer_size; j++) {
			double delta = next_layer_deltas[j];

			// Bias gradient is just the delta.
			biases[i][j] += learning_rate * delta;

			// Weight gradient uses activations of previous layer.
			for (int k = 0; k < current_layer_size; k++) {
				weights[i][j][k] += learning_rate * delta * activations[current_layer_idx][k];
			}
		}

		// Move one layer backwards.
		next_layer_deltas = current_layer_deltas;
	}
}

// Compute mean-squared-error style cost for a given sample.
double NeuralNet::get_cost(const Array &inputs, const Array &expected_outputs) {
	if (!network_initialized) {
		return 0.0;
	}

	set_inputs(inputs);
	forward_propagation();

	double total_squared_error = 0.0;
	for (size_t i = 0; i < output_values.size(); i++) {
		double diff = (double)expected_outputs[i] - output_values[i];
		total_squared_error += diff * diff;
	}

	// 0.5 factor is standard for convenient derivative (d/dx of 0.5*x^2 = x).
	return 0.5 * total_squared_error;
}

// Simple setters/getters for learning rate.
void NeuralNet::set_learning_rate(double rate) {
	learning_rate = rate;
}

double NeuralNet::get_learning_rate() const {
	return learning_rate;
}

// Configure network topology from a Godot Array and rebuild the network.
void NeuralNet::set_layer_sizes(const Array &sizes) {
	layer_sizes.clear();
	for (int i = 0; i < sizes.size(); i++) {
		layer_sizes.push_back((int)sizes[i]);
	}
	initialize_network();
}

// Return layer_sizes as a Godot Array.
Array NeuralNet::get_layer_sizes() const {
	Array result;
	for (size_t i = 0; i < layer_sizes.size(); i++) {
		result.append(layer_sizes[i]);
	}
	return result;
}

// Convert Godot Array inputs into internal double vector.
void NeuralNet::set_inputs(const Array &inputs) {
	input_values.clear();
	for (int i = 0; i < inputs.size(); i++) {
		input_values.push_back((double)inputs[i]);
	}
}

// Export current output_values to Godot Array.
Array NeuralNet::get_outputs() const {
	Array result;
	for (size_t i = 0; i < output_values.size(); i++) {
		result.append(output_values[i]);
	}
	return result;
}

// Public method to trigger a forward pass.
void NeuralNet::compute() {
	forward_propagation();
}
