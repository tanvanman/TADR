import os, time, sys

pipe_name = '\\\\.\\pipe\\tadr-debug'

def send_cmd(p, cmd):
    p.write((cmd + '\n').encode())
    resp = p.readline().decode().strip()
    print(f'  {cmd!r} -> {resp}')
    return resp

print('Setting up scenario...')
with open(pipe_name, 'r+b', buffering=0) as p:
    send_cmd(p, 'suppress_broadcast 1')
    send_cmd(p, 'reset_votes')

    # 4 players: local (slot 0), ally target (slot 1), foe target (slot 2), proposer (slot 3)
    send_cmd(p, 'setup_player 0 1 Local')
    send_cmd(p, 'setup_player 1 2 AllyPlayer')
    send_cmd(p, 'setup_player 2 3 FoePlayer')
    send_cmd(p, 'setup_player 3 4 Proposer')

    send_cmd(p, 'set_local 0')
    send_cmd(p, 'set_progress 3')

    # Local (slot 0) and AllyPlayer (slot 1) are allies
    send_cmd(p, 'set_ally 0 1 1')

    # Timeout vote on AllyPlayer (dpid=2) — local is allied, should show .take
    send_cmd(p, 'inject_propose 4 2 6')
    # Timeout vote on FoePlayer  (dpid=3) — local is not allied, no .take
    send_cmd(p, 'inject_propose 4 3 6')

    state = send_cmd(p, 'dump_votes')
    print('Vote state:', state)

print('Waiting for frame to render...')
time.sleep(2)

print('Taking screenshot...')
import ctypes, ctypes.wintypes as wt, struct, zlib

# GDI screenshot to BMP then save as PNG via ctypes only
user32  = ctypes.windll.user32
gdi32   = ctypes.windll.gdi32
kernel32 = ctypes.windll.kernel32

# Screen dimensions
w = user32.GetSystemMetrics(0)
h = user32.GetSystemMetrics(1)

hdc_screen  = user32.GetDC(0)
hdc_mem     = gdi32.CreateCompatibleDC(hdc_screen)

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ('biSize',          ctypes.c_uint32),
        ('biWidth',         ctypes.c_int32),
        ('biHeight',        ctypes.c_int32),
        ('biPlanes',        ctypes.c_uint16),
        ('biBitCount',      ctypes.c_uint16),
        ('biCompression',   ctypes.c_uint32),
        ('biSizeImage',     ctypes.c_uint32),
        ('biXPelsPerMeter', ctypes.c_int32),
        ('biYPelsPerMeter', ctypes.c_int32),
        ('biClrUsed',       ctypes.c_uint32),
        ('biClrImportant',  ctypes.c_uint32),
    ]

bih = BITMAPINFOHEADER()
bih.biSize        = ctypes.sizeof(BITMAPINFOHEADER)
bih.biWidth       = w
bih.biHeight      = -h  # top-down
bih.biPlanes      = 1
bih.biBitCount    = 24
bih.biCompression = 0   # BI_RGB

# row stride must be multiple of 4
stride = (w * 3 + 3) & ~3
buf = (ctypes.c_uint8 * (stride * h))()

hbmp = gdi32.CreateCompatibleBitmap(hdc_screen, w, h)
gdi32.SelectObject(hdc_mem, hbmp)
gdi32.BitBlt(hdc_mem, 0, 0, w, h, hdc_screen, 0, 0, 0x00CC0020)  # SRCCOPY
gdi32.GetDIBits(hdc_mem, hbmp, 0, h, buf, ctypes.byref(bih), 0)

gdi32.DeleteObject(hbmp)
gdi32.DeleteDC(hdc_mem)
user32.ReleaseDC(0, hdc_screen)

# Convert BGR rows to RGB and build a minimal PNG
raw_rows = []
for y in range(h):
    row = bytes(buf[y*stride : y*stride + w*3])
    # BGR -> RGB
    rgb = bytearray(w*3)
    for x in range(w):
        rgb[x*3]   = row[x*3+2]
        rgb[x*3+1] = row[x*3+1]
        rgb[x*3+2] = row[x*3]
    raw_rows.append(b'\x00' + bytes(rgb))

def png_chunk(tag, data):
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', crc)

ihdr_data = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
idat_data = zlib.compress(b''.join(raw_rows), 6)

out = os.path.join(os.path.dirname(__file__), 'vote_dialog_take.png')
with open(out, 'wb') as f:
    f.write(b'\x89PNG\r\n\x1a\n')
    f.write(png_chunk(b'IHDR', ihdr_data))
    f.write(png_chunk(b'IDAT', idat_data))
    f.write(png_chunk(b'IEND', b''))

print('Saved to', out)
