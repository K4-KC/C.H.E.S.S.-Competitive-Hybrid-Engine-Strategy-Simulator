#include "board_rules.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

BoardRules::BoardRules() {
    turn = 0;
    en_passant_target = Vector2i(-1, -1);
    promotion_pending = false;
    for(int x=0; x<8; x++) {
        for(int y=0; y<8; y++) board[x][y] = {EMPTY, NONE, false, false};
    }
}

BoardRules::~BoardRules() {}

void BoardRules::setup_board(const Array& custom_layout) {
    turn = 0;
    en_passant_target = Vector2i(-1, -1);
    promotion_pending = false;

    bool use_standard = custom_layout.is_empty();
    const char* std_layout[8][8] = {
        {"r1","n1","b1","q1","k1","b1","n1","r1"},
        {"p1","p1","p1","p1","p1","p1","p1","p1"},
        {"0", "0", "0", "0", "0", "0", "0", "0"},
        {"0", "0", "0", "0", "0", "0", "0", "0"},
        {"0", "0", "0", "0", "0", "0", "0", "0"},
        {"0", "0", "0", "0", "0", "0", "0", "0"},
        {"p0","p0","p0","p0","p0","p0","p0","p0"},
        {"r0","n0","b0","q0","k0","b0","n0","r0"}
    };

    for(int y=0; y<8; y++) {
        Array row = use_standard ? Array() : Array(custom_layout[y]);
        for(int x=0; x<8; x++) {
            String cell = use_standard ? std_layout[y][x] : (String)row[x];
            if (cell == "0") {
                board[x][y] = {EMPTY, NONE, false, false};
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
                board[x][y] = {pt, (PieceColor)color, false, true};
            }
        }
    }
}

Dictionary BoardRules::get_data_at(int x, int y) const {
    Dictionary d;
    if (!is_on_board(Vector2i(x, y))) return d;
    Piece p = board[x][y];
    if (!p.active) return d;

    String t = "";
    switch(p.type) {
        case PAWN: t = "p"; break;
        case ROOK: t = "r"; break;
        case KNIGHT: t = "n"; break;
        case BISHOP: t = "b"; break;
        case QUEEN: t = "q"; break;
        case KING: t = "k"; break;
        default: t = ""; break;
    }
    d["type"] = t;
    d["color"] = (int)p.color;
    return d;
}

// --- NEW FUNCTION: AI AGENT DATA ---
Array BoardRules::get_all_possible_moves(int color) {
    Array moves;
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            Piece p = board[x][y];
            if (!p.active || p.color != color) continue;
            
            // Check all squares for valid moves (Naive but functional for 8x8)
            // Optimization: bitboards or piece-specific move lists would go here
            for (int tx = 0; tx < 8; tx++) {
                for (int ty = 0; ty < 8; ty++) {
                    Vector2i start(x, y);
                    Vector2i end(tx, ty);
                    
                    if (is_valid_geometry(p, start, end)) {
                        if (!does_move_cause_self_check(start, end)) {
                            Dictionary move_data;
                            move_data["start"] = start;
                            move_data["end"] = end;
                            // Add extra metadata if the AI needs it (e.g., captured piece)
                            Piece target = board[tx][ty];
                            move_data["is_capture"] = target.active;
                            moves.append(move_data);
                        }
                    }
                }
            }
        }
    }
    return moves;
}

// --- NEW FUNCTION: UI HIGHLIGHTS ---
Array BoardRules::get_valid_moves_for_piece(Vector2i start_pos) {
    Array valid_targets;
    
    if (!is_on_board(start_pos)) return valid_targets;
    Piece p = board[start_pos.x][start_pos.y];
    if (!p.active) return valid_targets;
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            Vector2i target(x, y);
            if (is_valid_geometry(p, start_pos, target)) {
                if (!does_move_cause_self_check(start_pos, target)) {
                    valid_targets.append(target);
                }
            }
        }
    }
    return valid_targets;
}

int BoardRules::attempt_move(Vector2i start, Vector2i end) {
    if (promotion_pending) return 0;
    if (!is_on_board(start) || !is_on_board(end)) return 0;
    
    Piece p = board[start.x][start.y];
    if (!p.active || p.color != turn) return 0;
    
    if (!is_valid_geometry(p, start, end)) return 0;
    if (does_move_cause_self_check(start, end)) {
        return 0;
    }

    int promotion_row = (p.color == WHITE) ? 0 : 7;
    if (p.type == PAWN && end.y == promotion_row) {
        execute_move_internal(start, end, true);
        promotion_pending = true;
        promotion_square = end;
        return 2;
    }

    execute_move_internal(start, end, true);
    turn = 1 - turn;
    return 1;
}

void BoardRules::commit_promotion(String type_str) {
    if (!promotion_pending) return;
    PieceType pt = QUEEN;
    if (type_str == "r") pt = ROOK;
    else if (type_str == "b") pt = BISHOP;
    else if (type_str == "n") pt = KNIGHT;
    
    board[promotion_square.x][promotion_square.y].type = pt;
    promotion_pending = false;
    turn = 1 - turn;
}

int BoardRules::get_turn() const { return turn; }

// --- INTERNAL LOGIC --- (Simplified for brevity, same as previous logic)
void BoardRules::execute_move_internal(Vector2i start, Vector2i end, bool real_move) {
    Piece p = board[start.x][start.y];
    int move_dir = (p.color == WHITE) ? -1 : 1;
    
    // En Passant
    if (p.type == PAWN && end == en_passant_target) {
        board[end.x][end.y - move_dir] = {EMPTY, NONE, false, false};
    }
    // Castling
    if (p.type == KING && abs(end.x - start.x) > 1) {
        int rook_x = (end.x > start.x) ? 7 : 0;
        int rook_target_x = (end.x > start.x) ? 5 : 3;
        Piece rook = board[rook_x][end.y];
        board[rook_x][end.y] = {EMPTY, NONE, false, false};
        board[rook_target_x][end.y] = rook;
        board[rook_target_x][end.y].has_moved = true;
    }
    // Update En Passant Target
    if (real_move) {
        en_passant_target = Vector2i(-1, -1);
        if (p.type == PAWN && abs(end.y - start.y) == 2) {
            en_passant_target = Vector2i(start.x, start.y + move_dir);
        }
    }
    board[end.x][end.y] = p;
    board[end.x][end.y].has_moved = true;
    board[start.x][start.y] = {EMPTY, NONE, false, false};
}

bool BoardRules::is_valid_geometry(const Piece& p, Vector2i start, Vector2i end) const {
    int dx = end.x - start.x;
    int dy = end.y - start.y;
    int abs_dx = abs(dx);
    int abs_dy = abs(dy);
    Piece target = board[end.x][end.y];
    if (target.active && target.color == p.color) return false;
    int dir = (p.color == WHITE) ? -1 : 1;

    switch (p.type) {
        case PAWN:
            if (dx == 0 && dy == dir && !target.active) return true;
            if (dx == 0 && dy == dir * 2 && !p.has_moved && !target.active) 
                if (!board[start.x][start.y + dir].active) return true;
            if (abs_dx == 1 && dy == dir) {
                if (target.active) return true;
                if (end == en_passant_target) return true;
            }
            return false;
        case KING:
            if (abs_dx <= 1 && abs_dy <= 1) return true;
            if (abs_dy == 0 && abs_dx == 2 && !p.has_moved) {
                if (is_square_attacked(start, 1 - p.color)) return false;
                int rook_x = (dx > 0) ? 7 : 0;
                Piece rook = board[rook_x][start.y];
                if (!rook.active || rook.type != ROOK || rook.has_moved) return false;
                int step = (dx > 0) ? 1 : -1;
                for (int i = 1; i < 3; i++) {
                     Vector2i check_pos(start.x + (i * step), start.y);
                     if (board[check_pos.x][check_pos.y].active) return false;
                     if (is_square_attacked(check_pos, 1 - p.color)) return false;
                }
                return true;
            }
            return false;
        case KNIGHT: return (abs_dx == 2 && abs_dy == 1) || (abs_dx == 1 && abs_dy == 2);
        case ROOK: return (dx == 0 || dy == 0) && is_path_clear(start, end);
        case BISHOP: return abs_dx == abs_dy && is_path_clear(start, end);
        case QUEEN: return (dx == 0 || dy == 0 || abs_dx == abs_dy) && is_path_clear(start, end);
        default: return false;
    }
}

bool BoardRules::is_path_clear(Vector2i start, Vector2i end) const {
    int dx = (end.x - start.x) == 0 ? 0 : (end.x - start.x) > 0 ? 1 : -1;
    int dy = (end.y - start.y) == 0 ? 0 : (end.y - start.y) > 0 ? 1 : -1;
    Vector2i current = start + Vector2i(dx, dy);
    while (current != end) {
        if (board[current.x][current.y].active) return false;
        current += Vector2i(dx, dy);
    }
    return true;
}

bool BoardRules::is_square_attacked(Vector2i square, int by_color) const {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            Piece p = board[x][y];
            if (p.active && p.color == by_color) {
                if (p.type == PAWN) {
                    int dir = (p.color == WHITE) ? -1 : 1;
                    if (abs(square.x - x) == 1 && (square.y - y) == dir) return true;
                } 
                else if (p.type == KING) { if (abs(square.x - x) <= 1 && abs(square.y - y) <= 1) return true; }
                else if (is_valid_geometry(p, Vector2i(x, y), square)) return true;
            }
        }
    }
    return false;
}

bool BoardRules::does_move_cause_self_check(Vector2i start, Vector2i end) {
    Piece orig_start = board[start.x][start.y];
    Piece orig_end = board[end.x][end.y];
    board[end.x][end.y] = orig_start;
    board[start.x][start.y] = {EMPTY, NONE, false, false};
    bool check = is_in_check(orig_start.color);
    board[start.x][start.y] = orig_start;
    board[end.x][end.y] = orig_end;
    return check;
}

bool BoardRules::is_in_check(int color) const {
    Vector2i king_pos(-1, -1);
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board[x][y].active && board[x][y].type == KING && board[x][y].color == color) {
                king_pos = Vector2i(x, y);
                break;
            }
        }
    }
    return (king_pos.x != -1) && is_square_attacked(king_pos, 1 - color);
}

bool BoardRules::is_on_board(Vector2i pos) const { return pos.x >= 0 && pos.x < 8 && pos.y >= 0 && pos.y < 8; }

void BoardRules::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_board", "custom_layout"), &BoardRules::setup_board);
    ClassDB::bind_method(D_METHOD("get_data_at", "x", "y"), &BoardRules::get_data_at);
    ClassDB::bind_method(D_METHOD("attempt_move", "start", "end"), &BoardRules::attempt_move);
    ClassDB::bind_method(D_METHOD("commit_promotion", "type_str"), &BoardRules::commit_promotion);
    ClassDB::bind_method(D_METHOD("get_turn"), &BoardRules::get_turn);
    
    // Bind the new functions
    ClassDB::bind_method(D_METHOD("get_all_possible_moves", "color"), &BoardRules::get_all_possible_moves);
    ClassDB::bind_method(D_METHOD("get_valid_moves_for_piece", "start_pos"), &BoardRules::get_valid_moves_for_piece);
}
