#ifndef BOARD_RULES_H
#define BOARD_RULES_H

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

class BoardRules : public Node2D {
    GDCLASS(BoardRules, Node2D)

public:
    enum PieceType { EMPTY = -1, PAWN = 0, ROOK = 1, KNIGHT = 2, BISHOP = 3, QUEEN = 4, KING = 5 };
    enum PieceColor { WHITE = 0, BLACK = 1, NONE = -1 };

    struct Piece {
        PieceType type;
        PieceColor color;
        bool has_moved;
        bool active;
    };

private:
    Piece board[8][8];
    int turn; // 0 = White, 1 = Black
    Vector2i en_passant_target;
    bool promotion_pending;
    Vector2i promotion_square;
    
    // Internal Logic Helpers
    bool is_on_board(Vector2i pos) const;
    bool is_valid_geometry(const Piece& p, Vector2i start, Vector2i end) const;
    bool is_path_clear(Vector2i start, Vector2i end) const;
    bool is_square_attacked(Vector2i square, int by_color) const;
    bool does_move_cause_self_check(Vector2i start, Vector2i end);
    bool is_in_check(int color) const;
    bool is_checkmate(int color);
    void execute_move_internal(Vector2i start, Vector2i end, bool real_move);

protected:
    static void _bind_methods();

public:
    BoardRules();
    ~BoardRules();

    void setup_board(const Array& custom_layout);
    Dictionary get_data_at(int x, int y) const;
    
    // Core Gameplay
    int attempt_move(Vector2i start, Vector2i end); // 0:Fail, 1:Success, 2:Promo
    void commit_promotion(String type_str);
    int get_turn() const;
    
    // --- NEW FUNCTIONS ---
    
    // For AI: Returns Array of Dictionaries { "start": Vector2i, "end": Vector2i }
    // Represents "moves in board datatype" (the logical definition of a move)
    Array get_all_possible_moves(int color);

    // For UI: Returns Array of Vector2i
    // Represents "moves in terms of board squares" (valid targets for highlights)
    Array get_valid_moves_for_piece(Vector2i start_pos);
};

#endif // BOARD_RULES_H
