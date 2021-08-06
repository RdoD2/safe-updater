import sys
import struct
import os
import hashlib
from time import sleep



filename = ["boot.tar","opt.tar","documentation.tar"]

fwrite = open("FW","wb")



for f in filename:
    fsize = os.path.getsize(f)
    fp = open(f, "rb")
    fwrite.write(struct.pack("<i", len(f)))
    fwrite.write(f)
    fwrite.write(struct.pack("<i",fsize))
    fwrite.write(fp.read())
    fp.close()
    print("done")

fwrite.close()

sleep(1)

file = "FW" # Location of the file (can be set a different way)
BLOCK_SIZE = 65536 # The size of each read from the file

file_hash = hashlib.sha256() # Create the hash object, can use something other than `.sha256()` if you wish
with open(file, 'rb') as f: # Open the file to read it's bytes
    fb = f.read(BLOCK_SIZE) # Read from the file. Take in the amount declared above
    while len(fb) > 0: # While there is still data being read from the file
        file_hash.update(fb) # Update the hash
        fb = f.read(BLOCK_SIZE) # Read the next block from the file



f = open("hash.txt", 'w')
f.write(file_hash.hexdigest())
f.close

#sos.system('openssl enc -aes-128-ctr -a -in FW -out After -k "12341234912345678911234567832345" -iv "1234567891234567"')




