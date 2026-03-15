import time, sys

pipe_name = '\\\\.\\pipe\\tadr-debug'

for attempt in range(20):
    try:
        with open(pipe_name, 'r+b', buffering=0) as p:
            p.write(b'dump_votes\n')
            resp = p.readline()
            print(f"Pipe ready: {resp.decode().strip()}")
            sys.exit(0)
    except OSError as e:
        print(f"Attempt {attempt+1}: {e}", flush=True)
        time.sleep(1)

print("Pipe not available after 20s")
sys.exit(1)
