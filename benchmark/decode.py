from subprocess import call

CWD = "../"
DEGREE = [8, 12, 16, 32, 48, 63]
DELTA = [0.1, 0.2, 0.3, 0.4, 0.5]
call(['./bin/decode_client', '-s'], cwd=CWD)
for degree in DEGREE:
    for delta in DELTA:
        call(['./bin/decode_server', '--degree', str(degree), '--delta', str(delta)], cwd=CWD)
        call(['./bin/decode_client', '-s', '-o', 'out_{}_{}.png'.format(degree, delta)], cwd=CWD)                    