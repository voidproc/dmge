import os
import glob
import re

os.chdir(os.path.dirname(os.path.abspath(__file__)))
os.chdir('../dmge/App')

for current_dir, sub_dirs, files_list in os.walk('cartridges'):
  for file_name in filter(lambda x: re.search(r'\.gb.?', x, flags=re.IGNORECASE), files_list):
    print(';Cartridge =', os.path.join(current_dir, file_name))
