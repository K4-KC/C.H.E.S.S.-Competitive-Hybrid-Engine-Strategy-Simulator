#include <Python.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

static PyObject *ChessError;

static PyObject *
FEN_to_board(PyObject *self, PyObject *args) {
    const char *pos;
    if (!PyArg_ParseTuple(args, "s", &pos)) {
        return NULL;
    }

    PyObject *board = PyList_New(8);
    short currCol = 0;

    PyObject* board_row = PyList_New(8);
    short lastInRow = 0;

    while(true) {
        if(*pos & 0x40) {
            PyList_SetItem(board_row, lastInRow++, PyUnicode_FromStringAndSize(pos, 1));
        } 
        else if (*pos & 0x20 && *pos & 0x10) {
            for (short j = 0; j < *pos - '0'; j++) {
                PyList_SetItem(board_row, lastInRow++, PyUnicode_FromString("0"));
            }
        }
        else {
            PyList_SetItem(board, currCol++, board_row);
            lastInRow = 0;
            board_row = PyList_New(8);
            if(!(*pos & 0xF)) break;
        }

        pos++;
    }

    return board;
}

static PyObject *
board_to_FEN(PyObject *self, PyObject *args) {
    PyObject *board;
    const char *pos;
    int move = 0;
    int pawn_move = 0;
    const char *castle_rights;
    const char *en_passant;
    
    if (!PyArg_ParseTuple(args, "O|siiss", &board, &pos, &move, &pawn_move, &castle_rights, &en_passant)) {
        return NULL;
    }

    std::string fen = "";
    short currRow = 0;

    while(currRow < 8) {
        PyObject *row = PyList_GetItem(board, currRow++);
        short currCol = 0;
        short empty = 0;

        while(currCol < 8) {
            PyObject *cell = PyList_GetItem(row, currCol++);
            const char *cell_str = PyUnicode_AsUTF8(cell);

            if (*cell_str == '0') {
                empty++;
            } 
            else if (empty) {
                fen += std::to_string(empty);
                empty = 0;
                currCol--;
            }
            else {
                fen += *cell_str;
            }
        }

        if (empty) {
            fen += std::to_string(empty);
            empty = 0;
        }

        if (currRow != 8) {
            fen += "/";
        }
    }

    if(move) {
        while(*pos != ' ') pos++;
        pos++;

        char turn = *pos;

        fen += (turn == 'w') ? " b" : " w";
        
        fen += " ";
        fen += castle_rights;

        fen += " ";
        fen += en_passant;

        pos += 2;
        short spaceCount = 0;
        
        while (spaceCount < 2) {
            if (*pos == ' ') {
                spaceCount++;
            }
            pos++;
        }

        std::string first = "";
        std::string second = "";

        while (*pos != ' ') {
            first += *pos;
            pos++;
        }

        pos++;
        while (*pos != '\0') {
            second += *pos;
            pos++;
        }

        fen += " ";
        fen += (pawn_move) ? "0" : std::to_string(std::stoi(first) + 1);

        fen += " ";
        fen += (turn == 'b') ? std::to_string(std::stoi(second) + 1) : second;
    }

    return PyUnicode_FromString(fen.c_str());
}

static PyObject *
get_color(PyObject *self, PyObject *args) {
    const char *piece;
    if (!PyArg_ParseTuple(args, "s", &piece)) {
        return NULL;
    }
    
    if (isupper(piece[0])) {
        Py_INCREF(Py_True);
        return Py_True;
    } else if (islower(piece[0])) {
        Py_INCREF(Py_False);
        return Py_False;
    } else {
        Py_INCREF(Py_None);
        Py_RETURN_NONE;
    }
}

static PyMethodDef ChessMethods[] = {
    {"FEN_to_board", FEN_to_board, METH_VARARGS, 
    "Convert FEN string to board array"},
    {"board_to_FEN", board_to_FEN, METH_VARARGS, 
    "Convert board array to FEN string"},
    {"get_color", get_color, METH_VARARGS,
    "Get color of piece"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef chessmodule = {
    PyModuleDef_HEAD_INIT,
    "chess",
    NULL,
    -1,
    ChessMethods
};

PyMODINIT_FUNC PyInit_chess(void) {
    PyObject *m;

    m = PyModule_Create(&chessmodule);
    if (m == NULL)
        return NULL;

    ChessError = PyErr_NewException("chess.error", NULL, NULL);
    Py_XINCREF(ChessError);
    if (PyModule_AddObject(m, "error", ChessError) < 0) {
        Py_XDECREF(ChessError);
        Py_CLEAR(ChessError);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

int main(int argc, char *argv[]) {
    wchar_t *program = Py_DecodeLocale(argv[0], NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }

    if (PyImport_AppendInittab("chess", PyInit_chess) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table\n");
        exit(1);
    }

    Py_SetProgramName(program);
    Py_Initialize();

    PyObject *pmodule = PyImport_ImportModule("chess");
    
    if (!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import module 'chess'\n");
    }

    PyMem_RawFree(program);
    return 0;
}
