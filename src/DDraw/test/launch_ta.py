import os, subprocess

ta_dir = os.environ.get('TA_DIR', r'D:\games\TAF-Core Contingency')
subprocess.Popen(
    [os.path.join(ta_dir, 'TotalA.exe')],
    cwd=ta_dir,
)
print("TA launched")
