import glob
import os
import numpy as np
from functools import reduce

LOG_DIR = "../logs"

if __name__ == '__main__':
    fnames = glob.glob(LOG_DIR + "/" + "*.txt")
    for fname in fnames:
        # Split out params from name
        base = os.path.basename(fname)
        short_name = os.path.splitext(base)[0]
        # im_name, w, h, pcoeff, fcoeff, pmod = short_name.split("_")
        print(fname) 
        f = open(fname, 'r')
        all_data = {}
        for line in f:
            data = line.split(',')[:-1]
            if data[0] in all_data:
                all_data[data[0]] += data[1:]
            else:
                all_data[data[0]] = data[1:]
            # print(all_data[data[0]])
        for key in all_data:
            val = np.array(list(map(lambda x: float(x), all_data[key])))
            print(key + ',' + str(np.mean(val)) + ',' + str(np.std(val)))