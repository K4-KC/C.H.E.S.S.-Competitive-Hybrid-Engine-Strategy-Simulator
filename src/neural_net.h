#ifndef NEURALNET_H
#define NEURALNET_H

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <vector>

namespace godot {

class NeuralNet : public Node2D {
    GDCLASS(NeuralNet, Node2D)

private:
    std::vector<int> layer_sizes;
    std::vector<std::vector<std::vector<double>>> weights; // [layer][neuron][weight]
    std::vector<std::vector<double>> biases;               // [layer][neuron]
    std::vector<std::vector<double>> activations;          // [layer][neuron]
    std::vector<double> input_values;
    std::vector<double> output_values;
    
    double learning_rate;
    bool network_initialized;

    double sigmoid(double x);
    double sigmoid_derivative(double x);
    void initialize_network();
    void forward_propagation();

protected:
    static void _bind_methods();

public:
    NeuralNet();
    ~NeuralNet();

    void _ready() override;

    void set_layer_sizes(const Array &sizes);
    Array get_layer_sizes() const;

    void set_inputs(const Array &inputs);
    Array get_outputs() const;

    void compute();

    void set_learning_rate(double rate);
    double get_learning_rate() const;
    void train(const Array &inputs, const Array &expected_outputs);
    
    double get_cost(const Array &inputs, const Array &expected_outputs); 
};

}

#endif
