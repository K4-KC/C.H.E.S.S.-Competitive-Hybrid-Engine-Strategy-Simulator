extends Node2D

# --- CONFIGURATION ---
const TILE_SIZE = 16
const BOARD_OFFSET = Vector2(8, 8)

# --- DEFAULT POSITION ---
const STANDARD_LAYOUT = [
	["r1","n1","b1","q1","k1","b1","n1","r1"], # y = 0 (Black Home)
	["p1","p1","p1","p1","p1","p1","p1","p1"], # y = 1 (Black Pawns)
	["0", "0", "0", "0", "0", "0", "0", "0"],  # y = 2
	["0", "0", "0", "0", "0", "0", "0", "0"],  # y = 3
	["0", "0", "0", "0", "0", "0", "0", "0"],  # y = 4
	["0", "0", "0", "0", "0", "0", "0", "0"],  # y = 5
	["p0","p0","p0","p0","p0","p0","p0","p0"], # y = 6 (White Pawns)
	["r0","n0","b0","q0","k0","b0","n0","r0"]  # y = 7 (White Home)
]

# --- STATE VARIABLES ---
var board = [] 
var selected_piece = null 
var turn = 0 # 0 = White, 1 = Black
var en_passant_target = null
var is_promoting = false # NEW: Blocks input while choosing promotion

# --- UI VARIABLES ---
var promotion_panel = null
var promotion_buttons = {} # Stores references to the 4 buttons

# Preload Textures
var textures = {
	0: { "p": preload("res://assets/p0.png"), "r": preload("res://assets/r0.png"), "n": preload("res://assets/n0.png"), 
		 "b": preload("res://assets/b0.png"), "q": preload("res://assets/q0.png"), "k": preload("res://assets/k0.png") },
	1: { "p": preload("res://assets/p1.png"), "r": preload("res://assets/r1.png"), "n": preload("res://assets/n1.png"), 
		 "b": preload("res://assets/b1.png"), "q": preload("res://assets/q1.png"), "k": preload("res://assets/k1.png") }
}

func _ready():
	# Initialize 8x8 board
	for x in range(8):
		board.append([])
		for y in range(8):
			board[x].append(null)
			
	setup_ui() # Create the promotion popup
	
	# Use only if using a Custom Level
	var custom_level = []
	
	#custom_level = [
		#["r1",  "0",  "0",  "0", "k1",  "0",  "0",  "r1"], 
		#["0",  "0",  "0",  "0",  "0",  "0",  "0",  "0"],
		#["0",  "0",  "0",  "0",  "0",  "0",  "0",  "0"],
		#["0",  "0",  "r0", "0",  "0",  "0",  "0",  "0"], # White Rook in middle
		#["0",  "0",  "0",  "0",  "0",  "0",  "0",  "0"],
		#["0",  "0",  "0",  "0",  "0",  "0",  "0",  "0"],
		#["0",  "0",  "0",  "0",  "0",  "0",  "0",  "0"],
		#["0",  "0",  "0",  "0", "k0",  "0",  "0",  "0"]
	#]
	
	setup_board(custom_level)

func setup_ui():
	# Create a CanvasLayer to make sure UI is on top
	var canvas = CanvasLayer.new()
	add_child(canvas)
	
	# Create a Panel for the background
	promotion_panel = PanelContainer.new()
	promotion_panel.visible = false
	promotion_panel.anchors_preset = Control.PRESET_CENTER # Center on screen
	promotion_panel.position = Vector2(10, 52) # Fallback position if centering fails in some setups
	canvas.add_child(promotion_panel)
	
	var hbox = HBoxContainer.new()
	promotion_panel.add_child(hbox)
	
	# Create 4 buttons: Queen, Rook, Bishop, Knight
	var options = ["q", "r", "b", "n"]
	for type in options:
		var btn = Button.new()
		btn.custom_minimum_size = Vector2(16, 16)
		# Connect the click signal. We pass the 'type' using bind.
		btn.pressed.connect(self._on_promotion_selected.bind(type))
		hbox.add_child(btn)
		promotion_buttons[type] = btn

func setup_board(custom_layout = []):
	# If no custom layout is provided, use the standard board
	var layout = custom_layout
	if layout.is_empty():
		layout = STANDARD_LAYOUT

	# Iterate through the grid (Row = y, Column = x)
	for y in range(8):
		for x in range(8):
			var cell_data = layout[y][x]
			
			# Check if the cell is not empty ("0")
			if cell_data != "0":
				# Extract type and color from the string (e.g., "r0")
				var type = cell_data[0]        # First character: "r", "k", etc.
				var color = int(cell_data[1])  # Second character: "0" or "1"
				var has_moved = true
				if cell_data == STANDARD_LAYOUT[y][x]: has_moved = false
				
				spawn_piece(x, y, type, color, has_moved)

func spawn_piece(x, y, type, color, has_moved):
	var piece = {
		"type": type,
		"color": color,
		"sprite": Sprite2D.new(),
		"has_moved": has_moved
	}
	piece.sprite.texture = textures[color][type]
	piece.sprite.position = grid_to_pixel(Vector2(x, y))
	$Pieces.add_child(piece.sprite)
	board[x][y] = piece

func _input(event):
	# Block input if promotion popup is open
	if is_promoting: return

	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var clicked_pos = pixel_to_grid(event.position)
		if not is_on_board(clicked_pos): return

		if selected_piece:
			if clicked_pos == pixel_to_grid(selected_piece.sprite.position):
				deselect_piece()
			else:
				handle_move_attempt(selected_piece, clicked_pos)
		else:
			var piece = board[clicked_pos.x][clicked_pos.y]
			if piece and piece.color == turn:
				selected_piece = piece
				selected_piece.sprite.modulate = Color(0.5, 1, 0.5)

func deselect_piece():
	if selected_piece:
		selected_piece.sprite.modulate = Color(1, 1, 1)
	selected_piece = null

# --- CORE LOGIC ---

func handle_move_attempt(piece, target_pos):
	var start_pos = pixel_to_grid(piece.sprite.position)
	
	if not is_valid_geometry(piece, start_pos, target_pos):
		deselect_piece(); return

	if does_move_cause_self_check(piece, start_pos, target_pos):
		print("Illegal move: King would be in check!")
		deselect_piece(); return

	execute_move(piece, start_pos, target_pos)

func execute_move(piece, start, end):
	var captured_piece = board[end.x][end.y]
	var move_direction = -1 if piece.color == 0 else 1
	
	# Handle En Passant Capture
	if piece.type == "p" and end == en_passant_target:
		var pawn_to_kill_pos = Vector2(end.x, end.y - move_direction)
		var pawn_to_kill = board[pawn_to_kill_pos.x][pawn_to_kill_pos.y]
		if pawn_to_kill:
			pawn_to_kill.sprite.queue_free()
			board[pawn_to_kill_pos.x][pawn_to_kill_pos.y] = null

	# Handle Castling
	if piece.type == "k" and abs(end.x - start.x) > 1:
		var rook_x = 7 if end.x > start.x else 0
		var rook_target_x = 5 if end.x > start.x else 3
		var rook = board[rook_x][end.y]
		if rook:
			board[rook_x][end.y] = null
			board[rook_target_x][end.y] = rook
			rook.sprite.position = grid_to_pixel(Vector2(rook_target_x, end.y))
			rook.has_moved = true

	# Update En Passant Logic
	en_passant_target = null
	if piece.type == "p" and abs(end.y - start.y) == 2:
		en_passant_target = Vector2(start.x, start.y + move_direction)

	# Execute Standard Move
	if captured_piece: captured_piece.sprite.queue_free()
	board[start.x][start.y] = null
	board[end.x][end.y] = piece
	piece.sprite.position = grid_to_pixel(end)
	piece.has_moved = true
	
	deselect_piece()
	
	# --- PROMOTION CHECK ---
	# White reaches 0, Black reaches 7
	var promotion_row = 0 if piece.color == 0 else 7
	
	if piece.type == "p" and end.y == promotion_row:
		start_promotion(piece) # Pause and ask user
	else:
		finalize_turn() # Continue normally

# --- PROMOTION LOGIC ---

var piece_being_promoted = null # Store the pawn waiting for upgrade

func start_promotion(piece):
	is_promoting = true
	piece_being_promoted = piece
	
	# Update button icons to match the player's color
	for type in promotion_buttons:
		promotion_buttons[type].icon = textures[piece.color][type]
	
	# Show Popup
	promotion_panel.visible = true

func _on_promotion_selected(type):
	# 1. Update the piece data
	piece_being_promoted.type = type
	# 2. Update the sprite
	piece_being_promoted.sprite.texture = textures[piece_being_promoted.color][type]
	
	# 3. Cleanup UI
	promotion_panel.visible = false
	is_promoting = false
	piece_being_promoted = null
	
	# 4. Finish the turn (Check for checks, switch player)
	finalize_turn()

# --- TURN FINALIZATION ---

func finalize_turn():
	turn = 1 - turn # Switch players
	
	if is_in_check(turn):
		if is_checkmate(turn):
			print("GAME OVER! Checkmate.")
		else:
			print("CHECK!")
	elif is_checkmate(turn):
		print("GAME OVER! Stalemate.")

# --- HELPERS ---
func grid_to_pixel(grid_pos): return grid_pos * TILE_SIZE + BOARD_OFFSET
func pixel_to_grid(pixel_pos): return Vector2(floor(pixel_pos.x / TILE_SIZE), floor(pixel_pos.y / TILE_SIZE))
func is_on_board(pos): return pos.x >= 0 and pos.x < 8 and pos.y >= 0 and pos.y < 8

# --- MOVEMENT & CHECK LOGIC (Same as before) ---

func is_valid_geometry(piece, start, end):
	var dx = end.x - start.x
	var dy = end.y - start.y
	var abs_dx = abs(dx)
	var abs_dy = abs(dy)
	var target_piece = board[end.x][end.y]
	
	if target_piece and target_piece.color == piece.color: return false

	match piece.type:
		"p": 
			var dir = -1 if piece.color == 0 else 1
			if dx == 0 and dy == dir and target_piece == null: return true
			if dx == 0 and dy == dir * 2 and not piece.has_moved and target_piece == null:
				if board[start.x][start.y + dir] == null: return true
			if abs_dx == 1 and dy == dir:
				if target_piece != null: return true
				if end == en_passant_target: return true
			return false
		"k": 
			if abs_dx <= 1 and abs_dy <= 1: return true
			if abs_dy == 0 and abs_dx == 2 and not piece.has_moved:
				if is_square_attacked(start, 1 - piece.color): return false
				var rook_x = 7 if dx > 0 else 0
				var rook = board[rook_x][start.y]
				if rook == null or rook.type != "r" or rook.has_moved: return false
				var step = 1 if dx > 0 else -1
				for i in range(1, 3):
					var check_pos = Vector2(start.x + (i * step), start.y)
					if board[check_pos.x][check_pos.y] != null: return false
					if is_square_attacked(check_pos, 1 - piece.color): return false
				return true
			return false
		"n": return (abs_dx == 2 and abs_dy == 1) or (abs_dx == 1 and abs_dy == 2)
		"r": return (dx == 0 or dy == 0) and is_path_clear(start, end)
		"b": return abs_dx == abs_dy and is_path_clear(start, end)
		"q": return (dx == 0 or dy == 0 or abs_dx == abs_dy) and is_path_clear(start, end)
	return false

func is_path_clear(start, end):
	var dx = sign(end.x - start.x)
	var dy = sign(end.y - start.y)
	var current = start + Vector2(dx, dy)
	while current != end:
		if board[current.x][current.y] != null: return false
		current += Vector2(dx, dy)
	return true

func is_square_attacked(square, by_color):
	for x in range(8):
		for y in range(8):
			var p = board[x][y]
			if p and p.color == by_color:
				if p.type == "p":
					var dir = -1 if p.color == 0 else 1
					if abs(square.x - x) == 1 and (square.y - y) == dir: return true
				elif p.type == "k":
					if abs(square.x - x) <= 1 and abs(square.y - y) <= 1: return true
				elif is_valid_geometry(p, Vector2(x, y), square):
					return true
	return false

func is_in_check(color):
	var king_pos = null
	for x in range(8):
		for y in range(8):
			var p = board[x][y]
			if p and p.type == "k" and p.color == color:
				king_pos = Vector2(x, y); break
	if king_pos: return is_square_attacked(king_pos, 1 - color)
	return false

func does_move_cause_self_check(piece, start, end):
	var original_target = board[end.x][end.y]
	var original_start = board[start.x][start.y]
	board[start.x][start.y] = null
	board[end.x][end.y] = piece
	var in_check = is_in_check(piece.color)
	board[start.x][start.y] = original_start
	board[end.x][end.y] = original_target
	return in_check

func is_checkmate(color):
	for x in range(8):
		for y in range(8):
			var p = board[x][y]
			if p and p.color == color:
				for tx in range(8):
					for ty in range(8):
						var target = Vector2(tx, ty)
						var start = Vector2(x, y)
						if is_valid_geometry(p, start, target):
							if not does_move_cause_self_check(p, start, target):
								return false
	return true
