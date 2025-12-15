#ifndef BOARD_RULES_H
#define BOARD_RULES_H

// Godot C++ bindings for Node2D, core registration, and core types.
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

// BoardRules implements the chess rules and board state as a Godot Node2D.
class Board : public Node2D {
	GDCLASS(Board, Node2D)

public:
	// Internal representation of piece type and color.
	enum PieceType { EMPTY = -1, PAWN = 0, ROOK = 1, KNIGHT = 2, BISHOP = 3, QUEEN = 4, KING = 5 };
	enum PieceColor { WHITE = 0, BLACK = 1, NONE = -1 };

	struct Piece {
		PieceType type;
		PieceColor color;
		bool has_moved; // Tracks if piece has moved (for castling, pawn double step).
		bool active;    // false = empty square.
	};

private:
	// Board is indexed as board[x][y] with x,y in [0,7].
	Piece board[8][8];

	int turn; // 0 = White, 1 = Black

	// En passant target square; (-1, -1) if none is available.
	Vector2i en_passant_target;

	// Promotion state: used when a pawn reaches last rank.
	bool promotion_pending;
	Vector2i promotion_square;

	// Internal Logic helpers
	bool is_on_board(Vector2i pos) const;
	bool is_valid_geometry(const Piece &P, Vector2i start, Vector2i end) const;
	bool is_path_clear(Vector2i start, Vector2i end) const;
	bool is_square_attacked(Vector2i square, int by_color) const;
	bool does_move_cause_self_check(Vector2i start, Vector2i end);
	bool is_in_check(int color) const;
	bool is_checkmate(int color); // Declared for future use; not exposed to script.
	void execute_move_internal(Vector2i start, Vector2i end, bool real_move);

protected:
	static void _bind_methods();

public:
	Board();
	~Board();

	// Initializes board with standard layout or a custom layout array.
	void setup_board(const Array &custom_layout);

	// Returns a Dictionary describing piece at (x,y), or empty if no active piece.
	Dictionary get_data_at(int x, int y) const;

	// Core Gameplay

	// Attempts a move; return: 0 = fail, 1 = success, 2 = success with promotion pending.
	int attempt_move(Vector2i start, Vector2i end);

	// Finalize promotion at promotion_square, based on type_str ("q","r","b","n").
	void commit_promotion(String type_str);

	// Current side to move: 0 = white, 1 = black.
	int get_turn() const;

	// Returns all legal moves for given color as an Array of Dictionaries.
	Array get_all_possible_moves(int color);

	// Returns all legal target squares for a piece at start_pos.
	Array get_valid_moves_for_piece(Vector2i start_pos);
};

#endif
