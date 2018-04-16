import glob
import os

LOG_DIR = "../logs"

if __name__ == '__main__':
    fnames = glob.glob(LOG_DIR + "/" + "*.txt")
    for fname in fnames:
        # Split out params from name
        base = os.path.basename(fname)
        short_name = os.path.splitext(base)[0]
        im_name, w, h, pcoeff, fcoeff, pmod = short_name.split("_")
        print(fname) 