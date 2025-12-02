extends Node2D

func _ready():
	var nn = NNNode.new()
	add_child(nn)
	
	# Create a network: [1 output, 3 hidden, 2 inputs]
	nn.set_layer_sizes([1, 3, 2])
	
	# Test with some inputs
	nn.set_inputs([0.5, 0.3])
	nn.compute()
	
	var output = nn.get_outputs()
	print("Network output: ", output[0])
