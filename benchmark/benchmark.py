from subprocess import call

CWD = "../"
IMAGE_NAMES = [("image/test.jpg", 'test')]
WIDTH_HEIGHT_PAIRS = [(10, 10), (20, 20), (30, 30)]


def log_name(imname, width, height, pcoeff, fcoeff, pmod):
    LOG_DIR = "../logs"
    return "{}/{}_{}-{}_{}_{}_{}.txt".format(LOG_DIR, imname, width, height, pcoeff, fcoeff, pmod)


def call_command(image, width, height, logname):
    f = open(logname, 'w')
    print("Client (Sending)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--send'], 
          cwd=CWD, stdout=f)
    print("Server")
    call(['./bin/server_resize', '--width', str(width), '--height', str(height)], 
          cwd=CWD, stdout=f)
    print("Client (Recieving)")
    call(['./bin/client_resize', '--width', str(width), '--height', str(height), '--recieve'], 
          cwd=CWD, stdout=f)
    

if __name__ == '__main__':
    for image_name, short_name in IMAGE_NAMES:
        for width, height in WIDTH_HEIGHT_PAIRS:
            logname = log_name(short_name, width, height, 0, 0, 0)
            call_command(image_name, width, height, logname)
          