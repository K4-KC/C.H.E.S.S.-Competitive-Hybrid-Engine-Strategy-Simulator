extends "res://scenes/chess_game.gd"

const AI_COLOR = 1
var agent_module = null

func _ready():
	# Standard setup
	var hl_node = Node2D.new()
	hl_node.name = "Highlights"
	add_child(hl_node)
	var pieces_node = Node2D.new()
	pieces_node.name = "Pieces"
	add_child(pieces_node)

	board_rules = BoardRules.new()
	add_child(board_rules)
	
	if ClassDB.class_exists("ChessAgent"):
		agent_module = ClassDB.instantiate("ChessAgent")
		add_child(agent_module)
		print("C++ ChessAgent initialized.")
	else:
		printerr("CRITICAL: ChessAgent class missing.")

	setup_ui()
	board_rules.setup_board([]) 
	refresh_visuals()

func _input(event):
	if board_rules.get_turn() == AI_COLOR:
		return
	super._input(event)

func refresh_visuals():
	super.refresh_visuals()
	if board_rules.get_turn() == AI_COLOR:
		call_deferred("perform_ai_turn")

func perform_ai_turn():
	var possible_moves = board_rules.get_all_possible_moves(AI_COLOR)
	if possible_moves.is_empty():
		print("AI has no moves.")
		return

	var simple_board = []
	for y in range(8):
		for x in range(8):
			var data = board_rules.get_data_at(x, y)
			var val = 0
			if not data.is_empty():
				var type_map = {"p": 1, "n": 2, "b": 3, "r": 4, "q": 5, "k": 6}
				var base = type_map.get(data["type"], 0)
				if base > 0:
					val = base + (data["color"] * 6)
			simple_board.append(val)

	var best_move = agent_module.select_best_move(simple_board, possible_moves)

	if best_move.has("start") and best_move.has("end"):
		var start = best_move["start"]
		var end = best_move["end"]
		
		# Execute
		var result = board_rules.attempt_move(start, end)
		
		if result == 2:
			# AI Promotion
			board_rules.commit_promotion("q")
			# For AI, we must explicitly update visuals here because the UI callback isn't used
			update_last_move_visuals(start, end)
			refresh_visuals()
		elif result == 1:
			# Normal Move
			update_last_move_visuals(start, end)
			refresh_visuals()
