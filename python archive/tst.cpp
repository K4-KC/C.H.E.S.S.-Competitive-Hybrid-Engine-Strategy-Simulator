#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include <iostream>

static PyObject *ChessError;

static PyObject *
chess_system(PyObject *self, PyObject *args)
{
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;

    sts = system(command);
    
    if (sts < 0) {
        PyErr_SetString(ChessError, "System command failed");
        return NULL;
    }

    return PyLong_FromLong(sts);
}

static PyMethodDef ChessMethods[] = {
    {"system",  chess_system, METH_VARARGS,
     "Execute a shell command."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef chessmodule = {
    PyModuleDef_HEAD_INIT,
    "chess",
    NULL,
    -1,
    ChessMethods
};

PyMODINIT_FUNC
PyInit_chess(void)
{
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

int
main(int argc, char *argv[])
{
    wchar_t *program = Py_DecodeLocale(argv[0], NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }

    /* Add a built-in module, before Py_Initialize */
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