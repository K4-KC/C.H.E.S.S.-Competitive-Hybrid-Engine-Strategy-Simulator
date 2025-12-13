#include "board.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>

using namespace godot;

// Direction offsets for move generation
// Using single index: North = +8, South = -8, East = +1, West = -1
static const int DIRECTION_N = 8;
static const int DIRECTION_S = -8;
static const int DIRECTION_E = 1;
static const int DIRECTION_W = -1;
static const int DIRECTION_NE = 9;
static const int DIRECTION_NW = 7;
static const int DIRECTION_SE = -7;
static const int DIRECTION_SW = -9;

// Knight move offsets
static const int KNIGHT_OFFSETS[8] = {
    17,  // N+N+E
    15,  // N+N+W
    10,  // E+E+N
    6,   // W+W+N
    -6,  // E+E+S
    -10, // W+W+S
    -15, // S+S+E
    -17  // S+S+W
};

// Sliding piece directions
static const int ROOK_DIRECTIONS[4] = { DIRECTION_N, DIRECTION_S, DIRECTION_E, DIRECTION_W };
static const int BISHOP_DIRECTIONS[4] = { DIRECTION_NE, DIRECTION_NW, DIRECTION_SE, DIRECTION_SW };
static const int QUEEN_DIRECTIONS[8] = { 
    DIRECTION_N, DIRECTION_S, DIRECTION_E, DIRECTION_W,
    DIRECTION_NE, DIRECTION_NW, DIRECTION_SE, DIRECTION_SW 
};

// Helper to check if a move stays on the board and doesn't wrap
static inline bool is_valid_square(int sq) {
    return sq >= 0 && sq < 64;
}

// Check if moving from 'from' to 'to' wraps around the board edges
static inline bool does_wrap(int from, int to, int direction) {
    int from_file = from % 8;
    int to_file = to % 8;
    
    // For east/west moves, check file wrapping
    if (direction == DIRECTION_E || direction == DIRECTION_NE || direction == DIRECTION_SE) {
        return to_file <= from_file; // Wrapped from h to a
    }
    if (direction == DIRECTION_W || direction == DIRECTION_NW || direction == DIRECTION_SW) {
        return to_file >= from_file; // Wrapped from a to h
    }
    return false;
}

// Check if knight move is valid (doesn't wrap)
static inline bool is_valid_knight_move(int from, int to) {
    if (!is_valid_square(to)) return false;
    
    int from_file = from % 8;
    int to_file = to % 8;
    int file_diff = from_file - to_file;
    if (file_diff < 0) file_diff = -file_diff;
    
    // Knight moves change file by 1 or 2
    return file_diff == 1 || file_diff == 2;
}

// Check if king move is valid (doesn't wrap)
static inline bool is_valid_king_move(int from, int to) {
    if (!is_valid_square(to)) return false;
    
    int from_file = from % 8;
    int to_file = to % 8;
    int file_diff = from_file - to_file;
    if (file_diff < 0) file_diff = -file_diff;
    
    return file_diff <= 1;
}

Board::Board() {
    clear_board();
    turn = 0; // White starts
    en_passant_target = 255;
    halfmove_clock = 0;
    fullmove_number = 1;
    promotion_pending = false;
    promotion_pending_from = 0;
    promotion_pending_to = 0;
    
    for (int i = 0; i < 4; i++) {
        castling_rights[i] = true;
    }
}

Board::~Board() {
}

void Board::_ready() {
    // Initialize to starting position by default
    initialize_starting_position();
}

void Board::_bind_methods() {
    // Bind public methods to Godot
    ClassDB::bind_method(D_METHOD("get_turn"), &Board::get_turn);
    ClassDB::bind_method(D_METHOD("get_piece_on_square", "pos"), &Board::get_piece_on_square);
    ClassDB::bind_method(D_METHOD("set_piece_on_square", "pos", "piece"), &Board::set_piece_on_square);
    ClassDB::bind_method(D_METHOD("setup_board", "fen_notation"), &Board::setup_board);
    ClassDB::bind_method(D_METHOD("get_fen"), &Board::get_fen);
    ClassDB::bind_method(D_METHOD("attempt_move", "start", "end"), &Board::attempt_move);
    ClassDB::bind_method(D_METHOD("commit_promotion", "type_str"), &Board::commit_promotion);
    ClassDB::bind_method(D_METHOD("revert_move"), &Board::revert_move);
    ClassDB::bind_method(D_METHOD("get_moves"), &Board::get_moves);
    ClassDB::bind_method(D_METHOD("get_all_possible_moves", "color"), &Board::get_all_possible_moves);
    ClassDB::bind_method(D_METHOD("get_legal_moves_for_piece", "square"), &Board::get_legal_moves_for_piece);
    ClassDB::bind_method(D_METHOD("count_all_moves", "depth"), &Board::count_all_moves);
    ClassDB::bind_method(D_METHOD("make_move", "start", "end"), &Board::make_move);
    ClassDB::bind_method(D_METHOD("is_checkmate", "color"), &Board::is_checkmate);
    ClassDB::bind_method(D_METHOD("is_stalemate", "color"), &Board::is_stalemate);
    ClassDB::bind_method(D_METHOD("is_check", "color"), &Board::is_check);
    ClassDB::bind_method(D_METHOD("is_game_over"), &Board::is_game_over);
    ClassDB::bind_method(D_METHOD("get_game_result"), &Board::get_game_result);
    ClassDB::bind_method(D_METHOD("pos_to_coords", "pos"), &Board::pos_to_coords);
    ClassDB::bind_method(D_METHOD("coords_to_pos", "rank", "file"), &Board::coords_to_pos);
    ClassDB::bind_method(D_METHOD("square_to_algebraic", "pos"), &Board::square_to_algebraic);
    ClassDB::bind_method(D_METHOD("algebraic_to_square", "algebraic"), &Board::algebraic_to_square);
}

void Board::clear_board() {
    memset(squares, 0, sizeof(squares));
    move_history.clear();
    move_history_notation.clear();
}

void Board::initialize_starting_position() {
    clear_board();
    
    // Set up white pieces (rank 0 and 1)
    squares[0] = MAKE_PIECE(PIECE_ROOK, COLOR_WHITE);
    squares[1] = MAKE_PIECE(PIECE_KNIGHT, COLOR_WHITE);
    squares[2] = MAKE_PIECE(PIECE_BISHOP, COLOR_WHITE);
    squares[3] = MAKE_PIECE(PIECE_QUEEN, COLOR_WHITE);
    squares[4] = MAKE_PIECE(PIECE_KING, COLOR_WHITE);
    squares[5] = MAKE_PIECE(PIECE_BISHOP, COLOR_WHITE);
    squares[6] = MAKE_PIECE(PIECE_KNIGHT, COLOR_WHITE);
    squares[7] = MAKE_PIECE(PIECE_ROOK, COLOR_WHITE);
    
    for (int i = 8; i < 16; i++) {
        squares[i] = MAKE_PIECE(PIECE_PAWN, COLOR_WHITE);
    }
    
    // Set up black pieces (rank 6 and 7)
    for (int i = 48; i < 56; i++) {
        squares[i] = MAKE_PIECE(PIECE_PAWN, COLOR_BLACK);
    }
    
    squares[56] = MAKE_PIECE(PIECE_ROOK, COLOR_BLACK);
    squares[57] = MAKE_PIECE(PIECE_KNIGHT, COLOR_BLACK);
    squares[58] = MAKE_PIECE(PIECE_BISHOP, COLOR_BLACK);
    squares[59] = MAKE_PIECE(PIECE_QUEEN, COLOR_BLACK);
    squares[60] = MAKE_PIECE(PIECE_KING, COLOR_BLACK);
    squares[61] = MAKE_PIECE(PIECE_BISHOP, COLOR_BLACK);
    squares[62] = MAKE_PIECE(PIECE_KNIGHT, COLOR_BLACK);
    squares[63] = MAKE_PIECE(PIECE_ROOK, COLOR_BLACK);
    
    // Reset game state
    turn = 0;
    for (int i = 0; i < 4; i++) {
        castling_rights[i] = true;
    }
    en_passant_target = 255;
    halfmove_clock = 0;
    fullmove_number = 1;
    promotion_pending = false;
}

bool Board::parse_fen(const String &fen) {
    clear_board();
    
    // Split FEN into parts
    PackedStringArray parts = fen.split(" ");
    if (parts.size() < 1) return false;
    
    // Parse piece placement
    String placement = parts[0];
    int square = 56; // Start at a8
    
    for (int i = 0; i < placement.length(); i++) {
        char32_t c = placement[i];
        
        if (c == '/') {
            square -= 16; // Move to start of next rank down
            continue;
        }
        
        if (c >= '1' && c <= '8') {
            square += (c - '0');
            continue;
        }
        
        uint8_t piece = PIECE_NONE;
        uint8_t color = (c >= 'A' && c <= 'Z') ? COLOR_WHITE : COLOR_BLACK;
        
        char32_t lower_c = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
        
        switch (lower_c) {
            case 'p': piece = PIECE_PAWN; break;
            case 'n': piece = PIECE_KNIGHT; break;
            case 'b': piece = PIECE_BISHOP; break;
            case 'r': piece = PIECE_ROOK; break;
            case 'q': piece = PIECE_QUEEN; break;
            case 'k': piece = PIECE_KING; break;
            default: return false;
        }
        
        if (square >= 0 && square < 64) {
            squares[square] = MAKE_PIECE(piece, color);
        }
        square++;
    }
    
    // Parse active color
    if (parts.size() >= 2) {
        turn = (parts[1] == "b") ? 1 : 0;
    }
    
    // Parse castling rights
    castling_rights[0] = false; // WK
    castling_rights[1] = false; // WQ
    castling_rights[2] = false; // BK
    castling_rights[3] = false; // BQ
    
    if (parts.size() >= 3 && parts[2] != "-") {
        String castling = parts[2];
        for (int i = 0; i < castling.length(); i++) {
            char32_t c = castling[i];
            if (c == 'K') castling_rights[0] = true;
            if (c == 'Q') castling_rights[1] = true;
            if (c == 'k') castling_rights[2] = true;
            if (c == 'q') castling_rights[3] = true;
        }
    }
    
    // Parse en passant target
    en_passant_target = 255;
    if (parts.size() >= 4 && parts[3] != "-") {
        en_passant_target = algebraic_to_square(parts[3]);
    }
    
    // Parse halfmove clock
    halfmove_clock = 0;
    if (parts.size() >= 5) {
        halfmove_clock = parts[4].to_int();
    }
    
    // Parse fullmove number
    fullmove_number = 1;
    if (parts.size() >= 6) {
        fullmove_number = parts[5].to_int();
    }
    
    return true;
}

String Board::generate_fen() const {
    String fen = "";
    
    // Piece placement
    for (int rank = 7; rank >= 0; rank--) {
        int empty_count = 0;
        
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            uint8_t piece = squares[sq];
            
            if (IS_EMPTY(piece)) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += String::num_int64(empty_count);
                    empty_count = 0;
                }
                
                char piece_char = '.';
                switch (GET_PIECE_TYPE(piece)) {
                    case PIECE_PAWN:   piece_char = 'p'; break;
                    case PIECE_KNIGHT: piece_char = 'n'; break;
                    case PIECE_BISHOP: piece_char = 'b'; break;
                    case PIECE_ROOK:   piece_char = 'r'; break;
                    case PIECE_QUEEN:  piece_char = 'q'; break;
                    case PIECE_KING:   piece_char = 'k'; break;
                }
                
                if (IS_WHITE(piece)) {
                    piece_char -= 32; // To uppercase
                }
                
                fen += String::chr(piece_char);
            }
        }
        
        if (empty_count > 0) {
            fen += String::num_int64(empty_count);
        }
        
        if (rank > 0) {
            fen += "/";
        }
    }
    
    // Active color
    fen += (turn == 0) ? " w " : " b ";
    
    // Castling rights
    String castling = "";
    if (castling_rights[0]) castling += "K";
    if (castling_rights[1]) castling += "Q";
    if (castling_rights[2]) castling += "k";
    if (castling_rights[3]) castling += "q";
    if (castling.is_empty()) castling = "-";
    fen += castling;
    
    // En passant target
    fen += " ";
    if (en_passant_target == 255) {
        fen += "-";
    } else {
        fen += square_to_algebraic(en_passant_target);
    }
    
    // Halfmove clock
    fen += " " + String::num_int64(halfmove_clock);
    
    // Fullmove number
    fen += " " + String::num_int64(fullmove_number);
    
    return fen;
}

uint8_t Board::find_king(uint8_t color) const {
    uint8_t target_color = (color == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    for (int i = 0; i < 64; i++) {
        if (GET_PIECE_TYPE(squares[i]) == PIECE_KING && GET_COLOR(squares[i]) == target_color) {
            return i;
        }
    }
    return 255; // King not found (shouldn't happen in valid game)
}

bool Board::is_square_attacked(uint8_t pos, uint8_t attacking_color) const {
    uint8_t attacker_color = (attacking_color == 0) ? COLOR_WHITE : COLOR_BLACK;
    int target_pos = (int)pos;
    
    // Check for pawn attacks
    int pawn_direction = (attacking_color == 0) ? DIRECTION_S : DIRECTION_N;
    int pawn_attack_squares[2] = { target_pos + pawn_direction + DIRECTION_W, 
                                   target_pos + pawn_direction + DIRECTION_E };
    
    for (int i = 0; i < 2; i++) {
        int sq = pawn_attack_squares[i];
        if (is_valid_square(sq)) {
            int file_diff = (sq % 8) - (target_pos % 8);
            if (file_diff < 0) file_diff = -file_diff;
            if (file_diff == 1) { // Valid diagonal attack
                if (GET_PIECE_TYPE(squares[sq]) == PIECE_PAWN && GET_COLOR(squares[sq]) == attacker_color) {
                    return true;
                }
            }
        }
    }
    
    // Check for knight attacks
    for (int i = 0; i < 8; i++) {
        int sq = target_pos + KNIGHT_OFFSETS[i];
        if (is_valid_knight_move(target_pos, sq)) {
            if (GET_PIECE_TYPE(squares[sq]) == PIECE_KNIGHT && GET_COLOR(squares[sq]) == attacker_color) {
                return true;
            }
        }
    }
    
    // Check for king attacks
    for (int i = 0; i < 8; i++) {
        int sq = target_pos + QUEEN_DIRECTIONS[i];
        if (is_valid_king_move(target_pos, sq)) {
            if (GET_PIECE_TYPE(squares[sq]) == PIECE_KING && GET_COLOR(squares[sq]) == attacker_color) {
                return true;
            }
        }
    }
    
    // Check for rook/queen attacks (straight lines)
    for (int d = 0; d < 4; d++) {
        int direction = ROOK_DIRECTIONS[d];
        int sq = target_pos + direction;
        
        while (is_valid_square(sq) && !does_wrap(sq - direction, sq, direction)) {
            uint8_t piece = squares[sq];
            if (!IS_EMPTY(piece)) {
                if (GET_COLOR(piece) == attacker_color) {
                    uint8_t type = GET_PIECE_TYPE(piece);
                    if (type == PIECE_ROOK || type == PIECE_QUEEN) {
                        return true;
                    }
                }
                break; // Blocked
            }
            sq += direction;
        }
    }
    
    // Check for bishop/queen attacks (diagonals)
    for (int d = 0; d < 4; d++) {
        int direction = BISHOP_DIRECTIONS[d];
        int sq = target_pos + direction;
        
        while (is_valid_square(sq) && !does_wrap(sq - direction, sq, direction)) {
            uint8_t piece = squares[sq];
            if (!IS_EMPTY(piece)) {
                if (GET_COLOR(piece) == attacker_color) {
                    uint8_t type = GET_PIECE_TYPE(piece);
                    if (type == PIECE_BISHOP || type == PIECE_QUEEN) {
                        return true;
                    }
                }
                break; // Blocked
            }
            sq += direction;
        }
    }
    
    return false;
}

bool Board::is_king_in_check(uint8_t color) const {
    uint8_t king_pos = find_king(color);
    if (king_pos == 255) return false;
    
    uint8_t enemy_color = (color == 0) ? 1 : 0;
    return is_square_attacked(king_pos, enemy_color);
}

bool Board::would_be_in_check_after_move(uint8_t from, uint8_t to, uint8_t color) {
    // Make the move temporarily
    uint8_t captured = squares[to];
    uint8_t moving_piece = squares[from];
    
    squares[to] = moving_piece;
    squares[from] = 0;
    
    // Handle en passant capture
    uint8_t ep_captured = 0;
    uint8_t ep_capture_sq = 255;
    if (GET_PIECE_TYPE(moving_piece) == PIECE_PAWN && to == en_passant_target) {
        int direction = (color == 0) ? DIRECTION_S : DIRECTION_N;
        ep_capture_sq = to + direction;
        ep_captured = squares[ep_capture_sq];
        squares[ep_capture_sq] = 0;
    }
    
    bool in_check = is_king_in_check(color);
    
    // Restore the board
    squares[from] = moving_piece;
    squares[to] = captured;
    if (ep_capture_sq != 255) {
        squares[ep_capture_sq] = ep_captured;
    }
    
    return in_check;
}

void Board::add_pawn_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    int direction = (color == COLOR_WHITE) ? DIRECTION_N : DIRECTION_S;
    int start_rank = (color == COLOR_WHITE) ? 1 : 6;
    int rank = pos / 8;
    
    // Single push
    int to = pos + direction;
    if (is_valid_square(to) && IS_EMPTY(squares[to])) {
        moves.append(to);
        
        // Double push from starting position
        if (rank == start_rank) {
            int to2 = pos + 2 * direction;
            if (is_valid_square(to2) && IS_EMPTY(squares[to2])) {
                moves.append(to2);
            }
        }
    }
    
    // Captures (including en passant)
    int capture_offsets[2] = { direction + DIRECTION_W, direction + DIRECTION_E };
    
    for (int i = 0; i < 2; i++) {
        int capture_to = pos + capture_offsets[i];
        
        if (!is_valid_square(capture_to)) continue;
        
        // Check for wrapping
        int from_file = pos % 8;
        int to_file = capture_to % 8;
        int file_diff = from_file - to_file;
        if (file_diff < 0) file_diff = -file_diff;
        if (file_diff != 1) continue; // Wrapped around
        
        uint8_t target = squares[capture_to];
        
        // Regular capture
        if (!IS_EMPTY(target) && GET_COLOR(target) != color) {
            moves.append(capture_to);
        }
        // En passant
        else if (capture_to == en_passant_target) {
            moves.append(capture_to);
        }
    }
}

void Board::add_knight_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    
    for (int i = 0; i < 8; i++) {
        int to = pos + KNIGHT_OFFSETS[i];
        
        if (!is_valid_knight_move(pos, to)) continue;
        
        uint8_t target = squares[to];
        if (IS_EMPTY(target) || GET_COLOR(target) != color) {
            moves.append(to);
        }
    }
}

void Board::add_sliding_moves(uint8_t pos, Array &moves, const int directions[][2], int num_directions) const {
    // This method is not used directly - we use specialized methods
    // Keeping for interface compatibility
}

void Board::add_bishop_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    
    for (int d = 0; d < 4; d++) {
        int direction = BISHOP_DIRECTIONS[d];
        int to = pos + direction;
        
        while (is_valid_square(to) && !does_wrap(to - direction, to, direction)) {
            uint8_t target = squares[to];
            
            if (IS_EMPTY(target)) {
                moves.append(to);
            } else {
                if (GET_COLOR(target) != color) {
                    moves.append(to);
                }
                break;
            }
            
            to += direction;
        }
    }
}

void Board::add_rook_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    
    for (int d = 0; d < 4; d++) {
        int direction = ROOK_DIRECTIONS[d];
        int to = pos + direction;
        
        while (is_valid_square(to) && !does_wrap(to - direction, to, direction)) {
            uint8_t target = squares[to];
            
            if (IS_EMPTY(target)) {
                moves.append(to);
            } else {
                if (GET_COLOR(target) != color) {
                    moves.append(to);
                }
                break;
            }
            
            to += direction;
        }
    }
}

void Board::add_queen_moves(uint8_t pos, Array &moves) const {
    add_rook_moves(pos, moves);
    add_bishop_moves(pos, moves);
}

void Board::add_king_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    
    for (int d = 0; d < 8; d++) {
        int to = pos + QUEEN_DIRECTIONS[d];
        
        if (!is_valid_king_move(pos, to)) continue;
        
        uint8_t target = squares[to];
        if (IS_EMPTY(target) || GET_COLOR(target) != color) {
            moves.append(to);
        }
    }
}

bool Board::can_castle_kingside(uint8_t color) const {
    int rights_idx = (color == 0) ? 0 : 2;
    if (!castling_rights[rights_idx]) return false;
    
    int king_pos = (color == 0) ? 4 : 60;
    int rook_pos = (color == 0) ? 7 : 63;
    
    // Check squares between are empty
    if (!IS_EMPTY(squares[king_pos + 1]) || !IS_EMPTY(squares[king_pos + 2])) {
        return false;
    }
    
    // Check king and squares it passes through are not attacked
    uint8_t enemy_color = (color == 0) ? 1 : 0;
    if (is_square_attacked(king_pos, enemy_color)) return false;
    if (is_square_attacked(king_pos + 1, enemy_color)) return false;
    if (is_square_attacked(king_pos + 2, enemy_color)) return false;
    
    return true;
}

bool Board::can_castle_queenside(uint8_t color) const {
    int rights_idx = (color == 0) ? 1 : 3;
    if (!castling_rights[rights_idx]) return false;
    
    int king_pos = (color == 0) ? 4 : 60;
    
    // Check squares between are empty
    if (!IS_EMPTY(squares[king_pos - 1]) || !IS_EMPTY(squares[king_pos - 2]) || !IS_EMPTY(squares[king_pos - 3])) {
        return false;
    }
    
    // Check king and squares it passes through are not attacked
    uint8_t enemy_color = (color == 0) ? 1 : 0;
    if (is_square_attacked(king_pos, enemy_color)) return false;
    if (is_square_attacked(king_pos - 1, enemy_color)) return false;
    if (is_square_attacked(king_pos - 2, enemy_color)) return false;
    
    return true;
}

void Board::add_castling_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color_val = GET_COLOR(piece);
    uint8_t color = (color_val == COLOR_WHITE) ? 0 : 1;
    
    if (can_castle_kingside(color)) {
        moves.append(pos + 2);
    }
    if (can_castle_queenside(color)) {
        moves.append(pos - 2);
    }
}

Array Board::get_pseudo_legal_moves_for_piece(uint8_t pos) const {
    Array moves;
    
    if (pos >= 64) return moves;
    
    uint8_t piece = squares[pos];
    if (IS_EMPTY(piece)) return moves;
    
    uint8_t piece_type = GET_PIECE_TYPE(piece);
    
    switch (piece_type) {
        case PIECE_PAWN:
            add_pawn_moves(pos, moves);
            break;
        case PIECE_KNIGHT:
            add_knight_moves(pos, moves);
            break;
        case PIECE_BISHOP:
            add_bishop_moves(pos, moves);
            break;
        case PIECE_ROOK:
            add_rook_moves(pos, moves);
            break;
        case PIECE_QUEEN:
            add_queen_moves(pos, moves);
            break;
        case PIECE_KING:
            add_king_moves(pos, moves);
            add_castling_moves(pos, moves);
            break;
    }
    
    return moves;
}

void Board::make_move_internal(uint8_t from, uint8_t to, Move &move_record) {
    uint8_t moving_piece = squares[from];
    uint8_t piece_type = GET_PIECE_TYPE(moving_piece);
    uint8_t color = GET_COLOR(moving_piece);
    
    // Record move info
    move_record.from = from;
    move_record.to = to;
    move_record.captured_piece = squares[to];
    move_record.promotion_piece = 0;
    move_record.is_castling = false;
    move_record.is_en_passant = false;
    move_record.en_passant_target_before = en_passant_target;
    move_record.halfmove_clock_before = halfmove_clock;
    for (int i = 0; i < 4; i++) {
        move_record.castling_rights_before[i] = castling_rights[i];
    }
    
    // Handle en passant capture
    if (piece_type == PIECE_PAWN && to == en_passant_target) {
        move_record.is_en_passant = true;
        int capture_sq = to + ((color == COLOR_WHITE) ? DIRECTION_S : DIRECTION_N);
        move_record.captured_piece = squares[capture_sq];
        squares[capture_sq] = 0;
    }
    
    // Handle castling
    if (piece_type == PIECE_KING) {
        int move_dist = (int)to - (int)from;
        
        if (move_dist == 2) { // Kingside castling
            move_record.is_castling = true;
            // Move rook
            uint8_t rook = squares[from + 3];
            squares[from + 3] = 0;
            squares[from + 1] = rook;
        } else if (move_dist == -2) { // Queenside castling
            move_record.is_castling = true;
            // Move rook
            uint8_t rook = squares[from - 4];
            squares[from - 4] = 0;
            squares[from - 1] = rook;
        }
    }
    
    // Move the piece
    squares[to] = moving_piece;
    squares[from] = 0;
    
    // Update en passant target
    en_passant_target = 255;
    if (piece_type == PIECE_PAWN) {
        int move_dist = (int)to - (int)from;
        if (move_dist == 16 || move_dist == -16) {
            en_passant_target = (from + to) / 2;
        }
    }
    
    // Update castling rights
    // If king moves
    if (piece_type == PIECE_KING) {
        if (color == COLOR_WHITE) {
            castling_rights[0] = false;
            castling_rights[1] = false;
        } else {
            castling_rights[2] = false;
            castling_rights[3] = false;
        }
    }
    
    // If rook moves or is captured
    if (from == 0 || to == 0) castling_rights[1] = false;   // a1
    if (from == 7 || to == 7) castling_rights[0] = false;   // h1
    if (from == 56 || to == 56) castling_rights[3] = false; // a8
    if (from == 63 || to == 63) castling_rights[2] = false; // h8
    
    // Update halfmove clock
    if (piece_type == PIECE_PAWN || move_record.captured_piece != 0) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }
    
    // Update fullmove number (after black's move)
    if (color == COLOR_BLACK) {
        fullmove_number++;
    }
    
    // Switch turn
    turn = 1 - turn;
}

void Board::revert_move_internal(const Move &move) {
    uint8_t moving_piece = squares[move.to];
    uint8_t color = GET_COLOR(moving_piece);
    uint8_t piece_type = GET_PIECE_TYPE(moving_piece);
    
    // Handle promotion unmake
    if (move.promotion_piece != 0) {
        moving_piece = MAKE_PIECE(PIECE_PAWN, color);
    }
    
    // Move piece back
    squares[move.from] = moving_piece;
    squares[move.to] = move.captured_piece;
    
    // Handle en passant unmake
    if (move.is_en_passant) {
        squares[move.to] = 0; // The captured piece wasn't on 'to'
        int capture_sq = move.to + ((color == COLOR_WHITE) ? DIRECTION_S : DIRECTION_N);
        squares[capture_sq] = move.captured_piece;
    }
    
    // Handle castling unmake
    if (move.is_castling) {
        int move_dist = (int)move.to - (int)move.from;
        if (move_dist == 2) { // Kingside
            uint8_t rook = squares[move.from + 1];
            squares[move.from + 1] = 0;
            squares[move.from + 3] = rook;
        } else { // Queenside
            uint8_t rook = squares[move.from - 1];
            squares[move.from - 1] = 0;
            squares[move.from - 4] = rook;
        }
    }
    
    // Restore castling rights
    for (int i = 0; i < 4; i++) {
        castling_rights[i] = move.castling_rights_before[i];
    }
    
    // Restore en passant target
    en_passant_target = move.en_passant_target_before;
    
    // Restore halfmove clock
    halfmove_clock = move.halfmove_clock_before;
    
    // Switch turn back
    turn = 1 - turn;
    
    // Restore fullmove number
    if (color == COLOR_BLACK) {
        fullmove_number--;
    }
}

String Board::move_to_notation(const Move &move) const {
    String notation = "";
    
    notation += square_to_algebraic(move.from);
    notation += square_to_algebraic(move.to);
    
    if (move.promotion_piece != 0) {
        switch (GET_PIECE_TYPE(move.promotion_piece)) {
            case PIECE_QUEEN:  notation += "q"; break;
            case PIECE_ROOK:   notation += "r"; break;
            case PIECE_BISHOP: notation += "b"; break;
            case PIECE_KNIGHT: notation += "n"; break;
        }
    }
    
    return notation;
}

// Public API implementations

uint8_t Board::get_turn() const {
    return turn;
}

uint8_t Board::get_piece_on_square(uint8_t pos) const {
    if (pos >= 64) return 0;
    return squares[pos];
}

void Board::set_piece_on_square(uint8_t pos, uint8_t piece) {
    if (pos < 64) {
        squares[pos] = piece;
    }
}

void Board::setup_board(const String &fen_notation) {
    if (fen_notation.is_empty()) {
        initialize_starting_position();
    } else {
        parse_fen(fen_notation);
    }
}

String Board::get_fen() const {
    return generate_fen();
}

uint8_t Board::attempt_move(uint8_t start, uint8_t end) {
    if (start >= 64 || end >= 64) return 0;
    
    uint8_t piece = squares[start];
    if (IS_EMPTY(piece)) return 0;
    
    // Check it's the right player's turn
    uint8_t piece_color = (GET_COLOR(piece) == COLOR_WHITE) ? 0 : 1;
    if (piece_color != turn) return 0;
    
    // Get legal moves for this piece
    Array legal_moves = get_legal_moves_for_piece(start);
    
    bool is_legal = false;
    for (int i = 0; i < legal_moves.size(); i++) {
        if ((int)legal_moves[i] == end) {
            is_legal = true;
            break;
        }
    }
    
    if (!is_legal) return 0;
    
    // Check if this is a pawn promotion
    uint8_t piece_type = GET_PIECE_TYPE(piece);
    int end_rank = end / 8;
    
    if (piece_type == PIECE_PAWN && (end_rank == 0 || end_rank == 7)) {
        // Promotion pending
        promotion_pending = true;
        promotion_pending_from = start;
        promotion_pending_to = end;
        return 2; // Promotion pending
    }
    
    // Make the move
    Move move_record;
    make_move_internal(start, end, move_record);
    
    move_history.push_back(move_record);
    move_history_notation.push_back(move_to_notation(move_record));
    
    return 1; // Success
}

void Board::commit_promotion(const String &type_str) {
    if (!promotion_pending) return;
    
    uint8_t promotion_type = PIECE_QUEEN; // Default
    
    if (type_str.length() > 0) {
        char32_t c = type_str.to_lower()[0];
        switch (c) {
            case 'q': promotion_type = PIECE_QUEEN; break;
            case 'r': promotion_type = PIECE_ROOK; break;
            case 'b': promotion_type = PIECE_BISHOP; break;
            case 'n': promotion_type = PIECE_KNIGHT; break;
        }
    }
    
    uint8_t piece = squares[promotion_pending_from];
    uint8_t color = GET_COLOR(piece);
    
    // Make the move
    Move move_record;
    make_move_internal(promotion_pending_from, promotion_pending_to, move_record);
    
    // Apply promotion
    squares[promotion_pending_to] = MAKE_PIECE(promotion_type, color);
    move_record.promotion_piece = squares[promotion_pending_to];
    
    move_history.push_back(move_record);
    move_history_notation.push_back(move_to_notation(move_record));
    
    promotion_pending = false;
}

void Board::revert_move() {
    if (move_history.empty()) return;
    
    Move last_move = move_history.back();
    move_history.pop_back();
    move_history_notation.pop_back();
    
    revert_move_internal(last_move);
}

Array Board::get_moves() const {
    Array moves;
    for (size_t i = 0; i < move_history_notation.size(); i++) {
        moves.append(move_history_notation[i]);
    }
    return moves;
}

Array Board::get_all_possible_moves(uint8_t color) {
    Array all_moves;
    uint8_t target_color = (color == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    for (int sq = 0; sq < 64; sq++) {
        uint8_t piece = squares[sq];
        if (!IS_EMPTY(piece) && GET_COLOR(piece) == target_color) {
            Array piece_moves = get_legal_moves_for_piece(sq);
            for (int i = 0; i < piece_moves.size(); i++) {
                Dictionary move_dict;
                move_dict["from"] = sq;
                move_dict["to"] = (int)piece_moves[i];
                all_moves.append(move_dict);
            }
        }
    }
    
    return all_moves;
}

Array Board::get_legal_moves_for_piece(uint8_t square) {
    Array legal_moves;
    
    if (square >= 64) return legal_moves;
    
    uint8_t piece = squares[square];
    if (IS_EMPTY(piece)) return legal_moves;
    
    uint8_t color_val = GET_COLOR(piece);
    uint8_t color = (color_val == COLOR_WHITE) ? 0 : 1;
    
    Array pseudo_moves = get_pseudo_legal_moves_for_piece(square);
    
    for (int i = 0; i < pseudo_moves.size(); i++) {
        uint8_t to = (int)pseudo_moves[i];
        
        if (!would_be_in_check_after_move(square, to, color)) {
            legal_moves.append(to);
        }
    }
    
    return legal_moves;
}

uint32_t Board::count_all_moves(uint8_t depth) {
    // Base case: at depth 0, count this position as 1
    if (depth == 0) {
        return 1;
    }
    
    uint32_t count = 0;
    uint8_t current_color = turn;
    uint8_t target_color = (current_color == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    // Iterate over all squares to find pieces of the current player
    for (int sq = 0; sq < 64; sq++) {
        uint8_t piece = squares[sq];
        if (IS_EMPTY(piece) || GET_COLOR(piece) != target_color) {
            continue;
        }
        
        // Get legal moves for this piece
        Array legal_moves = get_legal_moves_for_piece(sq);
        uint8_t piece_type = GET_PIECE_TYPE(piece);
        
        for (int i = 0; i < legal_moves.size(); i++) {
            uint8_t to = (int)legal_moves[i];
            int to_rank = to / 8;
            
            // Check if this is a pawn promotion
            bool is_promotion = (piece_type == PIECE_PAWN && (to_rank == 0 || to_rank == 7));
            
            if (is_promotion) {
                // Promotions count as 4 separate moves (Q, R, B, N)
                uint8_t promotion_pieces[4] = { PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT };
                
                for (int p = 0; p < 4; p++) {
                    Move move_record;
                    make_move_internal(sq, to, move_record);
                    
                    // Apply promotion
                    squares[to] = MAKE_PIECE(promotion_pieces[p], target_color);
                    move_record.promotion_piece = squares[to];
                    
                    // Recurse
                    count += count_all_moves(depth - 1);
                    
                    // Revert
                    revert_move_internal(move_record);
                }
            } else {
                // Normal move
                Move move_record;
                make_move_internal(sq, to, move_record);
                
                // Recurse
                count += count_all_moves(depth - 1);
                
                // Revert
                revert_move_internal(move_record);
            }
        }
    }
    
    return count;
}

void Board::make_move(uint8_t start, uint8_t end) {
    if (start >= 64 || end >= 64) return;
    
    uint8_t piece = squares[start];
    if (IS_EMPTY(piece)) return;
    
    // Check for promotion
    uint8_t piece_type = GET_PIECE_TYPE(piece);
    int end_rank = end / 8;
    
    Move move_record;
    make_move_internal(start, end, move_record);
    
    // Auto-promote to queen if needed
    if (piece_type == PIECE_PAWN && (end_rank == 0 || end_rank == 7)) {
        uint8_t color = GET_COLOR(piece);
        squares[end] = MAKE_PIECE(PIECE_QUEEN, color);
        move_record.promotion_piece = squares[end];
    }
    
    move_history.push_back(move_record);
    move_history_notation.push_back(move_to_notation(move_record));
}

bool Board::is_checkmate(uint8_t color) {
    if (!is_king_in_check(color)) return false;
    
    // Check if any legal move exists
    Array all_moves = get_all_possible_moves(color);
    return all_moves.size() == 0;
}

bool Board::is_stalemate(uint8_t color) {
    if (is_king_in_check(color)) return false;
    
    // Check if any legal move exists
    Array all_moves = get_all_possible_moves(color);
    return all_moves.size() == 0;
}

bool Board::is_check(uint8_t color) const {
    return is_king_in_check(color);
}

bool Board::is_game_over() {
    // Check for checkmate or stalemate
    if (is_checkmate(turn) || is_stalemate(turn)) return true;
    
    // 50-move rule
    if (halfmove_clock >= 100) return true;
    
    // TODO: Add threefold repetition detection
    // TODO: Add insufficient material detection
    
    return false;
}

int Board::get_game_result() {
    if (is_checkmate(0)) return 2; // Black wins
    if (is_checkmate(1)) return 1; // White wins
    
    if (is_stalemate(turn)) return 3; // Draw
    if (halfmove_clock >= 100) return 3; // Draw by 50-move rule
    
    return 0; // Game ongoing
}

Vector2i Board::pos_to_coords(uint8_t pos) const {
    if (pos >= 64) return Vector2i(-1, -1);
    return Vector2i(pos / 8, pos % 8); // (rank, file)
}

uint8_t Board::coords_to_pos(int rank, int file) const {
    if (rank < 0 || rank > 7 || file < 0 || file > 7) return 255;
    return rank * 8 + file;
}

String Board::square_to_algebraic(uint8_t pos) const {
    if (pos >= 64) return "";
    
    int file = pos % 8;
    int rank = pos / 8;
    
    String result = "";
    result += String::chr('a' + file);
    result += String::num_int64(rank + 1);
    
    return result;
}

uint8_t Board::algebraic_to_square(const String &algebraic) const {
    if (algebraic.length() < 2) return 255;
    
    char32_t file_char = algebraic[0];
    char32_t rank_char = algebraic[1];
    
    int file = file_char - 'a';
    int rank = rank_char - '1';
    
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return 255;
    
    return rank * 8 + file;
}