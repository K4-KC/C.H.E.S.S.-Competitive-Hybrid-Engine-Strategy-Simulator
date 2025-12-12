#ifndef BOARD_H
#define BOARD_H

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <vector>
#include <cstdint>

using namespace godot;

// Piece type constants (lowest 3 bits)
#define PIECE_NONE   0b000
#define PIECE_PAWN   0b001
#define PIECE_KNIGHT 0b010
#define PIECE_BISHOP 0b011
#define PIECE_ROOK   0b100
#define PIECE_QUEEN  0b101
#define PIECE_KING   0b110

// Color constants (bits 3-4)
#define COLOR_NONE  0b00000
#define COLOR_WHITE 0b01000
#define COLOR_BLACK 0b10000

// Masks for bitwise operations
#define PIECE_TYPE_MASK 0b00111
#define COLOR_MASK      0b11000

// Helper macros
#define GET_PIECE_TYPE(square) ((square) & PIECE_TYPE_MASK)
#define GET_COLOR(square) ((square) & COLOR_MASK)
#define MAKE_PIECE(type, color) ((type) | (color))
#define IS_EMPTY(square) (GET_PIECE_TYPE(square) == PIECE_NONE)
#define IS_WHITE(square) (GET_COLOR(square) == COLOR_WHITE)
#define IS_BLACK(square) (GET_COLOR(square) == COLOR_BLACK)

// Move structure for internal representation
struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t captured_piece;
    uint8_t promotion_piece;
    bool is_castling;
    bool is_en_passant;
    uint8_t en_passant_target_before;
    bool castling_rights_before[4]; // WK, WQ, BK, BQ
};

class Board : public Node2D {
    GDCLASS(Board, Node2D)

private:
    // Board state: 64 squares, each uint8_t encoding piece and color
    uint8_t squares[64];
    
    // Game state
    uint8_t turn; // 0 = White, 1 = Black
    std::vector<Move> move_history;
    std::vector<String> move_history_notation; // FEN notation for each move
    
    // Castling rights: [0]=WK, [1]=WQ, [2]=BK, [3]=BQ
    bool castling_rights[4];
    
    // En passant target square (0-63, or 255 if none)
    uint8_t en_passant_target;
    
    // Halfmove clock for 50-move rule
    uint8_t halfmove_clock;
    
    // Fullmove number
    uint16_t fullmove_number;
    
    // For promotion handling
    uint8_t promotion_pending_from;
    uint8_t promotion_pending_to;
    bool promotion_pending;

    // Internal helper functions
    void clear_board();
    void initialize_starting_position();
    bool parse_fen(const String &fen);
    String generate_fen() const;
    
    // Coordinate conversion
    inline int get_rank(uint8_t pos) const { return pos / 8; }
    inline int get_file(uint8_t pos) const { return pos % 8; }
    inline uint8_t make_pos(int rank, int file) const { return rank * 8 + file; }
    inline bool is_valid_pos(int rank, int file) const { return rank >= 0 && rank < 8 && file >= 0 && file < 8; }
    
    // Move validation helpers
    bool is_square_attacked(uint8_t pos, uint8_t attacking_color) const;
    bool is_king_in_check(uint8_t color) const;
    bool would_be_in_check_after_move(uint8_t from, uint8_t to, uint8_t color);
    uint8_t find_king(uint8_t color) const;
    
    // Pseudo-legal move generation (doesn't check for checks)
    void add_pawn_moves(uint8_t pos, Array &moves) const;
    void add_knight_moves(uint8_t pos, Array &moves) const;
    void add_sliding_moves(uint8_t pos, Array &moves, const int directions[][2], int num_directions) const;
    void add_bishop_moves(uint8_t pos, Array &moves) const;
    void add_rook_moves(uint8_t pos, Array &moves) const;
    void add_queen_moves(uint8_t pos, Array &moves) const;
    void add_king_moves(uint8_t pos, Array &moves) const;
    Array get_pseudo_legal_moves_for_piece(uint8_t pos) const;
    
    // Move execution
    void make_move_internal(uint8_t from, uint8_t to, Move &move_record);
    void unmake_move_internal(const Move &move);
    
    // Castling helpers
    bool can_castle_kingside(uint8_t color) const;
    bool can_castle_queenside(uint8_t color) const;
    void add_castling_moves(uint8_t pos, Array &moves) const;
    
    // Move notation
    String move_to_notation(const Move &move) const;

protected:
    static void _bind_methods();

public:
    Board();
    ~Board();

    // Godot lifecycle
    void _ready();
    
    // Public API
    uint8_t get_turn() const; // Returns 0 for White and 1 for Black
    uint8_t get_piece_on_square(uint8_t pos) const;
    void set_piece_on_square(uint8_t pos, uint8_t piece); // For testing/setup
    
    void setup_board(const String &fen_notation); // Use FEN notation
    String get_fen() const; // Get current position as FEN
    
    uint8_t attempt_move(uint8_t start, uint8_t end); // 0=fail, 1=success, 2=promotion pending
    void commit_promotion(const String &type_str); // 'q','r','b','n'
    
    void revert_move(); // Undo last move
    Array get_moves() const; // Returns array of move strings in algebraic notation
    
    // AI/Analysis functions
    Array get_all_possible_moves(uint8_t color);
    Array get_legal_moves_for_piece(uint8_t square);
    void make_move(uint8_t start, uint8_t end); // Direct move without validation (for AI)
    
    // Game state queries
    bool is_checkmate(uint8_t color);
    bool is_stalemate(uint8_t color);
    bool is_check(uint8_t color) const;
    bool is_game_over();
    int get_game_result(); // 0=ongoing, 1=white wins, 2=black wins, 3=draw
    
    // Utility
    Vector2i pos_to_coords(uint8_t pos) const; // Convert 0-63 to (rank, file)
    uint8_t coords_to_pos(int rank, int file) const; // Convert (rank, file) to 0-63
    String square_to_algebraic(uint8_t pos) const; // e.g., 0 -> "a1"
    uint8_t algebraic_to_square(const String &algebraic) const; // e.g., "e4" -> 28
};

#endif // BOARD_H