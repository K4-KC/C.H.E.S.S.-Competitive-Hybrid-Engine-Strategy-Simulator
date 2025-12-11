#ifndef NEURALNET_H
#define NEURALNET_H

// Godot core node and Array types.
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>

// STL containers for internal numeric storage.
#include <vector>

namespace godot {

// Simple fully-connected feedforward neural network as a Godot Node2D.
class NeuralNet : public Node2D {
	GDCLASS(NeuralNet, Node2D)

private:
	// Network topology: number of neurons per layer, including input and output.
	std::vector<int> layer_sizes;

	// Weights and biases:
	// weights[layer][neuron][weight_from_prev_neuron]
	std::vector<std::vector<std::vector<double>>> weights; // [layer][neuron][weight]
	std::vector<std::vector<double>> biases;               // [layer][neuron]

	// Activations[layer][neuron] after forward propagation.
	std::vector<std::vector<double>> activations;          // [layer][neuron]

	// Current input and last-computed output values (as raw doubles).
	std::vector<double> input_values;
	std::vector<double> output_values;

	// Hyper-parameters and state.
	double learning_rate;
	bool network_initialized;

	// Activation function and its derivative.
	double sigmoid(double x);
	double sigmoid_derivative(double activated_value);

	// Internal helpers to create and run the network.
	void initialize_network();
	void forward_propagation();

protected:
	static void _bind_methods();

public:
	NeuralNet();
	~NeuralNet();

	// Called when the node enters the scene tree.
	void _ready() override;

	// Configure the network architecture from a Godot Array of ints.
	void set_layer_sizes(const Array &sizes);
	Array get_layer_sizes() const;

	// Set input values from a Godot Array and read outputs back into an Array.
	void set_inputs(const Array &inputs);
	Array get_outputs() const;

	// Convenience wrapper for forward_propagation from script.
	void compute();

	// Learning rate parameter control.
	void set_learning_rate(double rate);
	double get_learning_rate() const;

	// Perform one training step (backprop) on a single (inputs, expected_outputs) pair.
	void train(const Array &inputs, const Array &expected_outputs);

	// Compute mean-squared-error style cost for a given (inputs, expected_outputs).
	double get_cost(const Array &inputs, const Array &expected_outputs);
};

} // namespace godot

#endif
