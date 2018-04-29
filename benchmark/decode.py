import os

CWD = "../"
DEGREE = [8, 12, 16, 32, 48, 63]
DELTA = [0.1, 0.2, 0.3, 0.4, 0.5]
os.system("../bin/decode_client -s")
for degree in DEGREE:
    for delta in DELTA:
        os.system("../bin/decode_server --degree {} --delta {}".format(degree, delta))
        os.system("../bin/decode_client -r -o {}".format('out_{}_{}.png'.format(degree, delta)))
                    