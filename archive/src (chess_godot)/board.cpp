#include "board.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring> // For memcpy

using namespace godot;

// Constructor: reset board, turn, and special flags.
Board::Board() {
	turn = 0;
	en_passant_target = Vector2i(-1, -1);
	promotion_pending = false;
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			board[x][y] = { EMPTY, NONE, false, false };
		}
	}
}

Board::~Board() {}

// Set up the board to either standard chess layout or a custom one.
void Board::setup_board(const Array &custom_layout) {
	turn = 0;
	en_passant_target = Vector2i(-1, -1);
	promotion_pending = false;

	bool use_standard = custom_layout.is_empty();

	// Standard initial chess position (color suffix: 0 = white, 1 = black).
	const char *std_layout[8][8] = {
		{ "r1", "n1", "b1", "q1", "k1", "b1", "n1", "r1" },
		{ "p1", "p1", "p1", "p1", "p1", "p1", "p1", "p1" },
		{ "0", "0", "0", "0", "0", "0", "0", "0" },
		{ "0", "0", "0", "0", "0", "0", "0", "0" },
		{ "0", "0", "0", "0", "0", "0", "0", "0" },
		{ "0", "0", "0", "0", "0", "0", "0", "0" },
		{ "p0", "p0", "p0", "p0", "p0", "p0", "p0", "p0" },
		{ "r0", "n0", "b0", "q0", "k0", "b0", "n0", "r0" }
	};

	for (int y = 0; y < 8; y++) {
		Array row = use_standard ? Array() : Array(custom_layout[y]);
		for (int x = 0; x < 8; x++) {
			String cell = use_standard ? std_layout[y][x] : (String)row[x];
			if (cell == "0") {
				board[x][y] = { EMPTY, NONE, false, false };
			} else {
				String type_char = cell.substr(0, 1);
				int color = cell.substr(1, 1).to_int();
				PieceType pt = EMPTY;
				if (type_char == "p") pt = PAWN;
				else if (type_char == "r") pt = ROOK;
				else if (type_char == "n") pt = KNIGHT;
				else if (type_char == "b") pt = BISHOP;
				else if (type_char == "q") pt = QUEEN;
				else if (type_char == "k") pt = KING;

				board[x][y] = { pt, (PieceColor)color, false, true };
			}
		}
	}
}

// Export piece info at given coordinates in a Godot-friendly Dictionary.
Dictionary Board::get_data_at(int x, int y) const {
	Dictionary d;
	if (!is_on_board(Vector2i(x, y))) {
		return d;
	}

	Piece P = board[x][y];
	if (!P.active) {
		return d;
	}

	String t = "";
	switch (P.type) {
		case PAWN: t = "p"; break;
		case ROOK: t = "r"; break;
		case KNIGHT: t = "n"; break;
		case BISHOP: t = "b"; break;
		case QUEEN: t = "q"; break;
		case KING: t = "k"; break;
		default: t = ""; break;
	}

	d["type"] = t;
	d["color"] = (int)P.color;
	return d;
}

// Helper to serialize the entire board state into an Array of Arrays of Dictionaries.
// This matches the "full boards with 8x8 piece structs" requirement.
static Array get_board_state_snapshot(const Board::Piece b[8][8]) {
	Array rows;
	for (int y = 0; y < 8; y++) {
		Array row;
		for (int x = 0; x < 8; x++) {
			Dictionary d;
			// Mirror the Piece struct
			d["active"] = b[x][y].active;
			if (b[x][y].active) {
				d["type"] = (int)b[x][y].type;
				d["color"] = (int)b[x][y].color;
				d["has_moved"] = b[x][y].has_moved;
			}
			row.append(d);
		}
		rows.append(row);
	}
	return rows;
}

// Enumerate all legal moves for a given color.
// RETURNS: An Array where each element is a Dictionary containing:
// {
//    "start": Vector2i,
//    "end": Vector2i,
//    "promotion": String (optional, "q", "r", "b", "n"),
//    "board": Array (8x8 grid of piece data representing the state AFTER the move)
// }
Array Board::get_all_possible_moves(int color) {
	Array moves;
	
	// Backup state to restore after simulations
	Piece board_backup[8][8];
	Vector2i en_passant_backup = en_passant_target;
	// We don't backup turn/promotion_pending because we reset them manually or don't use them during generation logic loop
	
	int promotion_row = (color == WHITE) ? 0 : 7;

	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			Piece P = board[x][y];
			if (!P.active || P.color != color) {
				continue;
			}

			for (int tx = 0; tx < 8; tx++) {
				for (int ty = 0; ty < 8; ty++) {
					Vector2i start(x, y);
					Vector2i end(tx, ty);

					if (is_valid_geometry(P, start, end)) {
						if (!does_move_cause_self_check(start, end)) {
							// Check for promotion
							bool is_promotion = (P.type == PAWN && end.y == promotion_row);
							
							if (is_promotion) {
								const char* promo_chars[] = {"q", "r", "b", "n"};
								PieceType promo_types[] = {QUEEN, ROOK, BISHOP, KNIGHT};
								
								for (int i = 0; i < 4; i++) {
									// 1. Backup
									std::memcpy(board_backup, board, sizeof(board));
									en_passant_target = en_passant_backup;

									// 2. Execute move (updates board, castling, en passant logic)
									// We use real_move=true to get the correct 'next turn' en_passant_target state
									execute_move_internal(start, end, true);
									
									// 3. Apply promotion
									board[end.x][end.y].type = promo_types[i];

									// 4. Record State
									Dictionary move_data;
									move_data["start"] = start;
									move_data["end"] = end;
									move_data["promotion"] = String(promo_chars[i]);
									move_data["board"] = get_board_state_snapshot(board);
									moves.append(move_data);

									// 5. Restore
									std::memcpy(board, board_backup, sizeof(board));
									en_passant_target = en_passant_backup;
								}
							} else {
								// Normal Move (including Castling / En Passant)
								
								// 1. Backup
								std::memcpy(board_backup, board, sizeof(board));
								en_passant_target = en_passant_backup;

								// 2. Execute
								execute_move_internal(start, end, true);

								// 3. Record State
								Dictionary move_data;
								move_data["start"] = start;
								move_data["end"] = end;
								move_data["board"] = get_board_state_snapshot(board);
								moves.append(move_data);

								// 4. Restore
								std::memcpy(board, board_backup, sizeof(board));
								en_passant_target = en_passant_backup;
							}
						}
					}
				}
			}
		}
	}
	
	return moves;
}

// Get all legal target squares for the piece at start_pos.
Array Board::get_valid_moves_for_piece(Vector2i start_pos) {
	Array valid_targets;
	if (!is_on_board(start_pos)) {
		return valid_targets;
	}

	Piece P = board[start_pos.x][start_pos.y];
	if (!P.active) {
		return valid_targets;
	}

	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			Vector2i target(x, y);
			if (is_valid_geometry(P, start_pos, target)) {
				if (!does_move_cause_self_check(start_pos, target)) {
					valid_targets.append(target);
				}
			}
		}
	}
	return valid_targets;
}

// Main move entry point from script. Handles legality, self-check and promotion trigger.
int Board::attempt_move(Vector2i start, Vector2i end) {
	if (promotion_pending) {
		return 0;
	}
	if (!is_on_board(start) || !is_on_board(end)) {
		return 0;
	}

	Piece P = board[start.x][start.y];
	if (!P.active || P.color != turn) {
		return 0;
	}

	if (!is_valid_geometry(P, start, end)) {
		return 0;
	}

	if (does_move_cause_self_check(start, end)) {
		return 0;
	}

	int promotion_row = (P.color == WHITE) ? 0 : 7;
	
	// If pawn reaches last rank, mark promotion and let the UI choose the piece.
	if (P.type == PAWN && end.y == promotion_row) {
		execute_move_internal(start, end, true);
		promotion_pending = true;
		promotion_square = end;
		return 2;
	}

	// Normal move.
	execute_move_internal(start, end, true);
	turn = 1 - turn;
	return 1;
}

// Commit a previously prepared pawn promotion (after UI selection).
void Board::commit_promotion(String type_str) {
	if (!promotion_pending) {
		return;
	}
	PieceType pt = QUEEN;
	if (type_str == "r") pt = ROOK;
	else if (type_str == "b") pt = BISHOP;
	else if (type_str == "n") pt = KNIGHT;
	
	board[promotion_square.x][promotion_square.y].type = pt;
	promotion_pending = false;
	turn = 1 - turn;
}

// Simple getter for current side to move.
int Board::get_turn() const {
	return turn;
}

// Execute the low-level board update for a move, including en passant and castling.
void Board::execute_move_internal(Vector2i start, Vector2i end, bool real_move) {
	Piece P = board[start.x][start.y];
	int move_dir = (P.color == WHITE) ? -1 : 1;

	// En Passant: capture pawn behind the target square.
	if (P.type == PAWN && end == en_passant_target) {
		board[end.x][end.y - move_dir] = { EMPTY, NONE, false, false };
	}

	// Castling: move rook as well if king moves two squares horizontally.
	if (P.type == KING && abs(end.x - start.x) > 1) {
		int rook_x = (end.x > start.x) ? 7 : 0;
		int rook_target_x = (end.x > start.x) ? 5 : 3;
		Piece rook = board[rook_x][end.y];
		board[rook_x][end.y] = { EMPTY, NONE, false, false };
		board[rook_target_x][end.y] = rook;
		board[rook_target_x][end.y].has_moved = true;
	}

	// Update En Passant target square only for real moves.
	if (real_move) {
		en_passant_target = Vector2i(-1, -1);
		// If pawn moved two squares, set en passant square in between.
		if (P.type == PAWN && abs(end.y - start.y) == 2) {
			en_passant_target = Vector2i(start.x, start.y + move_dir);
		}
	}

	// Move the piece to its new square.
	board[end.x][end.y] = P;
	board[end.x][end.y].has_moved = true;
	board[start.x][start.y] = { EMPTY, NONE, false, false };
}

// Check basic piece movement rules and collisions, including special cases.
bool Board::is_valid_geometry(const Piece &P, Vector2i start, Vector2i end) const {
	int dx = end.x - start.x;
	int dy = end.y - start.y;
	int abs_dx = abs(dx);
	int abs_dy = abs(dy);
	
	Piece target = board[end.x][end.y];

	// Cannot capture own piece.
	if (target.active && target.color == P.color) {
		return false;
	}

	int dir = (P.color == WHITE) ? -1 : 1;

	switch (P.type) {
		case PAWN:
			// Single-step forward.
			if (dx == 0 && dy == dir && !target.active) {
				return true;
			}
			// Double-step forward from starting rank (must be unobstructed).
			if (dx == 0 && dy == dir * 2 && !P.has_moved && !target.active) {
				if (!board[start.x][start.y + dir].active) {
					return true;
				}
			}
			// Diagonal capture or en passant.
			if (abs_dx == 1 && dy == dir) {
				if (target.active) {
					return true;
				}
				if (end == en_passant_target) {
					return true;
				}
			}
			return false;

		case KING:
			// Normal king move: one square in any direction.
			if (abs_dx <= 1 && abs_dy <= 1) {
				return true;
			}
			// Castling: horizontal move by two squares, rook and check rules handled here.
			if (abs_dy == 0 && abs_dx == 2 && !P.has_moved) {
				// King cannot castle out of, through, or into check.
				if (is_square_attacked(start, 1 - P.color)) {
					return false;
				}
				
				int rook_x = (dx > 0) ? 7 : 0;
				Piece rook = board[rook_x][start.y];
				
				if (!rook.active || rook.type != ROOK || rook.has_moved) {
					return false;
				}

				int step = (dx > 0) ? 1 : -1;
				for (int i = 1; i < 3; i++) {
					Vector2i check_pos(start.x + (i * step), start.y);
					if (board[check_pos.x][check_pos.y].active) {
						return false;
					}
					if (is_square_attacked(check_pos, 1 - P.color)) {
						return false;
					}
				}
				return true;
			}
			return false;

		case KNIGHT:
			return (abs_dx == 2 && abs_dy == 1) || (abs_dx == 1 && abs_dy == 2);

		case ROOK:
			return (dx == 0 || dy == 0) && is_path_clear(start, end);

		case BISHOP:
			return abs_dx == abs_dy && is_path_clear(start, end);

		case QUEEN:
			return (dx == 0 || dy == 0 || abs_dx == abs_dy) && is_path_clear(start, end);

		default:
			return false;
	}
}

// Check that all squares between start and end (exclusive) are empty.
bool Board::is_path_clear(Vector2i start, Vector2i end) const {
	int dx = (end.x - start.x) == 0 ? 0 : (end.x - start.x) > 0 ? 1 : -1;
	int dy = (end.y - start.y) == 0 ? 0 : (end.y - start.y) > 0 ? 1 : -1;
	
	Vector2i current = start + Vector2i(dx, dy);
	while (current != end) {
		if (board[current.x][current.y].active) {
			return false;
		}
		current += Vector2i(dx, dy);
	}
	return true;
}

// Determine if a square is attacked by any piece of the given color.
bool Board::is_square_attacked(Vector2i square, int by_color) const {
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			Piece P = board[x][y];
			if (P.active && P.color == by_color) {
				// Pawn attacks differ from pawn movement geometry.
				if (P.type == PAWN) {
					int dir = (P.color == WHITE) ? -1 : 1;
					if (abs(square.x - x) == 1 && (square.y - y) == dir) {
						return true;
					}
				} else if (P.type == KING) {
					// King attacks one square in all directions.
					if (abs(square.x - x) <= 1 && abs(square.y - y) <= 1) {
						return true;
					}
				} else if (is_valid_geometry(P, Vector2i(x, y), square)) {
					// Other pieces reuse geometry rules for attack detection.
					return true;
				}
			}
		}
	}
	return false;
}

// Test if making this move would leave own king in check.
bool Board::does_move_cause_self_check(Vector2i start, Vector2i end) {
	Piece orig_start = board[start.x][start.y];
	Piece orig_end = board[end.x][end.y];

	// Apply move virtually.
	// Note: This naive swap does not handle en passant/castling logic perfectly 
	// for check validation, but serves the basic "don't leave king hanging" check.
	board[end.x][end.y] = orig_start;
	board[start.x][start.y] = { EMPTY, NONE, false, false };

	bool check = is_in_check(orig_start.color);

	// Revert board.
	board[start.x][start.y] = orig_start;
	board[end.x][end.y] = orig_end;

	return check;
}

// Check if the given color's king is currently in check.
bool Board::is_in_check(int color) const {
	Vector2i king_pos(-1, -1);
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			if (board[x][y].active && board[x][y].type == KING && board[x][y].color == color) {
				king_pos = Vector2i(x, y);
				break;
			}
		}
		if (king_pos.x != -1) {
			break;
		}
	}
	return (king_pos.x != -1) && is_square_attacked(king_pos, 1 - color);
}

// Simple bounds check for 8x8 board.
bool Board::is_on_board(Vector2i pos) const {
	return pos.x >= 0 && pos.x < 8 && pos.y >= 0 && pos.y < 8;
}

// Godot method binding for scripting API.
void Board::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_board", "custom_layout"), &Board::setup_board);
	ClassDB::bind_method(D_METHOD("get_data_at", "x", "y"), &Board::get_data_at);
	ClassDB::bind_method(D_METHOD("attempt_move", "start", "end"), &Board::attempt_move);
	ClassDB::bind_method(D_METHOD("commit_promotion", "type_str"), &Board::commit_promotion);
	ClassDB::bind_method(D_METHOD("get_turn"), &Board::get_turn);
	
	// Expose move generation helpers to GDScript/AI.
	ClassDB::bind_method(D_METHOD("get_all_possible_moves", "color"), &Board::get_all_possible_moves);
	ClassDB::bind_method(D_METHOD("get_valid_moves_for_piece", "start_pos"), &Board::get_valid_moves_for_piece);
}
