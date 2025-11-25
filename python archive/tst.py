import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

import copy

import tensorflow as tf
import numpy as np

TF_ENABLE_ONEDNN_OPTS = 0
print("TF version:", tf.__version__)

import chessPY
import pickle

move_list_path = "data/move_list/move_list.txt"

original_width, original_height = 640, 640
aspect_ratio = original_width / original_height

def FEN_to_board(pos):
    board_part = pos.split(' ')[0]
    rows = board_part.split('/')
    
    board = []
    
    for row in rows:
        board_row = []
        for char in row:
            if char.isdigit():
                board_row.extend(['0'] * int(char))
            else:
                board_row.append(char)
        board.append(board_row)
    
    return board

def board_to_FEN(board):
    fen_rows = []
    
    for row in board:
        fen_row = ""
        empty_count = 0
        
        for cell in row:
            if cell == '0':
                empty_count += 1
            else:
                if empty_count > 0:
                    fen_row += str(empty_count)
                    empty_count = 0
                fen_row += cell

        if empty_count > 0:
            fen_row += str(empty_count)
        
        fen_rows.append(fen_row)
    
    fen_position = "/".join(fen_rows)
    
    return fen_position + " w KQkq - 0 1"

init_pos = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

board_flip = False
board = FEN_to_board(init_pos)
# board = [['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],
#          ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],
#          ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']]

moves = chessPY.get_moves(init_pos, board)
pos = init_pos
print(pos)

move_list, draw_move_list = [], [pos]
selected = False

window_size = (original_width, original_height)
running = True

model = [tf.keras.models.Sequential([
  tf.keras.layers.Dense(768),
  tf.keras.layers.Dense(1, activation='sigmoid')
]) for i in range(2)]

while running:
    
    color = True if pos.split(' ')[1] == 'w' else False
    
    # get moves
    moves = chessPY.get_moves(pos, board)
    if moves == [[[] for i in range(8)] for j in range(8)]:
        move_list.append((pos, [[[] for i in range(8)] for j in range(8)]))
        # print(move_list)
        with open(move_list_path, 'wb') as f:
            pickle.dump(move_list, f)
        print('White won' if pos.split(' ')[1] == 'b' else 'Black won')
        running = False
        break
    if pos.split(' ')[4] == '50':
        move_list.append((pos, [[[] for i in range(8)] for j in range(8)]))
        # print(move_list)
        with open(move_list_path, 'wb') as f:
            pickle.dump(move_list, f)
        print('Draw by 50 move rule')
        running = False
        break
    if [i.split(' ')[0] for i in draw_move_list].count(pos.split(' ')[0]) == 3:
        move_list.append((pos, [[[] for i in range(8)] for j in range(8)]))
        # print(move_list)
        with open(move_list_path, 'wb') as f:
            pickle.dump(move_list, f)
        print('Draw by repetition')
        running = False
        break
    
    # predict
    predictions = [[np.zeros([0,]) for i in range(8)] for j in range(8)]
    max_prediction, max_prediction_square, max_prediction_square_move = 0, (0, 0), (0, 0)
    for row in range(8):
        for move_set in range(8):
            if moves[row][move_set] != []:

                piece_moves = [chessPY.make_move(
                    pos, (move_set, row), move, copy.deepcopy(board)) for move in moves[row][move_set]]
                
                move_input = np.array([chessPY.board_to_neural(
                    future_board, True if future_pos.split(' ')[1] == 'b' else False) 
                                       for future_pos, future_board in piece_moves])
                prediction = model[0 if color else 1].predict(move_input)

                if max(prediction) >= max_prediction:
                    max_prediction = max(prediction)
                    max_prediction_square = (move_set, row)
                    max_prediction_square_move = moves[row][move_set][np.argmax(prediction)]
                predictions[row][move_set] = np.array([i[0][0] for i in prediction])
            predictions[row][move_set] = predictions[row][move_set].tolist()
    
    # make the best move
    if board[max_prediction_square_move[1]][
        max_prediction_square_move[0]] == 'P' or board[max_prediction_square_move[1]][max_prediction_square_move[0]] == 'p':
        draw_move_list = []
    pre_pos = pos
    pos, board = chessPY.make_move(pos, max_prediction_square, max_prediction_square_move, board)
    print(pos, max_prediction_square, max_prediction_square_move)
    move_list.append((pre_pos, predictions))
    draw_move_list.append(pos)
    
    # if(1): running = False