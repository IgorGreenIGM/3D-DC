#coding:utf-8

import os
import pathlib
import threading
import time

# argv[1] = file path + name
# argv[2] = matrix size
# argv[3] = buffer size
# argv[4] = queue size
print("Hello guys")
queue_size = 3000
buffer_size = 50000
file_path = "mb.lyrics"

sizes = []

# creating initial file archive 
os.system(f"rar a init -m5 {file_path} > tmp.txt")
init_size = pathlib.Path("./init.rar").stat().st_size
print("Init file compress Done-------------------------------")

# we start by changing the matrix size
with open("matrix_2D.txt", mode="w") as _2D, open("matrix_3D.txt", mode="w") as _3D, open("matrix_2D_3D.txt", mode="w") as _2D_3D:
    for matrix_size in range(1, 1250):
        print(f"process status : {matrix_size} / 5000")

        print("\n\nProcessing 2D : ----------------------------------------------------------------------------\n")
        os.system(f"output.exe mb.lyrics {matrix_size} {queue_size} 1 0")
        os.system(f"rar a fltred_2D_{matrix_size} -m5 filtered__2D_{file_path} > tmp.txt")
        _2D.write(f"matrix size : {matrix_size} : init size : {init_size} filtred_size = {pathlib.Path(f'fltred_2D_{matrix_size}.rar').stat().st_size}\n")
        print("2D Processing finished------------------------------------------------------------------------")

        print("\n\nProcessing 3D : ----------------------------------------------------------------------\n")
        os.system(f"output.exe mb.lyrics {matrix_size} {queue_size} 0 1")
        os.system(f"rar a fltred_3D_{matrix_size} -m5 filtered__3D_{file_path} > tmp.txt")
        _3D.write(f"matrix size : {matrix_size} : init size : {init_size} filtred_size = {pathlib.Path(f'fltred_3D_{matrix_size}.rar').stat().st_size}\n")
        print("3D Processing finished------------------------------------------------------------------------")

        print("\n\nProcessing 2D_3D : ----------------------------------------------------------------------\n")
        os.system(f"output.exe mb.lyrics {matrix_size} {queue_size} 1 1")
        os.system(f"rar a fltred_2D_3D_{matrix_size} -m5 filtered__2D_3D_{file_path} > tmp.txt")
        _2D_3D.write(f"matrix size : {matrix_size} : init size : {init_size} filtred_size = {pathlib.Path(f'fltred_2D_3D_{matrix_size}.rar').stat().st_size}\n")
        print("2D_3D Processing finished------------------------------------------------------------------------\n")

        os.system("cls")
