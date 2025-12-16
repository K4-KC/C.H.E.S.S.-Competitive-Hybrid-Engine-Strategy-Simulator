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
#include <cstring>
#include <climits>
#include <algorithm>

using namespace godot;

// Piece type constants (lowest 3 bits)
#define PIECE_NONE   0
#define PIECE_PAWN   1
#define PIECE_KNIGHT 2
#define PIECE_BISHOP 3
#define PIECE_ROOK   4
#define PIECE_QUEEN  5
#define PIECE_KING   6

// Color constants (bits 3-4)
#define COLOR_NONE  0
#define COLOR_WHITE 8
#define COLOR_BLACK 16

// Masks for bitwise operations
#define PIECE_TYPE_MASK 7
#define COLOR_MASK      24

// Helper macros - inline for performance
#define GET_PIECE_TYPE(square) ((square) & PIECE_TYPE_MASK)
#define GET_COLOR(square) ((square) & COLOR_MASK)
#define MAKE_PIECE(type, color) ((type) | (color))
#define IS_EMPTY(square) (((square) & PIECE_TYPE_MASK) == 0)
#define IS_WHITE(square) (((square) & COLOR_MASK) == COLOR_WHITE)
#define IS_BLACK(square) (((square) & COLOR_MASK) == COLOR_BLACK)

// Direction constants
#define DIR_N   8
#define DIR_S  -8
#define DIR_E   1
#define DIR_W  -1
#define DIR_NE  9
#define DIR_NW  7
#define DIR_SE -7
#define DIR_SW -9

// AI evaluation constants
#define CHECKMATE_SCORE 100000
#define STALEMATE_SCORE 0

// Piece values for evaluation (centipawns)
#define PAWN_VALUE   100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 300
#define ROOK_VALUE   500
#define QUEEN_VALUE  900

// Move ordering score constants
#define SCORE_TT_MOVE           30000   // Transposition table best move (highest priority)
#define SCORE_QUEEN_PROMOTION   20000
#define SCORE_CAPTURE_BASE      10000
#define SCORE_OTHER_PROMOTION   9000
#define SCORE_QUIET_MOVE        0

// ==================== TRANSPOSITION TABLE ====================

// TT Entry flags
#define TT_FLAG_EXACT  0  // Exact score (PV node)
#define TT_FLAG_ALPHA  1  // Upper bound (failed low)
#define TT_FLAG_BETA   2  // Lower bound (failed high)

// Transposition Table size (number of entries)
// ~24 bytes per entry * 1M = ~24MB
#define TT_SIZE 1048576  // 2^20 = 1,048,576 entries

// Transposition Table Entry
struct TTEntry {
    uint64_t key;       // Full Zobrist hash for verification (handles collisions)
    int16_t score;      // Stored evaluation score
    int8_t depth;       // Search depth when this was stored
    uint8_t flag;       // TT_FLAG_EXACT, TT_FLAG_ALPHA, or TT_FLAG_BETA
    uint8_t best_from;  // Best move source square (255 if none)
    uint8_t best_to;    // Best move target square
    uint8_t age;        // Search age (for replacement strategy)
    uint8_t padding;    // Padding for alignment
};

// ==================== MOVE STRUCTURES ====================

// Lightweight move for perft and search
struct FastMove {
    uint8_t from;
    uint8_t to;
    uint8_t flags;      // bit 0: capture, bit 1: ep, bit 2: castling, bits 3-5: promotion piece
    uint8_t captured;
    int16_t score;      // Move ordering score (higher = search first)
};

// Full move structure for game history
struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t captured_piece;
    uint8_t promotion_piece;
    bool is_castling;
    bool is_en_passant;
    uint8_t en_passant_target_before;
    uint8_t halfmove_clock_before;
    bool castling_rights_before[4];
    uint64_t hash_before;  // Store hash for easy revert
};

// Pre-allocated move list to avoid heap allocations
struct MoveList {
    FastMove moves[256];
    int count;
    
    inline void clear() { count = 0; }
    inline void add(uint8_t from, uint8_t to, uint8_t flags = 0, uint8_t captured = 0) {
        moves[count++] = {from, to, flags, captured, 0};
    }
};

// ==================== BOARD CLASS ====================

class Board : public Node2D {
    GDCLASS(Board, Node2D)

private:
    // Board state: 64 squares
    uint8_t squares[64];
    
    // Cached king positions for fast lookup
    uint8_t white_king_pos;
    uint8_t black_king_pos;
    
    // Game state
    uint8_t turn;
    std::vector<Move> move_history;
    std::vector<String> move_history_notation;
    
    // Castling rights: [0]=WK, [1]=WQ, [2]=BK, [3]=BQ
    bool castling_rights[4];
    
    // En passant target square (0-63, or 255 if none)
    uint8_t en_passant_target;
    
    // Halfmove clock and fullmove number
    uint8_t halfmove_clock;
    uint16_t fullmove_number;
    
    // Promotion handling
    uint8_t promotion_pending_from;
    uint8_t promotion_pending_to;
    bool promotion_pending;
    
    // ==================== ZOBRIST HASHING ====================
    uint64_t current_hash;  // Current position hash
    
    // ==================== TRANSPOSITION TABLE ====================
    static TTEntry* tt_table;           // Transposition table
    static bool tt_initialized;         // Flag for TT initialization
    static uint8_t tt_age;              // Current search age
    
    // TT helper functions
    static void init_tt();
    void tt_store(uint64_t key, int score, int depth, int flag, uint8_t best_from, uint8_t best_to);
    TTEntry* tt_probe(uint64_t key) const;
    void tt_clear();
    void tt_new_search();  // Increment age for new search

    // ==================== PRECOMPUTED TABLES ====================
    static bool knight_attacks_initialized;
    static uint8_t knight_attack_squares[64][8];
    static uint8_t knight_attack_count[64];
    static uint8_t king_attack_squares[64][8];
    static uint8_t king_attack_count[64];
    static uint8_t squares_to_edge[64][8];
    static int16_t mvv_lva_table[7][7];
    
    static void init_attack_tables();
    static void init_mvv_lva_table();
    
    // ==================== ZOBRIST HELPERS ====================
    // Calculate full hash from scratch (used in setup)
    uint64_t calculate_hash() const;
    
    // Get Zobrist piece index (0-11) from piece byte
    int get_zobrist_piece_index(uint8_t piece) const;
    
    // Hash update helpers (XOR operations)
    void hash_piece(uint8_t piece, uint8_t square);      // XOR piece in/out
    void hash_castling(int right);                        // XOR castling right
    void hash_en_passant(uint8_t ep_square);             // XOR en passant
    void hash_side();                                     // XOR side to move
    
    // ==================== INTERNAL HELPERS ====================
    void clear_board();
    void initialize_starting_position();
    bool parse_fen(const String &fen);
    String generate_fen() const;
    void update_king_cache();

    bool is_square_attacked_fast(uint8_t pos, uint8_t attacking_color) const;
    
    void generate_pawn_moves(uint8_t pos, MoveList &moves) const;
    void generate_knight_moves(uint8_t pos, MoveList &moves) const;
    void generate_bishop_moves(uint8_t pos, MoveList &moves) const;
    void generate_rook_moves(uint8_t pos, MoveList &moves) const;
    void generate_queen_moves(uint8_t pos, MoveList &moves) const;
    void generate_king_moves(uint8_t pos, MoveList &moves) const;
    void generate_castling_moves(uint8_t pos, MoveList &moves) const;
    void generate_all_pseudo_legal(MoveList &moves) const;
    
    void make_move_fast(const FastMove &m);
    void unmake_move_fast(const FastMove &m, uint8_t ep_before, bool castling_before[4], uint64_t hash_before);
    
    uint64_t perft_internal(int depth);
    
    bool is_king_in_check(uint8_t color) const;
    bool would_be_in_check_after_move(uint8_t from, uint8_t to, uint8_t color);
    
    void add_pawn_moves(uint8_t pos, Array &moves) const;
    void add_knight_moves(uint8_t pos, Array &moves) const;
    void add_bishop_moves(uint8_t pos, Array &moves) const;
    void add_rook_moves(uint8_t pos, Array &moves) const;
    void add_queen_moves(uint8_t pos, Array &moves) const;
    void add_king_moves(uint8_t pos, Array &moves) const;
    Array get_pseudo_legal_moves_for_piece(uint8_t pos) const;
    
    void make_move_internal(uint8_t from, uint8_t to, Move &move_record);
    void revert_move_internal(const Move &move);
    
    bool can_castle_kingside(uint8_t color) const;
    bool can_castle_queenside(uint8_t color) const;
    void add_castling_moves(uint8_t pos, Array &moves) const;
    
    String move_to_notation(const Move &move) const;

    // ==================== AI INTERNAL HELPERS ====================
    int16_t score_move(const FastMove &m, uint8_t tt_best_from, uint8_t tt_best_to) const;
    void score_moves(MoveList &moves, uint8_t tt_best_from, uint8_t tt_best_to) const;
    void sort_moves(MoveList &moves) const;
    
    int minimax_internal(int depth, int alpha, int beta, bool is_maximizing);
    bool has_legal_moves() const;

protected:
    static void _bind_methods();

public:
    Board();
    ~Board();

    void _ready();
    
    // Public API
    uint8_t get_turn() const;
    uint8_t get_piece_on_square(uint8_t pos) const;
    void set_piece_on_square(uint8_t pos, uint8_t piece);
    
    void setup_board(const String &fen_notation);
    String get_fen() const;
    
    uint8_t attempt_move(uint8_t start, uint8_t end);
    void commit_promotion(const String &type_str);
    
    void revert_move();
    Array get_moves() const;
    
    Array get_all_possible_moves(uint8_t color);
    Array get_legal_moves_for_piece(uint8_t square);
    uint64_t count_all_moves(uint8_t depth);
    Dictionary get_perft_analysis(uint8_t depth);
    void make_move(uint8_t start, uint8_t end);
    
    // AI Functions
    int evaluate_board() const;
    Dictionary get_best_move(int depth);
    
    // Game state queries
    bool is_checkmate(uint8_t color);
    bool is_stalemate(uint8_t color);
    bool is_check(uint8_t color) const;
    bool is_game_over();
    int get_game_result();
    
    // Utility
    Vector2i pos_to_coords(uint8_t pos) const;
    uint8_t coords_to_pos(int rank, int file) const;
    String square_to_algebraic(uint8_t pos) const;
    uint8_t algebraic_to_square(const String &algebraic) const;
    
    // Hash accessor (for debugging/testing)
    uint64_t get_hash() const { return current_hash; }
    
    inline uint8_t get_king_pos(uint8_t color) const {
        return (color == 0) ? white_king_pos : black_king_pos;
    }
    
    static inline int get_piece_value(uint8_t piece_type) {
        switch (piece_type) {
            case PIECE_PAWN:   return PAWN_VALUE;
            case PIECE_KNIGHT: return KNIGHT_VALUE;
            case PIECE_BISHOP: return BISHOP_VALUE;
            case PIECE_ROOK:   return ROOK_VALUE;
            case PIECE_QUEEN:  return QUEEN_VALUE;
            case PIECE_KING:   return 0;
            default:          return 0;
        }
    }
};

#endif // BOARD_H