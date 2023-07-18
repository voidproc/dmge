import os
import re
from shutil import copy, make_archive

os.chdir(os.path.dirname(os.path.abspath(__file__)))


# バージョン文字列を取得

version = ''

with open('../dmge/version.h', 'r', encoding='utf8') as f:
  for line in f.readlines():
    m = re.search(r'v\d+\.\d+\.\d+', line)
    if m:
      version = m[0]

if version == '':
  print('version string not found')
  exit(-1)

dirname = 'dmge_' + version

try:
  os.mkdir(dirname)
except FileExistsError:
  pass

copy('../dmge/App/dmge.exe', dirname)
copy('../dmge/App/config.example.ini', dirname)
copy('../pub/readme.txt', dirname)

make_archive(dirname, 'zip', dirname)
