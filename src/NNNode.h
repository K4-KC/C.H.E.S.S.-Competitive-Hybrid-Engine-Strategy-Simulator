#ifndef NNNODE_H
#define NNNODE_H

#include <godot_cpp/classes/node2d.hpp>

namespace godot
{
    class NNNode : public Node2D
    {
        GDCLASS(NNNode, Node2D)

    private:
        double num;

    protected:
        static void _bind_methods();

    public:
        NNNode();
        ~NNNode();

        void _process(double delta) override;

        // void set_amplitude(const double p_amplitude);
        // double get_amplitude() const;
        void set_num(const double p_num);
        double get_num() const;
        void add_one();
    };

}

#endif
