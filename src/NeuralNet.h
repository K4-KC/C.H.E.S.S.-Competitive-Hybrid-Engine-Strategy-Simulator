#ifndef NEURALNET_H
#define NEURALNET_H

#include <godot_cpp/classes/node2d.hpp>
#include <vector>

namespace godot
{

    class NeuralNet : public Node2D
    {
        GDCLASS(NeuralNet, Node2D)

    private:
        std::vector<int> layer_sizes;
        std::vector<std::vector<std::vector<double>>> weights; // [layer][neuron][weight]
        std::vector<std::vector<double>> biases;               // [layer][neuron]
        std::vector<std::vector<double>> activations;          // [layer][neuron]
        std::vector<double> input_values;
        std::vector<double> output_values;
        bool network_initialized;

        // Neural network functions
        double sigmoid(double x);
        void initialize_network();
        void forward_propagation();

    protected:
        static void _bind_methods();

    public:
        NeuralNet();
        ~NeuralNet();

        void _ready() override;

        // Setters and getters for GDScript
        void set_layer_sizes(const Array &sizes);
        Array get_layer_sizes() const;

        void set_inputs(const Array &inputs);
        Array get_outputs() const;

        void compute();
    };

}

#endif
