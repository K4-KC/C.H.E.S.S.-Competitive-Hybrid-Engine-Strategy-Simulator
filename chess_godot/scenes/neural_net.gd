extends NeuralNet

func _ready():
	var nn = NeuralNet.new()
	add_child(nn)
	
	# Create a simple 2-3-1 network (2 inputs, 3 hidden, 1 output)
	nn.set_layer_sizes([2, 3, 3, 2])
	
	# Test with some inputs
	nn.set_inputs([0.5, 0.3])
	nn.compute()
	
	var output = nn.get_outputs()
	print("Network output: ", output[0])
