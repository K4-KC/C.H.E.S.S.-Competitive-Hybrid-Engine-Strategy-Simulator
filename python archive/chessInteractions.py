import chess
import pygame
import chessPY
from pathlib import Path
import pickle
import math

# system("echo 5.0")

import sys

pygame.init()

original_pieces = {
    'P' :[pygame.image.load('data/png/pieces/White/P' + str(P) +'.png') for P in range(2)],
    'K' :[pygame.image.load('data/png/pieces/White/K' + str(K) +'.png') for K in range(2)],
    'Q' :[pygame.image.load('data/png/pieces/White/Q' + str(Q) +'.png') for Q in range(2)],
    'R' :[pygame.image.load('data/png/pieces/White/R' + str(R) +'.png') for R in range(2)],
    'B' :[pygame.image.load('data/png/pieces/White/B' + str(B) +'.png') for B in range(2)],
    'N' :[pygame.image.load('data/png/pieces/White/N' + str(N) +'.png') for N in range(2)],

    'p' :[pygame.image.load('data/png/pieces/Black/p' + str(p) +'.png') for p in range(2)],
    'k' :[pygame.image.load('data/png/pieces/Black/k' + str(k) +'.png') for k in range(2)],
    'q' :[pygame.image.load('data/png/pieces/Black/q' + str(q) +'.png') for q in range(2)],
    'r' :[pygame.image.load('data/png/pieces/Black/r' + str(r) +'.png') for r in range(2)],
    'b' :[pygame.image.load('data/png/pieces/Black/b' + str(b) +'.png') for b in range(2)],
    'n' :[pygame.image.load('data/png/pieces/Black/n' + str(n) +'.png') for n in range(2)],

    '0' :[pygame.image.load('data/png/pieces/0' + str(i) +'.png') for i in range(2)]
    }

original_width, original_height = 640, 640
aspect_ratio = original_width / original_height

def transform_scale(original_pieces, width, height):
    return {
        'P' :[pygame.transform.scale(original_pieces['P'][i], (width//8, height//8)) for i in range(2)],
        'K' :[pygame.transform.scale(original_pieces['K'][i], (width//8, height//8)) for i in range(2)],
        'Q' :[pygame.transform.scale(original_pieces['Q'][i], (width//8, height//8)) for i in range(2)],
        'R' :[pygame.transform.scale(original_pieces['R'][i], (width//8, height//8)) for i in range(2)],
        'B' :[pygame.transform.scale(original_pieces['B'][i], (width//8, height//8)) for i in range(2)],
        'N' :[pygame.transform.scale(original_pieces['N'][i], (width//8, height//8)) for i in range(2)],

        'p' :[pygame.transform.scale(original_pieces['p'][i], (width//8, height//8)) for i in range(2)],
        'k' :[pygame.transform.scale(original_pieces['k'][i], (width//8, height//8)) for i in range(2)],
        'q' :[pygame.transform.scale(original_pieces['q'][i], (width//8, height//8)) for i in range(2)],
        'r' :[pygame.transform.scale(original_pieces['r'][i], (width//8, height//8)) for i in range(2)],
        'b' :[pygame.transform.scale(original_pieces['b'][i], (width//8, height//8)) for i in range(2)],
        'n' :[pygame.transform.scale(original_pieces['n'][i], (width//8, height//8)) for i in range(2)],

        '0' :[pygame.transform.scale(original_pieces['0'][i], (width//8, height//8)) for i in range(2)]
        }

pieces = transform_scale(original_pieces, original_width, original_height)

screen = pygame.display.set_mode((original_width, original_height), pygame.RESIZABLE)
capture_screen = pygame.Surface((original_width/8, original_height/8), pygame.SRCALPHA)
move_screen = pygame.Surface((original_width/8, original_height/8), pygame.SRCALPHA)
off_track_screen = pygame.Surface((original_width, original_height), pygame.SRCALPHA)
pygame.draw.circle(capture_screen,(100, 100, 100, 100), 
                   ((original_width/16), (original_height/16)), original_height//16, original_height//90)
pygame.draw.circle(move_screen,(100, 100, 100, 100), 
                   ((original_width/16), (original_height/16)), original_height//50)

off_track_screen.fill((255, 255, 255, 100))

pygame.display.set_caption("ChessAI")

def maintain_aspect_ratio(event_size):
    new_width, new_height = event_size
    new_width = 8 * (new_width//8)
    new_height = 8 * (new_height//8)
    if new_width / new_height > aspect_ratio:
        new_width = 8 * (int(new_height * aspect_ratio)//8)
    else:
        new_height = 8 * (int(new_width / aspect_ratio)//8)
    return new_width, new_height

def flip_board(board):
    return [row for row in board]

def flip_moves(moves):
    moves = flip_board(moves)
    return [[[(7 - move[0], 7 - move[1]) for move in piece] for piece in row] for row in moves]

def merge_colors(color1, color2, alpha):
    return(color2[0] - (color2[0] - color1[0]) * alpha,
           color2[1] - (color2[1] - color1[1]) * alpha,
           color2[2] - (color2[2] - color1[2]) * alpha)
    # return (color1[0] * alpha + color2[0] * (1 - alpha), 
    #         color1[1] * alpha + color2[1] * (1 - alpha), 
    #         color1[2] * alpha + color2[2] * (1 - alpha))

pre_move_list = Path("data/move_list/move_list.txt")
    # 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', 'rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq - 0 1', 'rnbqkb1r/pppppppp/5n2/8/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 1 2', 'rnbqkb1r/pppppppp/5n2/8/8/4P3/PPPPQPPP/RNB1KBNR b KQkq - 2 2', 'rnbqkb1r/pp1ppppp/5n2/2p5/8/4P3/PPPPQPPP/RNB1KBNR w KQkq - 0 3', 'rnbqkb1r/pp1ppppp/5n2/2p5/7P/4P3/PPPPQPP1/RNB1KBNR b KQkq - 0 3', 'rnb1kb1r/ppqppppp/5n2/2p5/7P/4P3/PPPPQPP1/RNB1KBNR w KQkq - 1 4', 'rnb1kb1r/ppqppppp/5n2/2p5/3P3P/4P3/PPP1QPP1/RNB1KBNR b KQkq - 0 4', 'rnb1kb1r/ppq1pppp/5n2/2pp4/3P3P/4P3/PPP1QPP1/RNB1KBNR w KQkq - 0 5', 'rnb1kb1r/ppq1pppp/5n2/2pp4/3P3P/4P3/PPPKQPP1/RNB2BNR b kq - 1 5', 'rn2kb1r/ppq1pppp/5n2/2pp4/3P2bP/4P3/PPPKQPP1/RNB2BNR w kq - 2 6', 'rn2kb1r/ppq1pppp/5n2/2pp4/3P2bP/4P1P1/PPPKQP2/RNB2BNR b kq - 0 6', 'rn2kb1r/ppq1pppp/5n2/2pp4/3P3P/4P1P1/PPPKbP2/RNB2BNR w kq - 1 7', 'rn2kb1r/ppq1pppp/5n2/2pp4/3P3P/2N1P1P1/PPPKbP2/R1B2BNR b kq - 2 7', 'rn2kb1r/ppq1pppp/5n2/3p4/2pP3P/2N1P1P1/PPPKbP2/R1B2BNR w kq - 0 8', 'rn2kb1r/ppq1pppp/5n2/3p4/N1pP3P/4P1P1/PPPKbP2/R1B2BNR b kq - 1 8', 'rn2kb1r/1pq1pppp/5n2/p2p4/N1pP3P/4P1P1/PPPKbP2/R1B2BNR w kq - 0 9', 'rn2kb1r/1pq1pppp/5n2/p2p4/N1pP3P/4P1P1/PPPKbP2/1RB2BNR b kq - 1 9', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1pP3P/4P1P1/PPPKbP2/1RB2BNR w k - 2 10', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1pP3P/1P2P1P1/P1PKbP2/1RB2BNR b k - 0 10', '1n3b1r/1pqkpppp/r4n2/p2p4/N1pP3P/1P2P1P1/P1PKbP2/1RB2BNR w k - 1 11', '1n3b1r/1pqkpppp/r4n2/p2p4/N1pP3P/1PP1P1P1/P2KbP2/1RB2BNR b k - 0 11', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1pP3P/1PP1P1P1/P2KbP2/1RB2BNR w k - 1 12', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/2P1P1P1/P2KbP2/1RB2BNR b k - 0 12', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/2P1P1P1/P2KbP2/1RB2BNR w k - 1 13', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/1RP1P1P1/P2KbP2/2B2BNR b k - 2 13', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/1RP1P1P1/P2KbP2/2B2BNR w k - 3 14', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/1RP1P1P1/PB1KbP2/5BNR b k - 4 14', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/1RP1P1P1/PB1KbP2/5BNR w k - 5 15', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/1RP1P1PR/PB1KbP2/5BN1 b k - 6 15', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/1RP1P1PR/PB1KbP2/5BN1 w k - 7 16', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/PRP1P1PR/1B1KbP2/5BN1 b k - 0 16', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/PRP1P1PR/1B1KbP2/5BN1 w k - 1 17', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/PRP1P1PR/1B1KBP2/6N1 b k - 2 17', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/PRP1P1PR/1B1KBP2/6N1 w k - 3 18', '1n2kb1r/1pq1pppp/r4n2/p2p4/N1PP3P/PRP1PBPR/1B1K1P2/6N1 b k - 4 18', '1n3b1r/1pqkpppp/r4n2/p2p4/N1PP3P/PRP1PBPR/1B1K1P2/6N1 w k - 5 19', '1n3b1r/1pqkpppp/r4n2/p2p3P/N1PP4/PRP1PBPR/1B1K1P2/6N1 b k - 0 19', '1n2kb1r/1pq1pppp/r4n2/p2p3P/N1PP4/PRP1PBPR/1B1K1P2/6N1 w k - 1 20', '1n2kb1r/1Rq1pppp/r4n2/p2p3P/N1PP4/P1P1PBPR/1B1K1P2/6N1 b k - 2 20', '1n3b1r/1Rqkpppp/r4n2/p2p3P/N1PP4/P1P1PBPR/1B1K1P2/6N1 w k - 3 21', '1n3b1r/2qkpppp/r4n2/p2p3P/N1PP4/PRP1PBPR/1B1K1P2/6N1 b k - 4 21', '1n2kb1r/2q1pppp/r4n2/p2p3P/N1PP4/PRP1PBPR/1B1K1P2/6N1 w k - 5 22', '1n2kb1r/2q1pppp/r4n2/p1Pp3P/N2P4/PRP1PBPR/1B1K1P2/6N1 b k - 0 22', '1n3b1r/2qkpppp/r4n2/p1Pp3P/N2P4/PRP1PBPR/1B1K1P2/6N1 w k - 1 23', '1n3b1r/2qkpppp/r1P2n2/p2p3P/N2P4/PRP1PBPR/1B1K1P2/6N1 b k - 0 23', '1n2kb1r/2q1pppp/r1P2n2/p2p3P/N2P4/PRP1PBPR/1B1K1P2/6N1 w k - 1 24', '1n2kb1r/1Rq1pppp/r1P2n2/p2p3P/N2P4/P1P1PBPR/1B1K1P2/6N1 b k - 2 24', '1n2kb1r/1Rq1pp1p/r1P2n2/p2p2pP/N2P4/P1P1PBPR/1B1K1P2/6N1 w k g6 0 25', '1n2kb1r/1Rq1pp1p/r1P2nP1/p2p4/N2P4/P1P1PBPR/1B1K1P2/6N1 b k - 0 25', '1n2kb1r/1Rq1p2p/r1P2np1/p2p4/N2P4/P1P1PBPR/1B1K1P2/6N1 w k - 0 26', '1n2kb1r/2q1p2p/r1P2np1/p2p4/N2P4/PRP1PBPR/1B1K1P2/6N1 b k - 1 26', '1n2kb1r/2q1p2p/r1P2n2/p2p2p1/N2P4/PRP1PBPR/1B1K1P2/6N1 w k - 0 27', '1n2kb1r/2q1p2R/r1P2n2/p2p2p1/N2P4/PRP1PBP1/1B1K1P2/6N1 b k - 1 27', '1n2kb1r/2q4R/r1P2n2/p2pp1p1/N2P4/PRP1PBP1/1B1K1P2/6N1 w k - 0 28', '1n2kb1r/2q5/r1P2n2/p2pp1p1/N2P4/PRP1PBPR/1B1K1P2/6N1 b k - 1 28', '1n2k2r/2q5/r1P2n2/p2pp1p1/N2P4/bRP1PBPR/1B1K1P2/6N1 w k - 2 29', '1n2k2r/2q5/r1P2n2/p2pP1p1/N7/bRP1PBPR/1B1K1P2/6N1 b k - 0 29', '1n2k2r/2q5/r1P2n2/p2pP1p1/N7/1RP1PBPR/1b1K1P2/6N1 w k - 1 30', '1n2k2r/1Rq5/r1P2n2/p2pP1p1/N7/2P1PBPR/1b1K1P2/6N1 b k - 2 30', '1n2k2r/1Rq5/r1P2n2/p2pP1p1/N7/b1P1PBPR/3K1P2/6N1 w k - 3 31', '1n2k2r/2q5/r1P2n2/p2pP1p1/N7/bRP1PBPR/3K1P2/6N1 b k - 4 31', '1n2k2r/q7/r1P2n2/p2pP1p1/N7/bRP1PBPR/3K1P2/6N1 w k - 5 32', '1n2k2r/qR6/r1P2n2/p2pP1p1/N7/b1P1PBPR/3K1P2/6N1 b k - 6 32', '1n2k2r/1R6/r1P2n2/p1qpP1p1/N7/b1P1PBPR/3K1P2/6N1 w k - 7 33', '1n2k2r/8/r1P2n2/p1qpP1p1/N7/bRP1PBPR/3K1P2/6N1 b k - 8 33', '1n2kq1r/8/r1P2n2/p2pP1p1/N7/bRP1PBPR/3K1P2/6N1 w k - 9 34', '1n2kq1r/1R6/r1P2n2/p2pP1p1/N7/b1P1PBPR/3K1P2/6N1 b k - 10 34', '1n2k2r/1R6/r1P2n2/p1qpP1p1/N7/b1P1PBPR/3K1P2/6N1 w k - 11 35', '1n2k2r/8/r1P2n2/p1qpP1p1/N7/bRP1PBPR/3K1P2/6N1 b k - 12 35', '1n2kq1r/8/r1P2n2/p2pP1p1/N7/bRP1PBPR/3K1P2/6N1 w k - 13 36', '1n2kq1r/1R6/r1P2n2/p2pP1p1/N7/b1P1PBPR/3K1P2/6N1 b k - 14 36', '1n2k2r/1R6/r1P2n2/p1qpP1p1/N7/b1P1PBPR/3K1P2/6N1 w k - 15 37']
    # 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', 'rnbqkbnr/pppppppp/8/8/8/1P6/P1PPPPPP/RNBQKBNR b KQkq - 0 1', 'rnbqkbnr/pppp1ppp/8/4p3/8/1P6/P1PPPPPP/RNBQKBNR w KQkq - 0 2', 'rnbqkbnr/pppp1ppp/8/4p3/8/1P6/PBPPPPPP/RN1QKBNR b KQkq - 1b KQkq - 1 2', 'rnbqkb1r/ppppnppp/8/4p3/8/1P6/PBPPPPPP/RN1QKBNR w KQkq - 2 3', 'rnbqkb1r/ppppnppp/8/4p3/8/1P3N2/PBPPPPPP/RN1QKB1R b KQkq - 3 3', 'r1bqkb1r/ppppnppp/2n5/4p3/8/1P3N2/PBPPPPPP/RN1QKB1R w KQkq - 4 4', 'r1bqkb1r/ppppnppp/2n5/4p3/3P4/1P3N2/PBP1PPPP/RN1QKB1R b KQkq - 0 4', 'r1bqkb1r/1pppnppp/p1n5/4p3/3P4/1P3N2/PBP1PPPP/RN1QKB1R w KQkq - 0 5', 'r1bqkb1r/1pppnppp/p1n5/4P3/8/1P3N2/PBP1PPPP/RN1QKB1R b KQkq - 0 5', 'r1bqkb1r/1pppnppp/p7/4P3/1n6/1P3N2/PBP1PPPP/RN1QKB1R w KQkq - 1 6', 'r1bqkb1r/1pppnppp/p7/4P3/1n6/1P3N2/PBPKPPPP/RN1Q1B1R b kq - 2 6', 'r1bqkb1r/1p1pnppp/p1p5/4P3/1n6/1P3N2/PBPKPPPP/RN1Q1B1R w kq - 0 7', 'r1bqkb1r/1p1pnppp/p1p5/4P3/1n5N/1P6/PBPKPPPP/RN1Q1B1R b kq - 1 7', 'r1bqkb1r/1p1p1ppp/p1p5/4Pn2/1n5N/1P6/PBPKPPPP/RN1Q1B1R w kq - 2 8', 'r1bqkb1r/1p1p1ppp/p1p5/4PN2/1n6/1P6/PBPKPPPP/RN1Q1B1R b kq - 3 8', 'r1bqk2r/1p1p1ppp/p1p5/2b1PN2/1n6/1P6/PBPKPPPP/RN1Q1B1R w kq - 4 9', 'r1bqk2r/1p1p1ppp/p1p5/2b1PN2/Pn6/1P6/1BPKPPPP/RN1Q1B1R b kq - 0 9', 'r1b1k2r/1p1p1ppp/p1p5/2b1PN2/Pn5q/1P6/1BPKPPPP/RN1Q1B1R w kq - 1 10', 'r1b1k2r/1p1p1ppp/p1p5/2b1PN2/Pn5q/1P6/RBPKPPPP/1N1Q1B1R b kq - 2 10', 'r1b1k2r/1p1p1ppp/p1p5/2b1PN2/Pn2q3/1P6/RBPKPPPP/1N1Q1B1R w kq - 3 11', 'r1b1k2r/1p1p1ppp/p1p5/P1b1PN2/1n2q3/1P6/RBPKPPPP/1N1Q1B1R b kq - 0 11', 'r1b1k2r/3p1ppp/p1p5/Ppb1PN2/1n2q3/1P6/RBPKPPPP/1N1Q1B1R w kq b6 0 12', 'r1b1k2r/3p1ppp/p1p5/Ppb1PN2/1n2q3/1PN5/RBPKPPPP/3Q1B1R b kq - 1 12', 'r1b1k2r/3p1ppp/p1p5/Ppb1qN2/1n6/1PN5/RBPKPPPP/3Q1B1R w kq - 2 13', 'r1b1k2r/3p1ppp/p1p5/Ppb1qN2/1n2N3/1P6/RBPKPPPP/3Q1B1R b kq - 3 13', 'r1b1k2r/3p1ppp/p1pq4/Ppb2N2/1n2N3/1P6/RBPKPPPP/3Q1B1R w kq - 4 14', 'r1b1k2r/3p1ppp/p1pq4/Ppb2N2/1n2N3/1PK5/RBP1PPPP/3Q1B1R b kq - 5 14', 'r1b1k2r/3p1ppp/p1p5/Ppb2N2/1n2N3/1PK5/RBP1PPPP/3q1B1R w kq - 6 15', 'r1b1k2r/3p1ppp/p1p5/Ppb2N2/Rn2N3/1PK5/1BP1PPPP/3q1B1R b kq - 7 15', 'r1b1k2r/3p1ppp/p1p5/Ppbn1N2/R3N3/1PK5/1BP1PPPP/3q1B1R w kq - 8 16']
if pre_move_list:
    with pre_move_list.open('rb') as f:
        pre_move_list = pickle.load(f)
    init_pos = pre_move_list[0][0]
else:
    init_pos = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

# init_pos = '1n2kbn1/rp2ppp1/1q5r/p6p/2p2PP1/1P2P2P/PR2K1B1/1b2q1NR w KQkq - 0 1'
# init_pos = 'R7/8/8/4R3/8/8/8/8 w KQkq - 0 1'

board_flip = False
board = chess.FEN_to_board(init_pos)
# board = [['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],
#          ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['0', '0', '0', '0', '0', '0', '0', '0'],
#          ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],
#          ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']]

moves = chessPY.get_moves(init_pos, board)

if board_flip:
    flipped_board = flip_board(board)
    flipped_moves = flip_moves(moves)

pos = init_pos
print(pos)

move_list, draw_move_list = [pos], [pos]
selected = False

window_size = (original_width, original_height)
running, off_track = True, False
while running:

    for event in pygame.event.get():
        
        if event.type == pygame.QUIT:
            running = False
        
        if event.type == pygame.MOUSEBUTTONDOWN:
            if pygame.mouse.get_pressed()[0]:
                x, y = pygame.mouse.get_pos()
                new_selected = (int(x/(window_size[0]/8)), int(y/(window_size[1]/8)))
                if board_flip and pos.split(' ')[1] == 'b': new_selected = (7 - new_selected[0], 7 - new_selected[1])

                if selected:
                    if new_selected in moves[selected[1]][selected[0]]:
                        if board[selected[1]][selected[0]] == 'P' or board[selected[1]][selected[0]] == 'p':
                            draw_move_list = []
                        pos, board = chessPY.make_move(pos, selected, new_selected, board)
                        if pre_move_list:
                            if len(move_list) < len(pre_move_list):
                                if pos != pre_move_list[len(move_list)][0]: off_track = True
                            else: off_track = True
                        print(pos)
                        
                        move_list.append(pos)
                        draw_move_list.append(pos)
                        moves = chessPY.get_moves(pos, board)
                        if board_flip and pos.split(' ')[1] == 'b':
                            flipped_board = flip_board(board)
                            flipped_moves = flip_moves(moves)
                        selected = False
                        
                        if moves == [[[] for i in range(8)] for j in range(8)]:
                            print('White won' if pos.split(' ')[1] == 'b' else 'Black won')
                            if not pre_move_list:
                                running = False
                                break
                        if pos.split(' ')[4] == '50':
                            print('Draw by 50 move rule')
                            if not pre_move_list:
                                running = False
                                break
                        if [i.split(' ')[0] for i in draw_move_list].count(pos.split(' ')[0]) >= 3:
                            print('Draw by repetition')
                            if not pre_move_list:
                                running = False
                                break

                    elif board[new_selected[1]][new_selected[0]] == '0': selected = False
                    else: selected = new_selected
                else:
                    if board[new_selected[1]][new_selected[0]] == '0': selected = False
                    else: selected = new_selected
        
        if event.type == pygame.KEYDOWN:
            
            if event.key == pygame.K_LEFT:
                if len(move_list) > 1:
                    move_list.pop()
                    if draw_move_list != []: draw_move_list.pop()
                    pos = move_list[-1]
                    board = chess.FEN_to_board(pos)
                    moves = chessPY.get_moves(pos, board)
                    if board_flip and pos.split(' ')[1] == 'b':
                        flipped_board = flip_board(board)
                        flipped_moves = flip_moves(moves)
                    print(pos)
                    selected = False
                    if pre_move_list and len(move_list) <= len(pre_move_list):
                        if pos == pre_move_list[len(move_list)-1][0]: off_track = False
                    
            elif event.key == pygame.K_RIGHT:
                if not off_track and len(move_list) < len(pre_move_list):
                    pos = pre_move_list[len(move_list)][0]
                    board = chess.FEN_to_board(pos)
                    moves = chessPY.get_moves(pos, board)
                    if board_flip and pos.split(' ')[1] == 'b':
                        flipped_board = flip_board(board)
                        flipped_moves = flip_moves(moves)
                    print(pos)
                    move_list.append(pos)
                    draw_move_list.append(pos)
                    selected = False
                    
                    # if moves == [[[] for i in range(8)] for j in range(8)]:
                    #     print('White won' if pos.split(' ')[1] == 'b' else 'Black won')
                    #     running = False
                    #     break
                    # if pos.split(' ')[4] == '50':
                    #     print('Draw by 50 move rule')
                    #     running = False
                    #     break
                    # if [i.split(' ')[0] for i in draw_move_list].count(pos.split(' ')[0]) >= 3:
                    #     print('Draw by repetition')
                    #     running = False
                    #     break

        elif event.type == pygame.VIDEORESIZE:
            window_size = maintain_aspect_ratio(event.size)
            print(window_size)

            pieces = transform_scale(original_pieces, window_size[0], window_size[1])
            
            capture_screen = pygame.Surface((window_size[0]/8, window_size[1]/8), pygame.SRCALPHA)
            move_screen = pygame.Surface((window_size[0]/8, window_size[1]/8), pygame.SRCALPHA)
            off_track_screen = pygame.Surface((window_size[0], window_size[1]), pygame.SRCALPHA)
            off_track_screen.fill((255, 255, 255, 100))
            pygame.draw.circle(capture_screen,(100, 100, 100, 100), 
                            ((window_size[0]/16), (window_size[1]/16)), 
                            window_size[1]//16, window_size[1]//90)
            pygame.draw.circle(move_screen,(100, 100, 100, 100), 
                            ((window_size[0]/16), (window_size[1]/16)), 
                            window_size[1]//50)
            
            screen = pygame.display.set_mode(window_size, pygame.RESIZABLE)

    if board_flip and pos.split(' ')[1] == 'b':
        for i in range(0, 8):
            for j in range(0, 8):
                screen.blit(pieces[flipped_board[j][i]][(i+j)%2], 
                            (i*window_size[0]/8, j*window_size[1]/8))
    else:
        for i in range(0, 8):
            for j in range(0, 8):
                screen.blit(pieces[board[j][i]][(i+j)%2], 
                            (i*window_size[0]/8, j*window_size[1]/8))
    
    if selected:
        if board_flip and pos.split(' ')[1] == 'b':
            flipped_selected = (7 - selected[0], 7 - selected[1])
            pygame.draw.rect(screen, (245, 246, 130), pygame.Rect(
                flipped_selected[0] * window_size[0]/8, flipped_selected[1] * window_size[1]/8, 
                                            window_size[0]/8, window_size[1]/8), window_size[0]//128)
            for move in moves[selected[1]][selected[0]]:
                if board[move[1]][move[0]] == '0':
                    screen.blit(move_screen, ((7-move[0]) * window_size[0]/8, (7-move[1]) * window_size[1]/8))
                else:
                    screen.blit(capture_screen, ((7-move[0]) * window_size[0]/8, (7-move[1]) * window_size[1]/8))
        else:
            pygame.draw.rect(screen, (245, 246, 130), pygame.Rect(
                selected[0] * window_size[0]/8, selected[1] * window_size[1]/8, 
                                            window_size[0]/8, window_size[1]/8), window_size[0]//128)
            # pygame.draw.rect(screen, (245, 246, 130) if (selected[0] + selected[1]) % 2 == 0 else (185, 202, 67),
            #                     pygame.Rect(selected[0] * window_size[0]/8, selected[1] * window_size[1]/8, 
            #                                 window_size[0]/8, window_size[1]/8), 
            #                     window_size[0]//128)
            for move in moves[selected[1]][selected[0]]:
                if board[move[1]][move[0]] == '0':
                    screen.blit(move_screen, (move[0] * window_size[0]/8, move[1] * window_size[1]/8))
                else:
                    screen.blit(capture_screen, (move[0] * window_size[0]/8, move[1] * window_size[1]/8))
        for i in range(len(pre_move_list[len(move_list)-1][1][selected[1]][selected[0]])):
            value = pre_move_list[len(move_list)-1][1][selected[1]][selected[0]][i]
            value = (math.sin((value-1)*math.pi/2)) + 1
            width = int((window_size[0]*value)/77)
            for p in range(width):
                pygame.draw.rect(screen, 
                    merge_colors((235, 236, 208), (merge_colors((15, 20, 240), 
                                merge_colors((15, 20, 240), (255, 255, 255), value), 
                                    (1-p/width)*value)), p/width) if (moves[selected[1]][selected[0]][i][0]+moves[selected[1]][selected[0]][i][1])%2 == 0 else 
                    merge_colors((118, 150, 86), (merge_colors((15, 20, 240), 
                                merge_colors((15, 20, 240), (255, 255, 255), value), 
                                    (1-p/width)*value)), p/width), pygame.Rect(
                                        moves[selected[1]][selected[0]][i][0] * window_size[0]/8 + p, moves[selected[1]][selected[0]][i][1] * window_size[1]/8 + p, 
                                        window_size[0]/8 - 2*p, window_size[1]/8 - 2*p), 1)
    
    if off_track: screen.blit(off_track_screen, (0, 0))
    elif pre_move_list and not selected:
        for i in range(8):
            for j in range(8):
                if pre_move_list[len(move_list)-1][1][j][i] == []: continue
                value = max(pre_move_list[len(move_list)-1][1][j][i])
                value = (math.sin((value-1)*math.pi/2)) + 1
                width = int((window_size[0]*value)/77)
                for p in range(width):
                    pygame.draw.rect(screen, 
                        merge_colors((235, 236, 208), (merge_colors((15, 20, 240), 
                                    merge_colors((15, 20, 240), (255, 255, 255), value), 
                                     (1-p/width)*value)), p/width) if (i+j)%2 == 0 else 
                        merge_colors((118, 150, 86), (merge_colors((15, 20, 240), 
                                    merge_colors((15, 20, 240), (255, 255, 255), value), 
                                     (1-p/width)*value)), p/width), pygame.Rect(i * window_size[0]/8 + p, j * window_size[1]/8 + p, 
                                            window_size[0]/8 - 2*p, window_size[1]/8 - 2*p), 1)
                
    
    pygame.display.flip()

pygame.quit()
sys.exit()