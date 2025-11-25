# import tensorflow as tf
# print("TensorFlow version:", tf.__version__)

# mnist = tf.keras.datasets.mnist

# (x_train, y_train), (x_test, y_test) = mnist.load_data()
# x_train, x_test = x_train / 255.0, x_test / 255.0

# import matplotlib.pyplot as plt
# first_array=x_train[1546]
# plt.imshow(first_array)
# plt.show()

# model = tf.keras.models.Sequential([
#   tf.keras.layers.Flatten(input_shape=(28, 28)),
#   tf.keras.layers.Dense(100, activation='relu'),
#   tf.keras.layers.Dropout(0.2),
#   tf.keras.layers.Dense(10)
# ])

# print(x_train[:1].shape)

# predictions = model(x_train[:1]).numpy()

# print(predictions.shape)

# tf.nn.softmax(predictions).numpy()

# loss_fn = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)

# loss_fn(y_train[:1], predictions).numpy()

# loss_fn(y_train[:1], predictions).numpy()

# model.compile(optimizer='adam',
#               loss=loss_fn,
#               metrics=['accuracy'])

# model.fit(x_train, y_train, epochs=5)

# probability_model = tf.keras.Sequential([
#   model,
#   tf.keras.layers.Softmax()
# ])

# probability_model(x_test[:5])

# import chess

# chess.system('echo sdf')
# a = chess.FEN_to_board('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1')
# print(a)


# import os
     
# os.system('echo sdf')

import chess
import chessPY
import time, timeit

# a = chess.FEN_to_board('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1')
# print(a)

# b = chess.board_to_FEN(a)
# print(b)

# print(chess.get_color('P'))

pos = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

setupCode = '''
import chessPY
import chess
pos = 'r1b1k2r/3p1ppp/p1p5/Ppbn1N2/R3N3/1PK5/1BP1PPPP/3q1B1R w KQkq - 0 1' 
board = [['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'], ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'], ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']]
'''
codeSnippetChess = '''b = chess.board_to_FEN(board, pos, True, True, 'kq', 'e6')'''
codeSnippetChessPY = '''b = chessPY.board_to_FEN(board, pos, True, True, 'kq', 'e6')'''

a = timeit.repeat(setup= setupCode,
              stmt= codeSnippetChess,
              repeat=3, number=1000000)
print('cpp: ', sum(a)/len(a))

a = timeit.repeat(setup= setupCode,
              stmt= codeSnippetChessPY,
              repeat=3, number=1000000)
print('py: ', sum(a)/len(a))

# print(chess.FEN_to_board(pos))

# board = [['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'], ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['0', '0', '0', '0', '0', '0', '0', '0'], ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'], ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']]

# print(chess.board_to_FEN(board))

# t1 = time.time()
# for i in range(1000000):
#     a = chessPY.FEN_to_board(pos)

# t2 = time.time()
# print(t2-t1)

# tt1 = time.time()
# for i in range(1):
#     b = chessPY.board_to_FEN(a)
# tt2 = time.time()
# # print(tt2-tt1)

# t3 = time.time()
# for i in range(1000000):
#     a = chess.FEN_to_board(pos)

# t4 = time.time()
# print(t4-t3)

# tt3 = time.time()
# for i in range(1):
#     b = chess.board_to_FEN(a)
# tt4 = time.time()
# # print(tt4-tt3)