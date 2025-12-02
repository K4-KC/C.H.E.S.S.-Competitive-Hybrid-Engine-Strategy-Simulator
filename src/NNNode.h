#ifndef NNNODE_H
#define NNNODE_H

#include <godot_cpp/classes/node2d.hpp>
#include <vector>

namespace godot {

class NNNode : public Node2D {
    GDCLASS(NNNode, Node2D)

    private:
        std::vector<int> layer_sizes; // [0] = output, [last] = input
        std::vector<std::vector<std::vector<double>>> weights; // weights[i][neuron][weight]
        std::vector<std::vector<double>> biases; // biases[i][neuron]
        std::vector<std::vector<double>> activations; // activations[i][neuron]
        std::vector<std::vector<bool>> computed; // Track if neuron is computed
        std::vector<double> input_values;
        std::vector<double> output_values;
        bool network_initialized;

        // Neural network functions
        double sigmoid(double x);
        void initialize_network();
        
        // Recursive neuron-level forward propagation
        double compute_neuron_recursive(int layer, int neuron);
        void forward_propagation();

    protected:
        static void _bind_methods();

    public:
        NNNode();
        ~NNNode();

        void _ready() override;

        // Setters and getters for GDScript
        void set_layer_sizes(const Array& sizes);
        Array get_layer_sizes() const;
        
        void set_inputs(const Array& inputs);
        Array get_outputs() const;
        
        void compute();
    };

}

#endif
