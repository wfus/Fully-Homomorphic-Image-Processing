from subprocess import call

CWD = "../"
IMAGE_NAMES = [("image/test.jpg", 'test')]
POLY_N = [2048, 4096, 8192, 16384]
ENC_BASE = [2, 3, 5, 11, 19, 29]
PLAIN_MOD = [4096, 8192, 16384, 32768]
WIDTH_HEIGHT_PAIRS = [(10, 10), (20, 20), (30, 30)]
LOG_DIR = "../logs"

def log_resize(imname, inter, width, height, poly_n, plain_mod, enc_base):
    return "{}/resize_{}_{}_{}_{}_{}_{}_{}.txt".format(LOG_DIR, imname, inter, width, height, poly_n, plain_mod, enc_base)

def log_jpeg(imname, poly_n, plain_mod, enc_base):
    return "{}/jpg_{}_{}_{}_{}.txt".format(LOG_DIR, imname, poly_n, plain_mod, enc_base)

def call_resize(image, logname, outname, inter, width, height, poly_n, plain_mod, enc_base):
    f = open(logname, 'w')
    print("Resize Client (Sending)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--send', '-f', image, '--cmod', str(poly_n), '--pmod', str(plain_mod), '--base', str(enc_base)], 
          cwd=CWD, stdout=f)
    print("Resize Server")
    call(['./bin/server_resize', '--width', str(width), '--height', str(height), inter, '--cmod', str(poly_n), '--pmod', str(plain_mod), '--base', str(enc_base)], 
          cwd=CWD, stdout=f)
    print("Resize Client (Recieving)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--recieve', '-o', outname, '--cmod', str(poly_n), '--pmod', str(plain_mod), '--base', str(enc_base)], 
          cwd=CWD, stdout=f)

def call_jpeg(image, logname, outname, poly_n, plain_mod, enc_base):
    f = open(logname, 'w')
    print("JPG Client (Sending)")
    call(['./bin/client_jpeg', '--send', '-f', image, '--cmod', str(poly_n), '--pmod', str(plain_mod), '--base', str(enc_base)], 
          cwd=CWD, stdout=f)
    print("JPG Server")
    call(['./bin/server_jpeg', '--cmod', str(poly_n), '--pmod', str(plain_mod), '--base', str(enc_base)], 
          cwd=CWD, stdout=f)
    print("JPG Client (Recieving)")
    call(['./bin/client_jpeg', '--recieve', '-o', outname, '--cmod', str(poly_n), '--pmod', str(plain_mod), '--base', str(enc_base)],  
          cwd=CWD, stdout=f)

if __name__ == '__main__':
    for image_name, short_name in IMAGE_NAMES:
        for poly_n in POLY_N:
            for plain_mod in PLAIN_MOD:
                for enc_base in ENC_BASE:
                    for width, height in WIDTH_HEIGHT_PAIRS:
                        for inter in ['bilinear', 'bicubic']:
                            logname = log_resize(short_name, inter, width, height, poly_n, plain_mod, enc_base)
                            outname = "image/{}_{}_{}_{}_{}_{}_{}.png".format(short_name, inter, width, height, poly_n, plain_mod, enc_base)
                            inter_param = '' if inter == 'bilinear' else '--bicubic'
                            call_resize(image_name, logname, outname, inter_param, width, height, poly_n, plain_mod, enc_base)
                    
                    logname = log_jpeg(short_name, poly_n, plain_mod, enc_base)
                    outname = "image/new_{}_{}_{}_{}.jpg".format(short_name, poly_n, plain_mod, enc_base)
                    call_jpeg(image_name, logname, outname, poly_n, plain_mod, enc_base)
                    