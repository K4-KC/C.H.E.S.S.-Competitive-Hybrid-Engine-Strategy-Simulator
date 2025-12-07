extends Node2D

func _ready():
	var nn = NeuralNet.new()
	add_child(nn)
	
	# 1. Setup: 1 Input -> 3 Hidden Neurons -> 1 Output
	nn.set_layer_sizes([1, 3, 1])
	nn.set_learning_rate(0.2)
	
	# The training data: Input 1.0 should result in Output 0.5
	var input_data = [1.0]
	var target_output = [0.1]
	
	print("--- Starting Training ---")
	print("Initial Output: ", get_current_prediction(nn, input_data))

	# 2. Training Loop
	# We loop multiple times to let the gradient descent minimize the error
	for i in range(200):
		nn.train(input_data, target_output)
		
		# Optional: Print progress every 1000 steps
		if i % 10 == 0:
			var cost = nn.get_cost(input_data, target_output)
			print("Epoch ", i, " | Cost: ", cost)

	# 3. Verification
	print("--- Training Finished ---")
	var final_output = get_current_prediction(nn, input_data)
	print("Final Output: ", final_output)
	print("Target: ", target_output[0])

func get_current_prediction(nn, input):
	nn.set_inputs(input)
	nn.compute()
	return nn.get_outputs()[0]
