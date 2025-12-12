#include "board.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <cctype>

Board::Board() : turn(0), en_passant_target(255), halfmove_clock(0), 
                 fullmove_number(1), promotion_pending(false),
                 promotion_pending_from(255), promotion_pending_to(255) {
    clear_board();
    castling_rights[0] = castling_rights[1] = castling_rights[2] = castling_rights[3] = true;
}

Board::~Board() {
    move_history.clear();
    move_history_notation.clear();
}

void Board::_bind_methods() {
    // Basic getters
    ClassDB::bind_method(D_METHOD("get_turn"), &Board::get_turn);
    ClassDB::bind_method(D_METHOD("get_piece_on_square", "pos"), &Board::get_piece_on_square);
    ClassDB::bind_method(D_METHOD("set_piece_on_square", "pos", "piece"), &Board::set_piece_on_square);
    
    // Board setup
    ClassDB::bind_method(D_METHOD("setup_board", "fen_notation"), &Board::setup_board);
    ClassDB::bind_method(D_METHOD("get_fen"), &Board::get_fen);
    
    // Move execution
    ClassDB::bind_method(D_METHOD("attempt_move", "start", "end"), &Board::attempt_move);
    ClassDB::bind_method(D_METHOD("commit_promotion", "type_str"), &Board::commit_promotion);
    ClassDB::bind_method(D_METHOD("revert_move"), &Board::revert_move);
    ClassDB::bind_method(D_METHOD("get_moves"), &Board::get_moves);
    
    // AI functions
    ClassDB::bind_method(D_METHOD("get_all_possible_moves", "color"), &Board::get_all_possible_moves);
    ClassDB::bind_method(D_METHOD("get_legal_moves_for_piece", "square"), &Board::get_legal_moves_for_piece);
    ClassDB::bind_method(D_METHOD("make_move", "start", "end"), &Board::make_move);
    
    // Game state
    ClassDB::bind_method(D_METHOD("is_checkmate", "color"), &Board::is_checkmate);
    ClassDB::bind_method(D_METHOD("is_stalemate", "color"), &Board::is_stalemate);
    ClassDB::bind_method(D_METHOD("is_check", "color"), &Board::is_check);
    ClassDB::bind_method(D_METHOD("is_game_over"), &Board::is_game_over);
    ClassDB::bind_method(D_METHOD("get_game_result"), &Board::get_game_result);
    
    // Utility
    ClassDB::bind_method(D_METHOD("pos_to_coords", "pos"), &Board::pos_to_coords);
    ClassDB::bind_method(D_METHOD("coords_to_pos", "rank", "file"), &Board::coords_to_pos);
    ClassDB::bind_method(D_METHOD("square_to_algebraic", "pos"), &Board::square_to_algebraic);
    ClassDB::bind_method(D_METHOD("algebraic_to_square", "algebraic"), &Board::algebraic_to_square);
}

void Board::_ready() {
    initialize_starting_position();
}

// ============================================================================
// BOARD INITIALIZATION
// ============================================================================

void Board::clear_board() {
    for (int i = 0; i < 64; i++) {
        squares[i] = PIECE_NONE;
    }
}

void Board::initialize_starting_position() {
    setup_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool Board::parse_fen(const String &fen) {
    clear_board();
    move_history.clear();
    move_history_notation.clear();
    
    CharString fen_chars = fen.utf8();
    const char* fen_str = fen_chars.get_data();
    
    int square = 56; // Start at a8 (top-left)
    int idx = 0;
    
    // Parse piece placement
    while (fen_str[idx] != ' ' && fen_str[idx] != '\0') {
        char c = fen_str[idx];
        
        if (c == '/') {
            square -= 16; // Move to start of previous rank
        } else if (c >= '1' && c <= '8') {
            square += (c - '0'); // Skip empty squares
        } else {
            uint8_t piece_type = PIECE_NONE;
            uint8_t color = COLOR_NONE;
            
            // Determine color
            if (c >= 'A' && c <= 'Z') {
                color = COLOR_WHITE;
                c = tolower(c);
            } else {
                color = COLOR_BLACK;
            }
            
            // Determine piece type
            switch (c) {
                case 'p': piece_type = PIECE_PAWN; break;
                case 'n': piece_type = PIECE_KNIGHT; break;
                case 'b': piece_type = PIECE_BISHOP; break;
                case 'r': piece_type = PIECE_ROOK; break;
                case 'q': piece_type = PIECE_QUEEN; break;
                case 'k': piece_type = PIECE_KING; break;
                default: return false;
            }
            
            if (square >= 0 && square < 64) {
                squares[square] = MAKE_PIECE(piece_type, color);
                square++;
            }
        }
        idx++;
    }
    
    if (fen_str[idx] == ' ') idx++;
    
    // Parse active color
    if (fen_str[idx] == 'w') {
        turn = 0;
    } else if (fen_str[idx] == 'b') {
        turn = 1;
    }
    idx += 2; // Skip color and space
    
    // Parse castling rights
    castling_rights[0] = castling_rights[1] = castling_rights[2] = castling_rights[3] = false;
    if (fen_str[idx] != '-') {
        while (fen_str[idx] != ' ' && fen_str[idx] != '\0') {
            switch (fen_str[idx]) {
                case 'K': castling_rights[0] = true; break; // White kingside
                case 'Q': castling_rights[1] = true; break; // White queenside
                case 'k': castling_rights[2] = true; break; // Black kingside
                case 'q': castling_rights[3] = true; break; // Black queenside
            }
            idx++;
        }
    } else {
        idx++;
    }
    
    if (fen_str[idx] == ' ') idx++;
    
    // Parse en passant target
    en_passant_target = 255;
    if (fen_str[idx] != '-') {
        if (fen_str[idx] >= 'a' && fen_str[idx] <= 'h' && 
            fen_str[idx+1] >= '1' && fen_str[idx+1] <= '8') {
            int file = fen_str[idx] - 'a';
            int rank = fen_str[idx+1] - '1';
            en_passant_target = make_pos(rank, file);
            idx += 2;
        }
    } else {
        idx++;
    }
    
    // Parse halfmove clock and fullmove number (optional)
    halfmove_clock = 0;
    fullmove_number = 1;
    
    return true;
}

String Board::generate_fen() const {
    String fen = "";
    
    // Piece placement
    for (int rank = 7; rank >= 0; rank--) {
        int empty_count = 0;
        for (int file = 0; file < 8; file++) {
            uint8_t square = squares[make_pos(rank, file)];
            
            if (IS_EMPTY(square)) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += String::num_int64(empty_count);
                    empty_count = 0;
                }
                
                char piece_char = ' ';
                uint8_t piece_type = GET_PIECE_TYPE(square);
                
                switch (piece_type) {
                    case PIECE_PAWN: piece_char = 'p'; break;
                    case PIECE_KNIGHT: piece_char = 'n'; break;
                    case PIECE_BISHOP: piece_char = 'b'; break;
                    case PIECE_ROOK: piece_char = 'r'; break;
                    case PIECE_QUEEN: piece_char = 'q'; break;
                    case PIECE_KING: piece_char = 'k'; break;
                }
                
                if (IS_WHITE(square)) {
                    piece_char = toupper(piece_char);
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
    fen += turn == 0 ? " w " : " b ";
    
    // Castling rights
    String castling = "";
    if (castling_rights[0]) castling += "K";
    if (castling_rights[1]) castling += "Q";
    if (castling_rights[2]) castling += "k";
    if (castling_rights[3]) castling += "q";
    if (castling.is_empty()) castling = "-";
    fen += castling + " ";
    
    // En passant
    if (en_passant_target != 255) {
        fen += square_to_algebraic(en_passant_target);
    } else {
        fen += "-";
    }
    
    // Halfmove and fullmove
    fen += " " + String::num_int64(halfmove_clock) + " " + String::num_int64(fullmove_number);
    
    return fen;
}

// ============================================================================
// MOVE GENERATION
// ============================================================================

void Board::add_pawn_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    int rank = get_rank(pos);
    int file = get_file(pos);
    int direction = IS_WHITE(piece) ? 1 : -1;
    int start_rank = IS_WHITE(piece) ? 1 : 6;
    
    // Forward move
    int new_rank = rank + direction;
    if (is_valid_pos(new_rank, file)) {
        uint8_t target = make_pos(new_rank, file);
        if (IS_EMPTY(squares[target])) {
            moves.append(target);
            
            // Double move from starting position
            if (rank == start_rank) {
                new_rank = rank + 2 * direction;
                target = make_pos(new_rank, file);
                if (IS_EMPTY(squares[target])) {
                    moves.append(target);
                }
            }
        }
    }
    
    // Captures
    for (int df = -1; df <= 1; df += 2) {
        int new_file = file + df;
        new_rank = rank + direction;
        
        if (is_valid_pos(new_rank, new_file)) {
            uint8_t target = make_pos(new_rank, new_file);
            uint8_t target_piece = squares[target];
            
            // Regular capture
            if (!IS_EMPTY(target_piece) && GET_COLOR(target_piece) != color) {
                moves.append(target);
            }
            
            // En passant
            if (target == en_passant_target) {
                moves.append(target);
            }
        }
    }
}

void Board::add_knight_moves(uint8_t pos, Array &moves) const {
    static const int knight_offsets[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };
    
    uint8_t color = GET_COLOR(squares[pos]);
    int rank = get_rank(pos);
    int file = get_file(pos);
    
    for (int i = 0; i < 8; i++) {
        int new_rank = rank + knight_offsets[i][0];
        int new_file = file + knight_offsets[i][1];
        
        if (is_valid_pos(new_rank, new_file)) {
            uint8_t target = make_pos(new_rank, new_file);
            uint8_t target_piece = squares[target];
            
            if (IS_EMPTY(target_piece) || GET_COLOR(target_piece) != color) {
                moves.append(target);
            }
        }
    }
}

void Board::add_sliding_moves(uint8_t pos, Array &moves, const int directions[][2], int num_directions) const {
    uint8_t color = GET_COLOR(squares[pos]);
    int rank = get_rank(pos);
    int file = get_file(pos);
    
    for (int dir = 0; dir < num_directions; dir++) {
        int dr = directions[dir][0];
        int df = directions[dir][1];
        
        int new_rank = rank + dr;
        int new_file = file + df;
        
        while (is_valid_pos(new_rank, new_file)) {
            uint8_t target = make_pos(new_rank, new_file);
            uint8_t target_piece = squares[target];
            
            if (IS_EMPTY(target_piece)) {
                moves.append(target);
            } else {
                if (GET_COLOR(target_piece) != color) {
                    moves.append(target);
                }
                break; // Can't move through pieces
            }
            
            new_rank += dr;
            new_file += df;
        }
    }
}

void Board::add_bishop_moves(uint8_t pos, Array &moves) const {
    static const int bishop_directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    add_sliding_moves(pos, moves, bishop_directions, 4);
}

void Board::add_rook_moves(uint8_t pos, Array &moves) const {
    static const int rook_directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    add_sliding_moves(pos, moves, rook_directions, 4);
}

void Board::add_queen_moves(uint8_t pos, Array &moves) const {
    static const int queen_directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    add_sliding_moves(pos, moves, queen_directions, 8);
}

void Board::add_king_moves(uint8_t pos, Array &moves) const {
    static const int king_offsets[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    
    uint8_t color = GET_COLOR(squares[pos]);
    int rank = get_rank(pos);
    int file = get_file(pos);
    
    for (int i = 0; i < 8; i++) {
        int new_rank = rank + king_offsets[i][0];
        int new_file = file + king_offsets[i][1];
        
        if (is_valid_pos(new_rank, new_file)) {
            uint8_t target = make_pos(new_rank, new_file);
            uint8_t target_piece = squares[target];
            
            if (IS_EMPTY(target_piece) || GET_COLOR(target_piece) != color) {
                moves.append(target);
            }
        }
    }
    
    // Add castling moves
    add_castling_moves(pos, moves);
}

void Board::add_castling_moves(uint8_t pos, Array &moves) const {
    uint8_t piece = squares[pos];
    uint8_t color = GET_COLOR(piece);
    
    if (can_castle_kingside(color)) {
        int rank = IS_WHITE(piece) ? 0 : 7;
        moves.append(make_pos(rank, 6));
    }
    
    if (can_castle_queenside(color)) {
        int rank = IS_WHITE(piece) ? 0 : 7;
        moves.append(make_pos(rank, 2));
    }
}

bool Board::can_castle_kingside(uint8_t color) const {
    int castle_index = (color == COLOR_WHITE) ? 0 : 2;
    if (!castling_rights[castle_index]) return false;
    
    int rank = (color == COLOR_WHITE) ? 0 : 7;
    uint8_t king_pos = make_pos(rank, 4);
    
    // Check if king is on starting square
    uint8_t king = squares[king_pos];
    if (GET_PIECE_TYPE(king) != PIECE_KING || GET_COLOR(king) != color) {
        return false;
    }
    
    // Check if squares between king and rook are empty
    if (!IS_EMPTY(squares[make_pos(rank, 5)]) || 
        !IS_EMPTY(squares[make_pos(rank, 6)])) {
        return false;
    }
    
    // Check if king is in check or passes through check
    if (is_square_attacked(king_pos, color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE)) {
        return false;
    }
    if (is_square_attacked(make_pos(rank, 5), color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE)) {
        return false;
    }
    if (is_square_attacked(make_pos(rank, 6), color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE)) {
        return false;
    }
    
    return true;
}

bool Board::can_castle_queenside(uint8_t color) const {
    int castle_index = (color == COLOR_WHITE) ? 1 : 3;
    if (!castling_rights[castle_index]) return false;
    
    int rank = (color == COLOR_WHITE) ? 0 : 7;
    uint8_t king_pos = make_pos(rank, 4);
    
    // Check if king is on starting square
    uint8_t king = squares[king_pos];
    if (GET_PIECE_TYPE(king) != PIECE_KING || GET_COLOR(king) != color) {
        return false;
    }
    
    // Check if squares between king and rook are empty
    if (!IS_EMPTY(squares[make_pos(rank, 1)]) || 
        !IS_EMPTY(squares[make_pos(rank, 2)]) ||
        !IS_EMPTY(squares[make_pos(rank, 3)])) {
        return false;
    }
    
    // Check if king is in check or passes through check
    if (is_square_attacked(king_pos, color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE)) {
        return false;
    }
    if (is_square_attacked(make_pos(rank, 3), color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE)) {
        return false;
    }
    if (is_square_attacked(make_pos(rank, 2), color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE)) {
        return false;
    }
    
    return true;
}

Array Board::get_pseudo_legal_moves_for_piece(uint8_t pos) const {
    Array moves;
    
    if (pos >= 64 || IS_EMPTY(squares[pos])) {
        return moves;
    }
    
    uint8_t piece = squares[pos];
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
            break;
    }
    
    return moves;
}

// ============================================================================
// MOVE VALIDATION AND CHECK DETECTION
// ============================================================================

uint8_t Board::find_king(uint8_t color) const {
    for (uint8_t i = 0; i < 64; i++) {
        uint8_t piece = squares[i];
        if (GET_PIECE_TYPE(piece) == PIECE_KING && GET_COLOR(piece) == color) {
            return i;
        }
    }
    return 255; // Should never happen in valid position
}

bool Board::is_square_attacked(uint8_t pos, uint8_t attacking_color) const {
    int rank = get_rank(pos);
    int file = get_file(pos);
    
    // Check for pawn attacks
    int pawn_direction = (attacking_color == COLOR_WHITE) ? 1 : -1;
    for (int df = -1; df <= 1; df += 2) {
        int attack_rank = rank - pawn_direction;
        int attack_file = file + df;
        
        if (is_valid_pos(attack_rank, attack_file)) {
            uint8_t attacker_pos = make_pos(attack_rank, attack_file);
            uint8_t attacker = squares[attacker_pos];
            
            if (GET_PIECE_TYPE(attacker) == PIECE_PAWN && 
                GET_COLOR(attacker) == attacking_color) {
                return true;
            }
        }
    }
    
    // Check for knight attacks
    static const int knight_offsets[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };
    
    for (int i = 0; i < 8; i++) {
        int attack_rank = rank + knight_offsets[i][0];
        int attack_file = file + knight_offsets[i][1];
        
        if (is_valid_pos(attack_rank, attack_file)) {
            uint8_t attacker_pos = make_pos(attack_rank, attack_file);
            uint8_t attacker = squares[attacker_pos];
            
            if (GET_PIECE_TYPE(attacker) == PIECE_KNIGHT && 
                GET_COLOR(attacker) == attacking_color) {
                return true;
            }
        }
    }
    
    // Check for sliding piece attacks (bishop, rook, queen)
    static const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},  // Rook directions
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}  // Bishop directions
    };
    
    for (int dir = 0; dir < 8; dir++) {
        int dr = directions[dir][0];
        int df = directions[dir][1];
        bool is_diagonal = (dir >= 4);
        
        int new_rank = rank + dr;
        int new_file = file + df;
        
        while (is_valid_pos(new_rank, new_file)) {
            uint8_t attacker_pos = make_pos(new_rank, new_file);
            uint8_t attacker = squares[attacker_pos];
            
            if (!IS_EMPTY(attacker)) {
                if (GET_COLOR(attacker) == attacking_color) {
                    uint8_t attacker_type = GET_PIECE_TYPE(attacker);
                    
                    if (attacker_type == PIECE_QUEEN) {
                        return true;
                    }
                    if (is_diagonal && attacker_type == PIECE_BISHOP) {
                        return true;
                    }
                    if (!is_diagonal && attacker_type == PIECE_ROOK) {
                        return true;
                    }
                }
                break;
            }
            
            new_rank += dr;
            new_file += df;
        }
    }
    
    // Check for king attacks
    static const int king_offsets[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    
    for (int i = 0; i < 8; i++) {
        int attack_rank = rank + king_offsets[i][0];
        int attack_file = file + king_offsets[i][1];
        
        if (is_valid_pos(attack_rank, attack_file)) {
            uint8_t attacker_pos = make_pos(attack_rank, attack_file);
            uint8_t attacker = squares[attacker_pos];
            
            if (GET_PIECE_TYPE(attacker) == PIECE_KING && 
                GET_COLOR(attacker) == attacking_color) {
                return true;
            }
        }
    }
    
    return false;
}

bool Board::is_king_in_check(uint8_t color) const {
    uint8_t king_pos = find_king(color);
    if (king_pos == 255) return false;
    
    uint8_t enemy_color = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    return is_square_attacked(king_pos, enemy_color);
}

bool Board::would_be_in_check_after_move(uint8_t from, uint8_t to, uint8_t color) {
    // Make the move temporarily
    uint8_t captured = squares[to];
    uint8_t moving_piece = squares[from];
    
    squares[to] = moving_piece;
    squares[from] = PIECE_NONE;
    
    // Check if king would be in check
    bool in_check = is_king_in_check(color);
    
    // Undo the move
    squares[from] = moving_piece;
    squares[to] = captured;
    
    return in_check;
}

Array Board::get_legal_moves_for_piece(uint8_t square) {
    Array legal_moves;
    
    if (square >= 64 || IS_EMPTY(squares[square])) {
        return legal_moves;
    }
    
    uint8_t piece = squares[square];
    uint8_t color = GET_COLOR(piece);
    
    // Get pseudo-legal moves
    Array pseudo_legal = get_pseudo_legal_moves_for_piece(square);
    
    // Filter out moves that would leave king in check
    for (int i = 0; i < pseudo_legal.size(); i++) {
        uint8_t to = pseudo_legal[i];
        
        if (!would_be_in_check_after_move(square, to, color)) {
            legal_moves.append(to);
        }
    }
    
    return legal_moves;
}

Array Board::get_all_possible_moves(uint8_t color) {
    Array all_moves;
    uint8_t piece_color = (color == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    for (uint8_t pos = 0; pos < 64; pos++) {
        uint8_t piece = squares[pos];
        if (!IS_EMPTY(piece) && GET_COLOR(piece) == piece_color) {
            Array piece_moves = get_legal_moves_for_piece(pos);
            
            for (int i = 0; i < piece_moves.size(); i++) {
                Dictionary move_dict;
                move_dict["from"] = pos;
                move_dict["to"] = piece_moves[i];
                all_moves.append(move_dict);
            }
        }
    }
    
    return all_moves;
}

// ============================================================================
// MOVE EXECUTION
// ============================================================================

void Board::make_move_internal(uint8_t from, uint8_t to, Move &move_record) {
    move_record.from = from;
    move_record.to = to;
    move_record.captured_piece = squares[to];
    move_record.promotion_piece = PIECE_NONE;
    move_record.is_castling = false;
    move_record.is_en_passant = false;
    move_record.en_passant_target_before = en_passant_target;
    
    // Save castling rights
    for (int i = 0; i < 4; i++) {
        move_record.castling_rights_before[i] = castling_rights[i];
    }
    
    uint8_t moving_piece = squares[from];
    uint8_t piece_type = GET_PIECE_TYPE(moving_piece);
    uint8_t color = GET_COLOR(moving_piece);
    
    // Handle en passant
    if (piece_type == PIECE_PAWN && to == en_passant_target) {
        move_record.is_en_passant = true;
        int capture_rank = get_rank(to) + (IS_WHITE(moving_piece) ? -1 : 1);
        uint8_t capture_pos = make_pos(capture_rank, get_file(to));
        move_record.captured_piece = squares[capture_pos];
        squares[capture_pos] = PIECE_NONE;
    }
    
    // Handle castling
    if (piece_type == PIECE_KING && abs((int)get_file(to) - (int)get_file(from)) == 2) {
        move_record.is_castling = true;
        int rank = get_rank(from);
        
        if (get_file(to) == 6) { // Kingside
            uint8_t rook_from = make_pos(rank, 7);
            uint8_t rook_to = make_pos(rank, 5);
            squares[rook_to] = squares[rook_from];
            squares[rook_from] = PIECE_NONE;
        } else { // Queenside
            uint8_t rook_from = make_pos(rank, 0);
            uint8_t rook_to = make_pos(rank, 3);
            squares[rook_to] = squares[rook_from];
            squares[rook_from] = PIECE_NONE;
        }
    }
    
    // Update en passant target
    en_passant_target = 255;
    if (piece_type == PIECE_PAWN && abs((int)get_rank(to) - (int)get_rank(from)) == 2) {
        int ep_rank = (get_rank(from) + get_rank(to)) / 2;
        en_passant_target = make_pos(ep_rank, get_file(from));
    }
    
    // Update castling rights
    if (piece_type == PIECE_KING) {
        if (color == COLOR_WHITE) {
            castling_rights[0] = castling_rights[1] = false;
        } else {
            castling_rights[2] = castling_rights[3] = false;
        }
    }
    
    if (piece_type == PIECE_ROOK) {
        if (from == 0) castling_rights[1] = false; // White queenside
        else if (from == 7) castling_rights[0] = false; // White kingside
        else if (from == 56) castling_rights[3] = false; // Black queenside
        else if (from == 63) castling_rights[2] = false; // Black kingside
    }
    
    // If capturing a rook on its starting square, remove castling rights
    if (to == 0) castling_rights[1] = false;
    else if (to == 7) castling_rights[0] = false;
    else if (to == 56) castling_rights[3] = false;
    else if (to == 63) castling_rights[2] = false;
    
    // Make the move
    squares[to] = moving_piece;
    squares[from] = PIECE_NONE;
    
    // Update halfmove clock
    if (piece_type == PIECE_PAWN || move_record.captured_piece != PIECE_NONE) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }
    
    // Update fullmove number
    if (color == COLOR_BLACK) {
        fullmove_number++;
    }
    
    // Switch turn
    turn = 1 - turn;
}

void Board::unmake_move_internal(const Move &move) {
    // Switch turn back
    turn = 1 - turn;
    
    uint8_t moving_piece = squares[move.to];
    
    // Move piece back
    squares[move.from] = moving_piece;
    squares[move.to] = move.captured_piece;
    
    // Restore en passant capture
    if (move.is_en_passant) {
        int capture_rank = get_rank(move.to) + (IS_WHITE(moving_piece) ? -1 : 1);
        uint8_t capture_pos = make_pos(capture_rank, get_file(move.to));
        squares[capture_pos] = move.captured_piece;
        squares[move.to] = PIECE_NONE;
    }
    
    // Restore castling move
    if (move.is_castling) {
        int rank = get_rank(move.from);
        
        if (get_file(move.to) == 6) { // Kingside
            uint8_t rook_from = make_pos(rank, 7);
            uint8_t rook_to = make_pos(rank, 5);
            squares[rook_from] = squares[rook_to];
            squares[rook_to] = PIECE_NONE;
        } else { // Queenside
            uint8_t rook_from = make_pos(rank, 0);
            uint8_t rook_to = make_pos(rank, 3);
            squares[rook_from] = squares[rook_to];
            squares[rook_to] = PIECE_NONE;
        }
    }
    
    // Handle promotion undo
    if (move.promotion_piece != PIECE_NONE) {
        uint8_t color = GET_COLOR(moving_piece);
        squares[move.from] = MAKE_PIECE(PIECE_PAWN, color);
    }
    
    // Restore en passant target
    en_passant_target = move.en_passant_target_before;
    
    // Restore castling rights
    for (int i = 0; i < 4; i++) {
        castling_rights[i] = move.castling_rights_before[i];
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

uint8_t Board::get_turn() const {
    return turn;
}

uint8_t Board::get_piece_on_square(uint8_t pos) const {
    if (pos >= 64) return PIECE_NONE;
    return squares[pos];
}

void Board::set_piece_on_square(uint8_t pos, uint8_t piece) {
    if (pos < 64) {
        squares[pos] = piece;
    }
}

void Board::setup_board(const String &fen_notation) {
    parse_fen(fen_notation);
}

String Board::get_fen() const {
    return generate_fen();
}

uint8_t Board::attempt_move(uint8_t start, uint8_t end) {
    if (start >= 64 || end >= 64) return 0;
    
    uint8_t piece = squares[start];
    if (IS_EMPTY(piece)) return 0;
    
    uint8_t color = GET_COLOR(piece);
    uint8_t expected_color = (turn == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    if (color != expected_color) return 0;
    
    // Check if move is legal
    Array legal_moves = get_legal_moves_for_piece(start);
    bool is_legal = false;
    
    for (int i = 0; i < legal_moves.size(); i++) {
        if ((uint8_t)legal_moves[i] == end) {
            is_legal = true;
            break;
        }
    }
    
    if (!is_legal) return 0;
    
    // Check for promotion
    uint8_t piece_type = GET_PIECE_TYPE(piece);
    if (piece_type == PIECE_PAWN) {
        int end_rank = get_rank(end);
        if ((IS_WHITE(piece) && end_rank == 7) || (IS_BLACK(piece) && end_rank == 0)) {
            // Promotion pending
            promotion_pending = true;
            promotion_pending_from = start;
            promotion_pending_to = end;
            return 2;
        }
    }
    
    // Make the move
    Move move_record;
    make_move_internal(start, end, move_record);
    move_history.push_back(move_record);
    move_history_notation.push_back(move_to_notation(move_record));
    
    return 1;
}

void Board::commit_promotion(const String &type_str) {
    if (!promotion_pending) return;
    
    CharString type_chars = type_str.utf8();
    char type_char = tolower(type_chars[0]);
    
    uint8_t promotion_type = PIECE_QUEEN; // Default
    
    switch (type_char) {
        case 'q': promotion_type = PIECE_QUEEN; break;
        case 'r': promotion_type = PIECE_ROOK; break;
        case 'b': promotion_type = PIECE_BISHOP; break;
        case 'n': promotion_type = PIECE_KNIGHT; break;
    }
    
    // Make the move with promotion
    Move move_record;
    make_move_internal(promotion_pending_from, promotion_pending_to, move_record);
    
    // Replace pawn with promoted piece
    uint8_t color = GET_COLOR(squares[promotion_pending_to]);
    squares[promotion_pending_to] = MAKE_PIECE(promotion_type, color);
    move_record.promotion_piece = promotion_type;
    
    move_history.push_back(move_record);
    move_history_notation.push_back(move_to_notation(move_record));
    
    promotion_pending = false;
}

void Board::revert_move() {
    if (move_history.empty()) return;
    
    Move last_move = move_history.back();
    unmake_move_internal(last_move);
    
    move_history.pop_back();
    move_history_notation.pop_back();
}

Array Board::get_moves() const {
    Array moves;
    for (size_t i = 0; i < move_history_notation.size(); i++) {
        moves.append(move_history_notation[i]);
    }
    return moves;
}

void Board::make_move(uint8_t start, uint8_t end) {
    Move move_record;
    make_move_internal(start, end, move_record);
    move_history.push_back(move_record);
    move_history_notation.push_back(move_to_notation(move_record));
}

bool Board::is_checkmate(uint8_t color) {
    uint8_t piece_color = (color == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    if (!is_king_in_check(piece_color)) {
        return false;
    }
    
    Array moves = get_all_possible_moves(color);
    return moves.size() == 0;
}

bool Board::is_stalemate(uint8_t color) {
    uint8_t piece_color = (color == 0) ? COLOR_WHITE : COLOR_BLACK;
    
    if (is_king_in_check(piece_color)) {
        return false;
    }
    
    Array moves = get_all_possible_moves(color);
    return moves.size() == 0;
}

bool Board::is_check(uint8_t color) const {
    uint8_t piece_color = (color == 0) ? COLOR_WHITE : COLOR_BLACK;
    return is_king_in_check(piece_color);
}

bool Board::is_game_over() {
    return is_checkmate(turn) || is_stalemate(turn) || halfmove_clock >= 100;
}

int Board::get_game_result() {
    if (!is_game_over()) return 0;
    
    if (is_checkmate(turn)) {
        return (turn == 0) ? 2 : 1; // If white's turn and checkmate, black wins
    }
    
    return 3; // Draw
}

String Board::move_to_notation(const Move &move) const {
    String notation = square_to_algebraic(move.from) + square_to_algebraic(move.to);
    
    if (move.promotion_piece != PIECE_NONE) {
        switch (move.promotion_piece) {
            case PIECE_QUEEN: notation += "q"; break;
            case PIECE_ROOK: notation += "r"; break;
            case PIECE_BISHOP: notation += "b"; break;
            case PIECE_KNIGHT: notation += "n"; break;
        }
    }
    
    return notation;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

Vector2i Board::pos_to_coords(uint8_t pos) const {
    return Vector2i(get_rank(pos), get_file(pos));
}

uint8_t Board::coords_to_pos(int rank, int file) const {
    if (!is_valid_pos(rank, file)) return 255;
    return make_pos(rank, file);
}

String Board::square_to_algebraic(uint8_t pos) const {
    if (pos >= 64) return "";
    
    int file = get_file(pos);
    int rank = get_rank(pos);
    
    String result = "";
    result += String::chr('a' + file);
    result += String::chr('1' + rank);
    
    return result;
}

uint8_t Board::algebraic_to_square(const String &algebraic) const {
    if (algebraic.length() < 2) return 255;
    
    CharString chars = algebraic.utf8();
    char file_char = tolower(chars[0]);
    char rank_char = chars[1];
    
    if (file_char < 'a' || file_char > 'h') return 255;
    if (rank_char < '1' || rank_char > '8') return 255;
    
    int file = file_char - 'a';
    int rank = rank_char - '1';
    
    return make_pos(rank, file);
}