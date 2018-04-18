from subprocess import call

CWD = "../"
IMAGE_NAMES = [("image/boazbarak.jpg", 'boaz')]
POLY_N = [2048, 4096, 8192, 16384]
PLAIN_MOD = [11, 31, 101, 307, 1009, 3001, 10007, 30011, 100003]
WIDTH_HEIGHT_PAIRS = [(17, 17)]
LOG_DIR = "../logs"

def log_resize(imname, inter, width, height, poly_n, plain_mod):
    return "{}/resize_{}_{}_{}_{}_{}_{}.txt".format(LOG_DIR, imname, inter, width, height, poly_n, plain_mod)

def log_jpeg(imname, poly_n, plain_mod):
    return "{}/jpg_{}_{}_{}.txt".format(LOG_DIR, imname, poly_n, plain_mod)

def call_resize(image, logname, outname, inter, width, height, poly_n, plain_mod):
    f = open(logname, 'w')
    print("Resize Client (Sending)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--send', '-f', image, '--cmod', str(poly_n), '--pmod', str(plain_mod)], 
          cwd=CWD, stdout=f)
    print("Resize Server")
    call(['./bin/server_resize', '--width', str(width), '--height', str(height), inter, '--cmod', str(poly_n), '--pmod', str(plain_mod)], 
          cwd=CWD, stdout=f)
    print("Resize Client (Recieving)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--recieve', '-f', image, '-o', outname, '--cmod', str(poly_n), '--pmod', str(plain_mod)], 
          cwd=CWD, stdout=f)

def call_jpeg(image, logname, outname, poly_n, plain_mod):
    f = open(logname, 'w')
    print("JPG Client (Sending)")
    call(['./bin/client_jpeg', '--send', '-f', image, '--cmod', str(poly_n), '--pmod', str(plain_mod)], 
          cwd=CWD, stdout=f)
    print("JPG Server")
    call(['./bin/server_jpeg', '--cmod', str(poly_n), '--pmod', str(plain_mod)], 
          cwd=CWD, stdout=f)
    print("JPG Client (Recieving)")
    call(['./bin/client_jpeg', '--recieve', '-f', image, '-o', outname, '--cmod', str(poly_n), '--pmod', str(plain_mod)],  
          cwd=CWD, stdout=f)

if __name__ == '__main__':
    for image_name, short_name in IMAGE_NAMES:
        for poly_n in POLY_N:
            for plain_mod in PLAIN_MOD:
                for width, height in WIDTH_HEIGHT_PAIRS:
                    for inter in ['bilinear', 'bicubic']:
                        logname = log_resize(short_name, inter, width, height, poly_n, plain_mod)
                        outname = "logs/{}_{}_{}_{}_{}_{}.png".format(short_name, inter, width, height, poly_n, plain_mod)
                        inter_param = '' if inter == 'bilinear' else '--bicubic'
                        call_resize(image_name, logname, outname, inter_param, width, height, poly_n, plain_mod)
                
                logname = log_jpeg(short_name, poly_n, plain_mod)
                outname = "logs/new_{}_{}_{}.jpg".format(short_name, poly_n, plain_mod)
                call_jpeg(image_name, logname, outname, poly_n, plain_mod)
                    