import glob
import os
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
        for line in f:
            data = line.split(',')[:-1]
            val = list(map(lambda x: float(x), data[1:]))
            avg = reduce(lambda x, y: x + y, val) / (len(data) - 1)
            print(data[0] + ',' + str(avg))