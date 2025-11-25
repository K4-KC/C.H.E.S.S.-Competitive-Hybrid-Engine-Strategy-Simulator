import numpy as np
import chess

# def FEN_to_board(pos):
#     board_part = pos.split(' ')[0]
#     rows = board_part.split('/')
    
#     board = []
    
#     for row in rows:
#         board_row = []
#         for char in row:
#             if char.isdigit():
#                 board_row.extend(['0'] * int(char))
#             else:
#                 board_row.append(char)
#         board.append(board_row)
    
#     return board

# def board_to_FEN(board, pos='rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', 
#                  move=False, pawn_move=True, castle_rights='KQkq', en_passant='-'):

#     pos_part = [pos.split(' ')[i] for i in range(1, 6)]
#     if move:
#         pos_part[0] = 'w' if pos_part[0] == 'b' else 'b'
#         pos_part[1] = castle_rights
#         pos_part[2] = en_passant
#         pos_part[3] = '0' if pawn_move else str(int(pos_part[3]) + 1)
#         pos_part[4] = str(int(pos_part[4]) + 1) if pos_part[0] == 'w' else pos_part[4]
#     pos_part = " ".join(pos_part)

#     fen_rows = []
    
#     for row in board:
#         fen_row = ""
#         empty_count = 0
        
#         for cell in row:
#             if cell == '0':
#                 empty_count += 1
#             else:
#                 if empty_count > 0:
#                     fen_row += str(empty_count)
#                     empty_count = 0
#                 fen_row += cell
        
#         if empty_count > 0:
#             fen_row += str(empty_count)
        
#         fen_rows.append(fen_row)
    
#     fen_position = "/".join(fen_rows)
    
#     return fen_position + " " + pos_part

def get_color(piece):
    if piece.isupper():
        return True
    elif piece.islower():
        return False
    else: return None

def get_territory(pos, board, color):
    territory = [[False for i in range(8)] for j in range(8)]

    for i in range(8):
        for j in range(8):
            piece = board[j][i]
            if color:
                if piece == 'P':
                    for square in get_pawn_territory(board, True, i, j):
                        territory[square[1]][square[0]] = True
                elif piece == 'R':
                    for square in get_rook_territory(board, True, i, j):
                        territory[square[1]][square[0]] = True
                elif piece == 'N':
                    for square in get_knight_territory(board, True, i, j):
                        territory[square[1]][square[0]] = True
                elif piece == 'B':
                    for square in get_bishop_territory(board, True, i, j):
                        territory[square[1]][square[0]] = True
                elif piece == 'Q':
                    for square in get_queen_territory(board, True, i, j):
                        territory[square[1]][square[0]] = True
                elif piece == 'K':
                    for square in get_king_territory(board, True, i, j):
                        territory[square[1]][square[0]] = True
            
            elif piece == 'p':
                for square in get_pawn_territory(board, False, i, j):
                    territory[square[1]][square[0]] = True
            elif piece == 'r':
                for square in get_rook_territory(board, False, i, j):
                    territory[square[1]][square[0]] = True
            elif piece == 'n':
                for square in get_knight_territory(board, False, i, j):
                    territory[square[1]][square[0]] = True
            elif piece == 'b':
                for square in get_bishop_territory(board, False, i, j):
                    territory[square[1]][square[0]] = True
            elif piece == 'q':
                for square in get_queen_territory(board, False, i, j):
                    territory[square[1]][square[0]] = True
            elif piece == 'k':
                for square in get_king_territory(board, False, i, j):
                    territory[square[1]][square[0]] = True

    return territory

def get_pawn_territory(board, color, x, y):
    territory = []

    if color:
        if y > 0:
            if x > 0: territory.append((x-1, y-1))
            if x < 7: territory.append((x+1, y-1))
    else:
        if y < 7:
            if x > 0: territory.append((x-1, y+1))
            if x < 7: territory.append((x+1, y+1))

    return territory

def get_rook_territory(board, color, x, y):
    territory = []

    for i in range(1, x+1):
        territory.append((x-i, y))
        if board[y][x-i] != '0':
            if (board[y][x-i] == 'k' and color) or (board[y][x-i] == 'K' and not color): 
                if i != x: territory.append((x-i-1, y))
            break

    for i in range(x+1, 8):
        territory.append((i, y))
        if board[y][i] != '0':
            if (board[y][i] == 'k' and color) or (board[y][i] == 'K' and not color):
                if i != 7: territory.append((i+1, y))
            break
    
    for i in range(1, y+1):
        territory.append((x, y-i))
        if board[y-i][x] != '0':
            if (board[y-i][x] == 'k' and color) or (board[y-i][x] == 'K' and not color):
                if i != y: territory.append((x, y-i-1))
            break

    for i in range(y+1, 8):
        territory.append((x, i))
        if board[i][x] != '0':
            if (board[i][x] == 'k' and color) or (board[i][x] == 'K' and not color):
                if i != 7: territory.append((x, i+1))
            break

    return territory

def get_knight_territory(board, color, x, y):
    territory = []

    if x > 0:
        if x > 1:
            if y > 0: territory.append((x-2, y-1))
            if y < 7: territory.append((x-2, y+1))
        if y > 1: territory.append((x-1, y-2))
        if y < 6: territory.append((x-1, y+2))
    if x < 7:
        if x < 6:
            if y > 0: territory.append((x+2, y-1))
            if y < 7: territory.append((x+2, y+1))
        if y > 1: territory.append((x+1, y-2))
        if y < 6: territory.append((x+1, y+2))
    
    return territory

def get_bishop_territory(board, color, x, y):
    territory = []

    diag_bott_rght, diag_bott_left, diag_top_rght, diag_top_left = True, True, True, True
    for i in range(1, 8):
        if diag_bott_rght and x+i < 8 and y+i < 8:
            territory.append((x+i, y+i))
            if board[y+i][x+i] != '0':
                if (board[y+i][x+i] != 'k' and color) or (board[y+i][x+i] != 'K' and not color): diag_bott_rght = False  

        if diag_bott_left and x+i < 8 and y-i >= 0:
            territory.append((x+i, y-i))
            if board[y-i][x+i] != '0':
                if (board[y-i][x+i] != 'k' and color) or (board[y-i][x+i] != 'K' and not color): diag_bott_left = False

        if diag_top_rght and x-i >= 0 and y+i < 8:
            territory.append((x-i, y+i))
            if board[y+i][x-i] != '0':
                if (board[y+i][x-i] != 'k' and color) or (board[y+i][x-i] != 'K' and not color): diag_top_rght = False

        if diag_top_left and x-i >= 0 and y-i >= 0:
            territory.append((x-i, y-i))
            if board[y-i][x-i] != '0':
                if (board[y-i][x-i] != 'k' and color) or (board[y-i][x-i] != 'K' and not color): diag_top_left = False

    return territory

def get_queen_territory(board, color, x, y):
    return get_rook_territory(board, color, x, y) + get_bishop_territory(board, color, x, y)

def get_king_territory(board, color, x, y):
    territory = []

    if x > 0: territory.append((x-1, y))
    if x < 7: territory.append((x+1, y))
    if y > 0: territory.append((x, y-1))
    if y < 7: territory.append((x, y+1))
    if x > 0 and y > 0: territory.append((x-1, y-1))
    if x > 0 and y < 7: territory.append((x-1, y+1))
    if x < 7 and y > 0: territory.append((x+1, y-1))
    if x < 7 and y < 7: territory.append((x+1, y+1))

    return territory

def get_moves(pos, board):
    color = True if pos.split(' ')[1] == 'w' else False
    castle_rights, en_passant = pos.split(' ')[2:4]
    territory = get_territory(pos, board, not color)
    
    # check and pin
    x, y = get_king_position(board, color)
    check, check_flag = [], False
    pin = []
    if color:
        if territory[y][x]:
        
            if x > 0:
                if board[y-1][x-1] == 'p':
                    check.append((x-1, y-1))
                    check_flag = True
                if x > 1:
                    if y > 0 and board[y-1][x-2] == 'n':
                        check.append((x-2, y-1))
                        check_flag = True
                    if y < 7 and board[y+1][x-2] == 'n':
                        check.append((x-2, y+1))
                        check_flag = True
                if y > 1 and board[y-2][x-1] == 'n':
                    check.append((x-1, y-2))
                    check_flag = True
                if y < 6 and board[y+2][x-1] == 'n':
                    check.append((x-1, y+2))
                    check_flag = True
            if x < 7:
                if board[y-1][x+1] == 'p':
                    check.append((x+1, y-1))
                    check_flag = True
                if x < 6:
                    if y > 0 and board[y-1][x+2] == 'n':
                        check.append((x+2, y-1))
                        check_flag = True
                    if y < 7 and board[y+1][x+2] == 'n':
                        check.append((x+2, y+1))
                        check_flag = True
                if y > 1 and board[y-2][x+1] == 'n':
                    check.append((x+1, y-2))
                    check_flag = True
                if y < 6 and board[y+2][x+1] == 'n':
                    check.append((x+1, y+2))
                    check_flag = True
        
        diag_bott_rght, diag_bott_left, diag_top_rght, diag_top_left = True, True, True, True
        bott, rght, top, left = True, True, True, True
        diag_bott_rght_squares, diag_bott_left_squares, diag_top_rght_squares, diag_top_left_squares = [], [], [] ,[]
        bott_squares, rght_squares, top_squares, left_squares = [], [], [], []
        diag_bott_rght_pin, diag_bott_left_pin, diag_top_rght_pin, diag_top_left_pin = (), (), (), ()
        bott_pin, rght_pin, top_pin, left_pin = (), (), (), ()
        for i in range(1, 8):
            
            if diag_bott_rght and x+i < 8 and y+i < 8:
                if board[y+i][x+i] == '0': diag_bott_rght_squares.append((x+i, y+i))
                elif board[y+i][x+i] == 'b' or board[y+i][x+i] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_bott_rght_pin:
                        diag_bott_rght_squares.append((x+i, y+i))
                        pin.append((diag_bott_rght_pin, [square for square in diag_bott_rght_squares]))
                    else:
                        for square in diag_bott_rght_squares: check.append(square)
                        check.append((x+i, y+i))
                        check_flag = True
                    diag_bott_rght = False
                elif get_color(board[y+i][x+i]) == color:
                    if diag_bott_rght_pin:
                        diag_bott_rght_pin = ()
                        diag_bott_rght = False
                    diag_bott_rght_squares.append((x+i, y+i))
                    diag_bott_rght_pin = (x+i, y+i)
                else: diag_bott_rght = False
                    
            if diag_bott_left and x+i < 8 and y-i >= 0:
                if board[y-i][x+i] == '0': diag_bott_left_squares.append((x+i, y-i))
                elif board[y-i][x+i] == 'b' or board[y-i][x+i] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_bott_left_pin:
                        diag_bott_left_squares.append((x+i, y-i))
                        pin.append((diag_bott_left_pin, [square for square in diag_bott_left_squares]))
                    else:
                        for square in diag_bott_left_squares: check.append(square)
                        check.append((x+i, y-i))
                        check_flag = True
                    diag_bott_left = False
                elif get_color(board[y-i][x+i]) == color:
                    if diag_bott_left_pin:
                        diag_bott_left_pin = ()
                        diag_bott_left = False
                    diag_bott_left_squares.append((x+i, y-i))
                    diag_bott_left_pin = (x+i, y-i)
                else: diag_bott_left = False

            if diag_top_rght and x-i >= 0 and y+i < 8:
                if board[y+i][x-i] == '0': diag_top_rght_squares.append((x-i, y+i))
                elif board[y+i][x-i] == 'b' or board[y+i][x-i] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_top_rght_pin:
                        diag_top_rght_squares.append((x-i, y+i))
                        pin.append((diag_top_rght_pin, [square for square in diag_top_rght_squares]))
                    else:
                        for square in diag_top_rght_squares: check.append(square)
                        check.append((x-i, y+i))
                        check_flag = True
                    diag_top_rght = False
                elif get_color(board[y+i][x-i]) == color:
                    if diag_top_rght_pin:
                        diag_top_rght_pin = ()
                        diag_top_rght = False
                    diag_top_rght_squares.append((x-i, y+i))
                    diag_top_rght_pin = (x-i, y+i)
                else: diag_top_rght = False

            if diag_top_left and x-i >= 0 and y-i >= 0:
                if board[y-i][x-i] == '0': diag_top_left_squares.append((x-i, y-i))
                elif board[y-i][x-i] == 'b' or board[y-i][x-i] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_top_left_pin:
                        diag_top_left_squares.append((x-i, y-i))
                        pin.append((diag_top_left_pin, [square for square in diag_top_left_squares]))
                    else:
                        for square in diag_top_left_squares: check.append(square)
                        check.append((x-i, y-i))
                        check_flag = True
                    diag_top_left = False
                elif get_color(board[y-i][x-i]) == color:
                    if diag_top_left_pin:
                        diag_top_left_pin = ()
                        diag_top_left = False
                    diag_top_left_squares.append((x-i, y-i))
                    diag_top_left_pin = (x-i, y-i)
                else: diag_top_left = False
            
            if bott and y+i < 8:
                if board[y+i][x] == '0': bott_squares.append((x, y+i))
                elif board[y+i][x] == 'r' or board[y+i][x] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if bott_pin:
                        bott_squares.append((x, y+i))
                        pin.append((bott_pin, [square for square in bott_squares]))
                    else:
                        for square in bott_squares: check.append(square)
                        check.append((x, y+i))
                        check_flag = True
                    bott = False
                elif get_color(board[y+i][x]) == color:
                    if bott_pin:
                        bott_pin = ()
                        bott = False
                    bott_squares.append((x, y+i))
                    bott_pin = (x, y+i)
                else: bott = False
            
            if rght and x+i < 8:
                if board[y][x+i] == '0': rght_squares.append((x+i, y))
                elif board[y][x+i] == 'r' or board[y][x+i] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if rght_pin:
                        rght_squares.append((x+i, y))
                        pin.append((rght_pin, [square for square in rght_squares]))
                    else:
                        for square in rght_squares: check.append(square)
                        check.append((x+i, y))
                        check_flag = True
                    rght = False
                elif get_color(board[y][x+i]) == color:
                    if rght_pin:
                        rght_pin = ()
                        rght = False
                    rght_squares.append((x+i, y))
                    rght_pin = (x+i, y)
                else: rght = False
            
            if top and y-i >= 0:
                if board[y-i][x] == '0': top_squares.append((x, y-i))
                elif board[y-i][x] == 'r' or board[y-i][x] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if top_pin:
                        top_squares.append((x, y-i))
                        pin.append((top_pin, [square for square in top_squares]))
                    else:
                        for square in top_squares: check.append(square)
                        check.append((x, y-i))
                        check_flag = True
                    top = False
                elif get_color(board[y-i][x]) == color:
                    if top_pin:
                        top_pin = ()
                        top = False
                    top_squares.append((x, y-i))
                    top_pin = (x, y-i)
                else: top = False
            
            if left and x-i >= 0:
                if board[y][x-i] == '0': left_squares.append((x-i, y))
                elif board[y][x-i] == 'r' or board[y][x-i] == 'q':
                    if check_flag:
                        check = [None]
                        break
                    if left_pin:
                        left_squares.append((x-i, y))
                        pin.append((left_pin, [square for square in left_squares]))
                    else:
                        for square in left_squares: check.append(square)
                        check.append((x-i, y))
                        check_flag = True
                    left = False
                elif get_color(board[y][x-i]) == color:
                    if left_pin:
                        left_pin = ()
                        left = False
                    left_squares.append((x-i, y))
                    left_pin = (x-i, y)
                else: left = False
            
    elif not color:
        if territory[y][x]:
            
            if x > 0:
                if board[y+1][x-1] == 'P':
                    check.append((x-1, y+1))
                    check_flag = True
                if x > 1:
                    if y > 0 and board[y-1][x-2] == 'N':
                        check.append((x-2, y-1))
                        check_flag = True
                    if y < 7 and board[y+1][x-2] == 'N':
                        check.append((x-2, y+1))
                        check_flag = True
                if y > 1 and board[y-2][x-1] == 'N':
                    check.append((x-1, y-2))
                    check_flag = True
                if y < 6 and board[y+2][x-1] == 'N':
                    check.append((x-1, y+2))
                    check_flag = True
            if x < 7:
                if board[y+1][x+1] == 'P':
                    check.append((x+1, y+1))
                    check_flag = True
                if x < 6:
                    if y > 0 and board[y-1][x+2] == 'N':
                        check.append((x+2, y-1))
                        check_flag = True
                    if y < 7 and board[y+1][x+2] == 'N':
                        check.append((x+2, y+1))
                        check_flag = True
                if y > 1 and board[y-2][x+1] == 'N':
                    check.append((x+1, y-2))
                    check_flag = True
                if y < 6 and board[y+2][x+1] == 'N':
                    check.append((x+1, y+2))
                    check_flag = True
            
        diag_bott_rght, diag_bott_left, diag_top_rght, diag_top_left = True, True, True, True
        bott, rght, top, left = True, True, True, True
        diag_bott_rght_squares, diag_bott_left_squares, diag_top_rght_squares, diag_top_left_squares = [], [], [] ,[]
        bott_squares, rght_squares, top_squares, left_squares = [], [], [], []
        diag_bott_rght_pin, diag_bott_left_pin, diag_top_rght_pin, diag_top_left_pin = (), (), (), ()
        bott_pin, rght_pin, top_pin, left_pin = (), (), (), ()
        for i in range(1, 8):
            
            if diag_bott_rght and x+i < 8 and y+i < 8:
                if board[y+i][x+i] == '0': diag_bott_rght_squares.append((x+i, y+i))
                elif board[y+i][x+i] == 'B' or board[y+i][x+i] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_bott_rght_pin:
                        diag_bott_rght_squares.append((x+i, y+i))
                        pin.append((diag_bott_rght_pin, [square for square in diag_bott_rght_squares]))
                    else:
                        for square in diag_bott_rght_squares: check.append(square)
                        check.append((x+i, y+i))
                        check_flag = True
                    diag_bott_rght = False
                elif get_color(board[y+i][x+i]) is color:
                    if diag_bott_rght_pin:
                        diag_bott_rght_pin = ()
                        diag_bott_rght = False
                    diag_bott_rght_squares.append((x+i, y+i))
                    diag_bott_rght_pin = (x+i, y+i)
                else: diag_bott_rght = False
            
            if diag_bott_left and x+i < 8 and y-i >= 0:
                if board[y-i][x+i] == '0': diag_bott_left_squares.append((x+i, y-i))
                elif board[y-i][x+i] == 'B' or board[y-i][x+i] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_bott_left_pin:
                        diag_bott_left_squares.append((x+i, y-i))
                        pin.append((diag_bott_left_pin, [square for square in diag_bott_left_squares]))
                    else:
                        for square in diag_bott_left_squares: check.append(square)
                        check.append((x+i, y-i))
                        check_flag = True
                    diag_bott_left = False
                elif get_color(board[y-i][x+i]) is color:
                    if diag_bott_left_pin:
                        diag_bott_left_pin = ()
                        diag_bott_left = False
                    diag_bott_left_squares.append((x+i, y-i))
                    diag_bott_left_pin = (x+i, y-i)
                else: diag_bott_left = False
            
            if diag_top_rght and x-i >= 0 and y+i < 8:
                if board[y+i][x-i] == '0': diag_top_rght_squares.append((x-i, y+i))
                elif board[y+i][x-i] == 'B' or board[y+i][x-i] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_top_rght_pin:
                        diag_top_rght_squares.append((x-i, y+i))
                        pin.append((diag_top_rght_pin, [square for square in diag_top_rght_squares]))
                    else:
                        for square in diag_top_rght_squares: check.append(square)
                        check.append((x-i, y+i))
                        check_flag = True
                    diag_top_rght = False
                elif get_color(board[y+i][x-i]) is color:
                    if diag_top_rght_pin:
                        diag_top_rght_pin = ()
                        diag_top_rght = False
                    diag_top_rght_squares.append((x-i, y+i))
                    diag_top_rght_pin = (x-i, y+i)
                else: diag_top_rght = False
            
            if diag_top_left and x-i >= 0 and y-i >= 0:
                if board[y-i][x-i] == '0':
                    diag_top_left_squares.append((x-i, y-i))
                elif board[y-i][x-i] == 'B' or board[y-i][x-i] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if diag_top_left_pin:
                        diag_top_left_squares.append((x-i, y-i))
                        pin.append((diag_top_left_pin, [square for square in diag_top_left_squares]))
                    else:
                        for square in diag_top_left_squares: check.append(square)
                        check.append((x-i, y-i))
                        check_flag = True
                    diag_top_left = False
                elif get_color(board[y-i][x-i]) is color:
                    if diag_top_left_pin:
                        diag_top_left_pin = ()
                        diag_top_left = False
                    diag_top_left_squares.append((x-i, y-i))
                    diag_top_left_pin = (x-i, y-i)
                else: diag_top_left = False
            
            if bott and y+i < 8:
                if board[y+i][x] == '0': bott_squares.append((x, y+i))
                elif board[y+i][x] == 'R' or board[y+i][x] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if bott_pin:
                        bott_squares.append((x, y+i))
                        pin.append((bott_pin, [square for square in bott_squares]))
                    else:
                        for square in bott_squares: check.append(square)
                        check.append((x, y+i))
                        check_flag = True
                    bott = False
                elif get_color(board[y+i][x]) is color:
                    if bott_pin:
                        bott_pin = ()
                        bott = False
                    bott_squares.append((x, y+i))
                    bott_pin = (x, y+i)
                else: bott = False
            
            if rght and x+i < 8:
                if board[y][x+i] == '0': rght_squares.append((x+i, y))
                elif board[y][x+i] == 'R' or board[y][x+i] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if rght_pin:
                        rght_squares.append((x+i, y))
                        pin.append((rght_pin, [square for square in rght_squares]))
                    else:
                        for square in rght_squares: check.append(square)
                        check.append((x+i, y))
                        check_flag = True
                    rght = False
                elif get_color(board[y][x+i]) is color:
                    if rght_pin:
                        rght_pin = ()
                        rght = False
                    rght_squares.append((x+i, y))
                    rght_pin = (x+i, y)
                else: rght = False
            
            if top and y-i >= 0:
                if board[y-i][x] == '0': top_squares.append((x, y-i))
                elif board[y-i][x] == 'R' or board[y-i][x] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if top_pin: pin.append((top_pin, [square for square in top_squares]))
                    else:
                        for square in top_squares:
                            top_squares.append(square)
                            check.append(square)
                        check.append((x, y-i))
                        check_flag = True
                    top = False
                elif get_color(board[y-i][x]) is color:
                    if top_pin:
                        top_pin = ()
                        top = False
                    top_squares.append((x, y-i))
                    top_pin = (x, y-i)
                else: top = False
            
            if left and x-i >= 0:
                if board[y][x-i] == '0': left_squares.append((x-i, y))
                elif board[y][x-i] == 'R' or board[y][x-i] == 'Q':
                    if check_flag:
                        check = [None]
                        break
                    if left_pin:
                        left_squares.append((x-i, y))
                        pin.append((left_pin, [square for square in left_squares]))
                    else:
                        for square in left_squares: check.append(square)
                        check.append((x-i, y))
                        check_flag = True
                    left = False
                elif get_color(board[y][x-i]) is color:
                    if left_pin:
                        left_pin = ()
                        left = False
                    left_squares.append((x-i, y))
                    left_pin = (x-i, y)
                else: left = False
            
    return [[get_moves_selected(color, board, territory, castle_rights, en_passant, check, pin, i, j) for i in range(8)]for j in range(8)]

def get_king_position(board, color=None):
    if color is None:
        found = False
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'K':
                    if found:
                        return ((i, j), found_pos)
                    found_pos = (i, j)
                    found = True
                elif board[j][i] == 'k':
                    if found:
                        return (found_pos, (i, j))
                    found_pos = (i, j)
                    found = True
                    
    for i in range(8):
        for j in range(8):
            if color:
                if board[j][i] == 'K':
                    return (i, j)
            else:
                if board[j][i] == 'k':
                    return (i, j)

def get_moves_selected(color, board, territory, castle_rights, en_passant, check, pin, x , y):
    piece = board[y][x]
    moves = []
    
    pinned = False
    for square in pin:
        if (x, y) == square[0]:
            pinned = True
            pinned_squares = square[1]
    
    if color:
        if piece == 'K': return get_king_moves(castle_rights, board, territory, color, x, y)
        
        elif piece == 'P':
            for move in get_pawn_moves(en_passant, board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'R':
            for move in get_rook_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'N':
            for move in get_knight_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'B':
            for move in get_bishop_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'Q':
            for move in get_queen_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
    
    else:
        if piece == 'k': return get_king_moves(castle_rights, board, territory, color, x, y)
        
        elif piece == 'p':
            for move in get_pawn_moves(en_passant, board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'r':
            for move in get_rook_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'n':
            for move in get_knight_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'b':
            for move in get_bishop_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None
        
        elif piece == 'q':
            for move in get_queen_moves(board, color, x, y): moves.append(move)
            if check:
                check_moves = moves[:]
                for square in check_moves:
                    moves.remove(square) if square not in check else None
            if pinned:
                pinned_moves = moves[:]
                for square in pinned_moves:
                    moves.remove(square) if square not in pinned_squares else None

    return moves

def get_pawn_moves(en_passant, board, color, x, y):
    moves = []

    if color:
        if y > 0:
            moves.append((x, y-1)) if board[y-1][x] == '0' else None
            if y == 6 and board[y-1][x] == '0':
                moves.append((x, y-2)) if board[y-2][x] == '0' else None
            # en passant
            elif en_passant != '-':
                if y == 3:
                    en_passant_pos = ord(en_passant[0])-97
                    if en_passant_pos == x+1:
                        moves.append((x+1, y-1))
                    elif en_passant_pos == x-1:
                        moves.append((x-1, y-1))
            if x > 0 and get_color(board[y-1][x-1]) is False: moves.append((x-1, y-1))
            if x < 7 and get_color(board[y-1][x+1]) is False: moves.append((x+1, y-1))
    else:
        if y < 7:
            moves.append((x, y+1)) if board[y+1][x] == '0' else None
            if y == 1 and board[y+1][x] == '0':
                moves.append((x, y+2)) if board[y+2][x] == '0' else None
            # en passant
            elif en_passant != '-':
                if y == 4:
                    en_passant_pos = ord(en_passant[0])-97
                    if en_passant_pos == x+1:
                        moves.append((x+1, y+1))
                    elif en_passant_pos == x-1:
                        moves.append((x-1, y+1))
            if x > 0 and get_color(board[y+1][x-1]): moves.append((x-1, y+1))
            if x < 7 and get_color(board[y+1][x+1]): moves.append((x+1, y+1))

    return moves

def get_rook_moves(board, color, x, y):
    moves = []

    for i in range(1, x+1):
        if board[y][x-i] != '0':
            if get_color(board[y][x-i]) is not color: moves.append((x-i, y))
            break
        else: moves.append((x-i, y))

    for i in range(x+1, 8):
        if board[y][i] != '0':
            if get_color(board[y][i]) is not color: moves.append((i, y))
            break
        else: moves.append((i, y))
    
    for i in range(1, y+1):
        if board[y-i][x] != '0':
            if get_color(board[y-i][x]) is not color: moves.append((x, y-i))
            break
        else: moves.append((x, y-i))

    for i in range(y+1, 8):
        if board[i][x] != '0':
            if get_color(board[i][x]) is not color: moves.append((x, i))
            break
        else: moves.append((x, i))

    return moves

def get_knight_moves(board, color, x, y):
    moves = []

    if x > 0:
        if x > 1:
            if y > 0: moves.append((x-2, y-1)) if get_color(board[y-1][x-2]) is not color else None
            if y < 7: moves.append((x-2, y+1)) if get_color(board[y+1][x-2]) is not color else None
        if y > 1: moves.append((x-1, y-2)) if get_color(board[y-2][x-1]) is not color else None
        if y < 6: moves.append((x-1, y+2)) if get_color(board[y+2][x-1]) is not color else None
    if x < 7:
        if x < 6:
            if y > 0: moves.append((x+2, y-1)) if get_color(board[y-1][x+2]) is not color else None
            if y < 7: moves.append((x+2, y+1)) if get_color(board[y+1][x+2]) is not color else None
        if y > 1: moves.append((x+1, y-2)) if get_color(board[y-2][x+1]) is not color else None
        if y < 6: moves.append((x+1, y+2)) if get_color(board[y+2][x+1]) is not color else None
    
    return moves

def get_bishop_moves(board, color, x, y):
    moves = []

    diag_bott_rght, diag_bott_left, diag_top_rght, diag_top_left = True, True, True, True
    for i in range(1, 8):
        if diag_bott_rght and x+i < 8 and y+i < 8:
            if board[y+i][x+i] == '0': moves.append((x+i, y+i))
            else:
                if get_color(board[y+i][x+i]) is not color: moves.append((x+i, y+i))
                diag_bott_rght = False        
        if diag_bott_left and x+i < 8 and y-i >= 0:
            if board[y-i][x+i] == '0': moves.append((x+i, y-i))
            else:
                if get_color(board[y-i][x+i]) is not color: moves.append((x+i, y-i))
                diag_bott_left = False

        if diag_top_rght and x-i >= 0 and y+i < 8:
            if board[y+i][x-i] == '0': moves.append((x-i, y+i))
            else:
                if get_color(board[y+i][x-i]) is not color: moves.append((x-i, y+i))
                diag_top_rght = False

        if diag_top_left and x-i >= 0 and y-i >= 0:
            if board[y-i][x-i] == '0': moves.append((x-i, y-i))
            else:
                if get_color(board[y-i][x-i]) is not color: moves.append((x-i, y-i))
                diag_top_left = False

    return moves

def get_queen_moves(board, color, x, y):
    return get_rook_moves(board, color, x, y) + get_bishop_moves(board, color, x, y)

def get_king_moves(castle_rights, board, territory, color, x, y):
    moves = []

    if x > 0: moves.append((x-1, y)) if get_color(board[y][x-1]) is not color and not territory[y][x-1] else None
    if x < 7: moves.append((x+1, y)) if get_color(board[y][x+1]) is not color and not territory[y][x+1] else None
    if y > 0: moves.append((x, y-1)) if get_color(board[y-1][x]) is not color and not territory[y-1][x] else None
    if y < 7: moves.append((x, y+1)) if get_color(board[y+1][x]) is not color and not territory[y+1][x] else None
    if x > 0 and y > 0: moves.append((x-1, y-1)) if get_color(board[y-1][x-1]) is not color and not territory[y-1][x-1] else None
    if x > 0 and y < 7: moves.append((x-1, y+1)) if get_color(board[y+1][x-1]) is not color and not territory[y+1][x-1] else None
    if x < 7 and y > 0: moves.append((x+1, y-1)) if get_color(board[y-1][x+1]) is not color and not territory[y-1][x+1] else None
    if x < 7 and y < 7: moves.append((x+1, y+1)) if get_color(board[y+1][x+1]) is not color and not territory[y+1][x+1] else None

    # castle
    if color:
        if 'K' in castle_rights:
            if board[7][5] == '0' and board[7][6] == '0':
                if not (territory[7][4] or territory[7][5] or territory[7][6]):
                    moves.append((6, 7))
        if 'Q' in castle_rights:
            if board[7][1] == '0' and board[7][2] == '0' and board[7][3] == '0':
                if not (territory[7][4] or territory[7][3] or territory[7][2] or territory[7][1]):
                    moves.append((2, 7))
    else:
        if 'k' in castle_rights:
            if board[0][5] == '0' and board[0][6] == '0':
                if not (territory[0][4] or territory[0][5] or territory[0][6]):
                    moves.append((6, 0))
        if 'q' in castle_rights:
            if board[0][1] == '0' and board[0][2] == '0' and board[0][3] == '0':
                if not (territory[0][4] or territory[0][3] or territory[0][2] or territory[0][1]):
                    moves.append((2, 0))

    return moves

def make_move(pos, selected, new_selected, board):
    
    if board[selected[1]][selected[0]] == 'P' or board[selected[1]][selected[0]] == 'p':
        pawn_move = True
    else: pawn_move = False
    
    castle_rights = pos.split(' ')[2]
    en_passant = '-'
    if board[new_selected[1]][new_selected[0]] == '0':

        # check en passant
        if selected[1] == 6:
            if new_selected[1] == 4 and board[selected[1]][selected[0]] == 'P':
                if new_selected[0] < 7:
                    if board[new_selected[1]][new_selected[0]+1] == 'p':
                        en_passant = chr(selected[0]+97) + '3'
                if new_selected[0] > 0:
                    if board[new_selected[1]][new_selected[0]-1] == 'p':
                        en_passant = chr(selected[0]+97) + '3'
        elif selected[1] == 1:
            if new_selected[1] == 3 and board[selected[1]][selected[0]] == 'p':
                if new_selected[0] < 7:
                    if board[new_selected[1]][new_selected[0]+1] == 'P':
                        en_passant = chr(selected[0]+97) + '6'
                if new_selected[0] > 0:
                    if board[new_selected[1]][new_selected[0]-1] == 'P':
                        en_passant = chr(selected[0]+97) + '6'

        en_passant_move = pos.split(' ')[3]

        # make en passant move
        if en_passant_move != '-':
            if new_selected == (ord(en_passant_move[0])-97, 8 - int(en_passant_move[1])):
                if board[selected[1]][selected[0]] == 'P':
                    board[new_selected[1]+1][new_selected[0]] = '0'
                elif board[selected[1]][selected[0]] == 'p':
                    board[new_selected[1]-1][new_selected[0]] = '0'

        # castling
        if board[selected[1]][selected[0]] == 'K':
            castle_rights = castle_rights.replace('KQ', '')
            if castle_rights == '': castle_rights = '-'
            if selected == (4, 7) and new_selected == (6, 7):
                board[7][5] = 'R'
                board[7][7] = '0'
            elif selected == (4, 7) and new_selected == (2, 7):
                board[7][3] = 'R'
                board[7][0] = '0'
        elif board[selected[1]][selected[0]] == 'k':
            castle_rights = castle_rights.replace('kq', '')
            if castle_rights == '': castle_rights = '-'
            if selected == (4, 0) and new_selected == (6, 0):
                board[0][5] = 'r'
                board[0][7] = '0'
            elif selected == (4, 0) and new_selected == (2, 0):
                board[0][3] = 'r'
                board[0][0] = '0'
        elif board[selected[1]][selected[0]] == 'R':
            if selected == (0, 7):
                castle_rights = castle_rights.replace('Q', '')
                if castle_rights == '': castle_rights = '-'
            elif selected == (7, 7):
                castle_rights = castle_rights.replace('K', '')
                if castle_rights == '': castle_rights = '-'
        elif board[selected[1]][selected[0]] == 'r':
            if selected == (0, 0):
                castle_rights = castle_rights.replace('q', '')
                if castle_rights == '': castle_rights = '-'
            elif selected == (7, 0):
                castle_rights = castle_rights.replace('k', '')
                if castle_rights == '': castle_rights = '-'
                

        board[new_selected[1]][new_selected[0]] = board[selected[1]][selected[0]]
        board[selected[1]][selected[0]] = '0'
    else:
        # capture
        board[new_selected[1]][new_selected[0]] = board[selected[1]][selected[0]]
        board[selected[1]][selected[0]] = '0'
    
    # improve promotion options
    if new_selected[1] == 0:
        if board[new_selected[1]][new_selected[0]] == 'P':
            board[new_selected[1]][new_selected[0]] = 'Q'
    elif new_selected[1] == 7:
        if board[new_selected[1]][new_selected[0]] == 'p':
            board[new_selected[1]][new_selected[0]] = 'q'
    
    pos = chess.board_to_FEN(board, pos, True, pawn_move, castle_rights, en_passant)
    return (pos, board)

def board_to_neural(board, color):
    neural_board = np.empty((0, 1), dtype=np.int8)
    neural_board = np.append(neural_board, get_king_neuron(board, color))
    neural_board = np.append(neural_board, get_pawn_neuron(board, color))
    neural_board = np.append(neural_board, get_rook_neuron(board, color))
    neural_board = np.append(neural_board, get_knight_neuron(board, color))
    neural_board = np.append(neural_board, get_bishop_neuron(board, color))
    neural_board = np.append(neural_board, get_queen_neuron(board, color))
    
    return np.reshape(neural_board, (1, 768))

def get_pawn_neuron(board, color):
    
    neurons = np.zeros((128), dtype=np.int8)
    if color:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'P': neurons[j*8+i] = 1
                elif board[j][i] == 'p': neurons[64 + j*8+i] = 1

    else:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'p': neurons[j*8+i] = 1
                elif board[j][i] == 'P': neurons[64 + j*8+i] = 1
    
    return neurons

def get_rook_neuron(board, color):
    
    neurons = np.zeros((128), dtype=np.int8)
    if color:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'R': neurons[j*8+i] = 1
                elif board[j][i] == 'r': neurons[64 + j*8+i] = 1

    else:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'r': neurons[j*8+i] = 1
                elif board[j][i] == 'R': neurons[64 + j*8+i] = 1
    
    return neurons

def get_knight_neuron(board, color):
    
    neurons = np.zeros((128), dtype=np.int8)
    if color:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'N': neurons[j*8+i] = 1
                elif board[j][i] == 'n': neurons[64 + j*8+i] = 1

    else:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'n': neurons[j*8+i] = 1
                elif board[j][i] == 'N': neurons[64 + j*8+i] = 1
                
    return neurons

def get_bishop_neuron(board, color):
    
    neurons = np.zeros((128), dtype=np.int8)
    if color:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'B': neurons[j*8+i] = 1
                elif board[j][i] == 'b': neurons[64 + j*8+i] = 1

    else:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'b': neurons[j*8+i] = 1
                elif board[j][i] == 'B': neurons[64 + j*8+i] = 1
                
    return neurons

def get_queen_neuron(board, color):
    
    neurons = np.zeros((128), dtype=np.int8)
    if color:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'Q': neurons[j*8+i] = 1
                elif board[j][i] == 'q': neurons[64 + j*8+i] = 1

    else:
        for i in range(8):
            for j in range(8):
                if board[j][i] == 'q': neurons[j*8+i] = 1
                elif board[j][i] == 'Q': neurons[64 + j*8+i] = 1
                
    return neurons

def get_king_neuron(board, color):
    
    neurons = np.zeros((128), dtype=np.int8)
    squares = get_king_position(board)
    
    neurons[squares[0][1]*8 + squares[0][0]] = 1 if color else 0
    neurons[squares[1][1]*8 + squares[1][0]] = 1 if color else 0
    neurons[squares[0][1]*8 + squares[0][0] + 64] = 1 if not color else 0
    neurons[squares[1][1]*8 + squares[1][0] + 64] = 1 if not color else 0
                
    return neurons