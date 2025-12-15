# Main chess board scene: handles user input, piece sprites, highlights, and pawn promotion.
extends Node2D

const TILE_SIZE = 16
const BOARD_OFFSET = Vector2(8, 8)

var board_rules: BoardRules
var sprites = {} # Map<Vector2i, Sprite2D>
var selected_pos = null # Vector2i

# Temporary visual helpers for current move and last move.
var highlight_sprites = []
var last_move_sprites = []

# Highlight textures used to show possible moves, captures, and last move.
var tex_move = preload("res://assets/move.png")
var tex_capture = preload("res://assets/capture.png")
var tex_moving = preload("res://assets/moving.png")
var tex_moved = preload("res://assets/moved.png")

# Piece textures indexed by color (0/1) and type ("p","r","n","b","q","k").
var textures = {
	0: { "p": preload("res://assets/p0.png"), "r": preload("res://assets/r0.png"), "n": preload("res://assets/n0.png"), 
		 "b": preload("res://assets/b0.png"), "q": preload("res://assets/q0.png"), "k": preload("res://assets/k0.png") },
	1: { "p": preload("res://assets/p1.png"), "r": preload("res://assets/r1.png"), "n": preload("res://assets/n1.png"), 
		 "b": preload("res://assets/b1.png"), "q": preload("res://assets/q1.png"), "k": preload("res://assets/k1.png") }
}

# UI nodes and state tracking for pawn promotion flow.
var promotion_panel = null
var promotion_buttons = {} 
var is_promoting = false
var pending_promotion_move_start = null # Store move details to highlight after promotion
var pending_promotion_move_end = null

func _ready():
	# Two child containers: one for highlights, one for piece sprites.
	var hl_node = Node2D.new()
	hl_node.name = "Highlights"
	add_child(hl_node)
	
	var pieces_node = Node2D.new()
	pieces_node.name = "Pieces"
	add_child(pieces_node)

	# Create rules object and set up initial board.
	board_rules = ClassDB.instantiate("BoardRules")
	add_child(board_rules)
	
	setup_ui()
	board_rules.setup_board([]) 
	refresh_visuals()

# Build a simple promotion UI: 4 buttons centered over the board.
func setup_ui():
	var canvas = CanvasLayer.new()
	add_child(canvas)
	promotion_panel = PanelContainer.new()
	promotion_panel.visible = false
	promotion_panel.anchors_preset = Control.PRESET_CENTER 
	promotion_panel.position = Vector2(10, 52) 
	canvas.add_child(promotion_panel)
	var hbox = HBoxContainer.new()
	promotion_panel.add_child(hbox)
	var options = ["q", "r", "b", "n"]
	for type in options:
		var btn = Button.new()
		btn.custom_minimum_size = Vector2(16, 16)
		btn.pressed.connect(self._on_promotion_selected.bind(type))
		hbox.add_child(btn)
		promotion_buttons[type] = btn

# Handle mouse input for selecting pieces, making moves, and triggering promotion.
func _input(event):
	if is_promoting: return

	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var clicked_pos = pixel_to_grid(event.position)
		if not is_on_board(clicked_pos): return

		if selected_pos != null:
			if clicked_pos == selected_pos:
				deselect_piece()
			else:
				var valid_targets = board_rules.get_valid_moves_for_piece(selected_pos)
				
				if clicked_pos in valid_targets:
					var move_start = selected_pos
					var move_end = clicked_pos
					
					var result = board_rules.attempt_move(selected_pos, clicked_pos)
					if result == 1:
						update_last_move_visuals(move_start, move_end)
						deselect_piece()
						refresh_visuals()
					elif result == 2:
						# Move leads to promotion: remember for later highlight and open UI.
						pending_promotion_move_start = move_start
						pending_promotion_move_end = move_end
						
						refresh_visuals()

						start_promotion(board_rules.get_data_at(clicked_pos.x, clicked_pos.y))
				else:
					var p = board_rules.get_data_at(clicked_pos.x, clicked_pos.y)
					if not p.is_empty() and p.color == board_rules.get_turn():
						select_piece(clicked_pos)
					else:
						deselect_piece()
		else:
			var p = board_rules.get_data_at(clicked_pos.x, clicked_pos.y)
			if not p.is_empty() and p.color == board_rules.get_turn():
				select_piece(clicked_pos)

# Select a piece and spawn move/capture highlights for its legal moves.
func select_piece(pos: Vector2i):
	deselect_piece()
	selected_pos = pos
	
	spawn_highlight(tex_moving, pos)
	
	var valid_moves = board_rules.get_valid_moves_for_piece(pos)
	
	for target in valid_moves:
		var target_data = board_rules.get_data_at(target.x, target.y)
		var is_capture = not target_data.is_empty()
			
		if is_capture:
			spawn_highlight(tex_capture, target)
		else:
			spawn_highlight(tex_move, target)

func deselect_piece():
	selected_pos = null
	clear_temp_highlights()

func spawn_highlight(texture, grid_pos):
	var s = Sprite2D.new()
	s.texture = texture
	s.position = grid_to_pixel(Vector2(grid_pos.x, grid_pos.y))
	$Highlights.add_child(s)
	highlight_sprites.append(s)

func clear_temp_highlights():
	for s in highlight_sprites:
		s.queue_free()
	highlight_sprites.clear()

# Store and render the last move as two highlighted squares.
func update_last_move_visuals(start: Vector2i, end: Vector2i):
	for s in last_move_sprites:
		s.queue_free()
	last_move_sprites.clear()
	
	var s1 = Sprite2D.new()
	s1.texture = tex_moved
	s1.position = grid_to_pixel(Vector2(start.x, start.y))
	$Highlights.add_child(s1)
	last_move_sprites.append(s1)
	
	var s2 = Sprite2D.new()
	s2.texture = tex_moved
	s2.position = grid_to_pixel(Vector2(end.x, end.y))
	$Highlights.add_child(s2)
	last_move_sprites.append(s2)

# Sync Sprite2D pieces with current board state from BoardRules.
func refresh_visuals():
	var active_positions = []
	for x in range(8):
		for y in range(8):
			var data = board_rules.get_data_at(x, y)
			var pos = Vector2i(x, y)
			if data.is_empty():
				if sprites.has(pos):
					sprites[pos].queue_free()
					sprites.erase(pos)
			else:
				active_positions.append(pos)
				var type = data["type"]
				var color = data["color"]
				if sprites.has(pos):
					sprites[pos].texture = textures[color][type]
				else:
					var s = Sprite2D.new()
					s.texture = textures[color][type]
					s.position = grid_to_pixel(Vector2(x, y))
					$Pieces.add_child(s)
					sprites[pos] = s
				sprites[pos].position = grid_to_pixel(Vector2(x, y))

# Show promotion panel and set icons based on the pawn's color.
func start_promotion(piece_data):
	is_promoting = true
	var color = piece_data["color"]
	for type in promotion_buttons:
		promotion_buttons[type].icon = textures[color][type]
	promotion_panel.visible = true
	clear_temp_highlights()

# Callback for when a promotion piece is chosen in the UI.
func _on_promotion_selected(type):
	board_rules.commit_promotion(type)
	promotion_panel.visible = false
	is_promoting = false
	
	if pending_promotion_move_start != null and pending_promotion_move_end != null:
		update_last_move_visuals(pending_promotion_move_start, pending_promotion_move_end)
		pending_promotion_move_start = null
		pending_promotion_move_end = null
	
	deselect_piece()
	refresh_visuals()

# Coordinate helpers between grid (0–7) and pixel space.
func grid_to_pixel(grid_pos): return grid_pos * TILE_SIZE + BOARD_OFFSET
func pixel_to_grid(pixel_pos): return Vector2i(floor(pixel_pos.x / TILE_SIZE), floor(pixel_pos.y / TILE_SIZE))
func is_on_board(pos): return pos.x >= 0 and pos.x < 8 and pos.y >= 0 and pos.y < 8
