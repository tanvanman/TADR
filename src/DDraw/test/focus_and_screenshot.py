import ctypes, ctypes.wintypes as wt, os, time, struct, zlib

user32   = ctypes.windll.user32
gdi32    = ctypes.windll.gdi32

# Find the TotalA window
EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
found_hwnd = [0]

def enum_cb(hwnd, lParam):
    buf = ctypes.create_unicode_buffer(256)
    user32.GetWindowTextW(hwnd, buf, 256)
    title = buf.value
    cls   = ctypes.create_unicode_buffer(256)
    user32.GetClassNameW(hwnd, cls, 256)
    if 'Total Annihilation' in title or 'TotalA' in title:
        found_hwnd[0] = hwnd
        print(f'Found window: {title!r} class={cls.value!r} hwnd={hwnd}')
        return False
    return True

user32.EnumWindows(EnumWindowsProc(enum_cb), 0)

hwnd = found_hwnd[0]
if not hwnd:
    print('TA window not found — taking full screen anyway')
else:
    # Restore if minimised, then bring to foreground
    user32.ShowWindow(hwnd, 9)   # SW_RESTORE
    user32.SetForegroundWindow(hwnd)
    time.sleep(1.5)              # let it paint

# Full-screen capture
w = user32.GetSystemMetrics(0)
h = user32.GetSystemMetrics(1)

hdc_screen = user32.GetDC(0)
hdc_mem    = gdi32.CreateCompatibleDC(hdc_screen)

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
bih.biSize      = ctypes.sizeof(BITMAPINFOHEADER)
bih.biWidth     = w
bih.biHeight    = -h
bih.biPlanes    = 1
bih.biBitCount  = 24

stride = (w * 3 + 3) & ~3
buf = (ctypes.c_uint8 * (stride * h))()
hbmp = gdi32.CreateCompatibleBitmap(hdc_screen, w, h)
gdi32.SelectObject(hdc_mem, hbmp)
gdi32.BitBlt(hdc_mem, 0, 0, w, h, hdc_screen, 0, 0, 0x00CC0020)
gdi32.GetDIBits(hdc_mem, hbmp, 0, h, buf, ctypes.byref(bih), 0)
gdi32.DeleteObject(hbmp)
gdi32.DeleteDC(hdc_mem)
user32.ReleaseDC(0, hdc_screen)

raw_rows = []
for y in range(h):
    row = bytes(buf[y*stride : y*stride + w*3])
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

print(f'Saved {w}x{h} screenshot to {out}')
