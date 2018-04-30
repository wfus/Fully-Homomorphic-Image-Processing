import glob
import os
import numpy as np
from functools import reduce
import matplotlib.pyplot as plt
import math

RESULTS_FILE = "results.txt"

f = open(RESULTS_FILE, 'r')
count = 0
lines = f.readlines()
data = {'jpg' : {}, 'bicubic' : {}, 'bilinear' : {}}
for i in range(len(lines)):
    if lines[i][0] == '.':
        name_split = lines[i].split('.')[2].split('/')[-1].split('_')
        run = name_split[0]
        if run == 'resize':
            run = name_split[2]
        poly_mod = name_split[-2]
        plain_mod = name_split[-1]
        file_data = {}
        times = 5 if run == 'jpg' else 4
        for _ in range(times):
            i += 1
            data_point = lines[i].strip().split(',')
            file_data[data_point[0]] = tuple(list(data_point[1:]))
        data[run][(poly_mod, plain_mod)] = file_data

NUM_PLAIN_MOD = 9

processed_data = {}
DATA_LABELS = ['Encryption', 'Decryption', 'Linear', 'Cubic', 'DCT', 'RGBYCC']

for key in DATA_LABELS:
    processed_data[key] = {}

for (poly_mod, plain_mod) in data['bilinear']:
    for key in DATA_LABELS:
        processed_data[key][poly_mod] = 0


for data_set in ['bilinear', 'bicubic', 'jpg']:
    for (poly_mod, plain_mod) in data[data_set]:
        for key in DATA_LABELS:
            if key in data[data_set][('2048', '11')]:
                processed_data[key][poly_mod] += float(data[data_set][(poly_mod, plain_mod)][key][0]) / NUM_PLAIN_MOD
    for poly_mod in processed_data['Encryption']:
        processed_data['Encryption'][poly_mod] /= 3
        processed_data['Decryption'][poly_mod] /= 3

print(processed_data)

