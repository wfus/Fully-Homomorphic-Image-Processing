from subprocess import call

CWD = "../"
IMAGE_NAMES = [("image/test.jpg", 'test')]
WIDTH_HEIGHT_PAIRS = [(10, 10), (20, 20), (30, 30)]
LOG_DIR = "../logs"

def log_resize(imname, width, height, pcoeff, fcoeff, pmod):
    return "{}/resize_{}_{}_{}_{}_{}_{}.txt".format(LOG_DIR, imname, width, height, pcoeff, fcoeff, pmod)

def log_jpg(imname, pcoeff, fcoeff, pmod):
    return "{}/jpg_{}_{}_{}_{}_{}_{}.txt".format(LOG_DIR, imname, pcoeff, fcoeff, pmod)

def call_resize(image, width, height, logname, outname):
    f = open(logname, 'w')
    print("Resize Client (Sending)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--send', '-f', image], 
          cwd=CWD, stdout=f)
    print("Resize Server")
    call(['./bin/server_resize', '--width', str(width), '--height', str(height)], 
          cwd=CWD, stdout=f)
    print("Resize Client (Recieving)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--recieve', '-o', outname], 
          cwd=CWD, stdout=f)

def call_jpeg(image, logname, outname):
    f = open(logname, 'w')
    print("JPG Client (Sending)")
    call(['./bin/client_jpeg', '--send', '-f', image], 
          cwd=CWD, stdout=f)
    print("JPG Server")
    call(['./bin/server_jpeg'], 
          cwd=CWD, stdout=f)
    print("JPG Client (Recieving)")
    call(['./bin/client_jpeg', '--recieve', '-o', outname], 
          cwd=CWD, stdout=f)

if __name__ == '__main__':
    for image_name, short_name in IMAGE_NAMES:
        for width, height in WIDTH_HEIGHT_PAIRS:
            logname = log_resize(short_name, width, height, 0, 0, 0)
            outname = "image/{}_{}_{}.png".format(short_name, width, height)
            call_resize(image_name, width, height, logname, outname)
    for image_name, short_name in IMAGE_NAMES:
        logname = log_jpeg(short_name, width, height, 0, 0, 0)
        outname = "image/new_{}.jpg".format(short_name)
        call_jpeg(image_name, logname, outname)
          