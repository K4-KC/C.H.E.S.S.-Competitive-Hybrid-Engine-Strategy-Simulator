# Chess game with a simple AI opponent, extending the base board/UX from chess_game.gd.
extends "res://scenes/chess_game.gd"

const AI_COLOR = 1
var chess_agent = null

func _ready():
	# Recreate visual containers (highlights and pieces) for this scene.
	var hl_node = Node2D.new()
	hl_node.name = "Highlights"
	add_child(hl_node)
	var pieces_node = Node2D.new()
	pieces_node.name = "Pieces"
	add_child(pieces_node)

	# Initialize rules and optional C++ ChessAgent module.
	board_rules = ClassDB.instantiate("BoardRules")
	add_child(board_rules)
	
	if ClassDB.class_exists("ChessAgent"):
		chess_agent = ClassDB.instantiate("ChessAgent")
		add_child(chess_agent)
		print("C++ ChessAgent initialized.")
	else:
		printerr("CRITICAL: ChessAgent class missing.")

	setup_ui()
	board_rules.setup_board([]) 
	refresh_visuals()

# Player input is disabled while it is the AI's turn.
func _input(event):
	if board_rules.get_turn() == AI_COLOR:
		return
	super._input(event)

# After every visual update, schedule an AI move if it is AI's turn.
func refresh_visuals():
	super.refresh_visuals()
	if board_rules.get_turn() == AI_COLOR:
		call_deferred("perform_ai_turn")

# Ask the agent for the best move using the pre-calculated board states.
func perform_ai_turn():
	# get_all_possible_moves now returns an array of Dictionaries, each containing:
	# { "start": Vector2, "end": Vector2, "board": Array[Array[Dictionary]], "promotion": String }
	var possible_moves = board_rules.get_all_possible_moves(AI_COLOR)
	
	if possible_moves.is_empty():
		print("AI has no moves.")
		return

	# We no longer need to generate a 'simple_board' here.
	# The agent receives the full list of future states directly.
	var best_move = chess_agent.select_best_move(possible_moves)

	if best_move.has("start") and best_move.has("end"):
		var start = best_move["start"]
		var end = best_move["end"]
		
		var result = board_rules.attempt_move(start, end)
		
		if result == 2:
			# AI Promotion
			# Use the promotion type decided by the AI, or default to Queen if missing
			var promo = "q"
			if best_move.has("promotion"):
				promo = best_move["promotion"]
			
			board_rules.commit_promotion(promo)
			
			# For AI, we must explicitly update visuals here because the UI callback isn't used
			update_last_move_visuals(start, end)
			refresh_visuals()
		elif result == 1:
			# Normal Move
			update_last_move_visuals(start, end)
			refresh_visuals()
