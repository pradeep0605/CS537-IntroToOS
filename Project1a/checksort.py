#! /usr/bin/env python
import struct
import sys

# get unsigned integer data from binary string
def getUint(rec):
   (data,) = struct.unpack("I", rec)
   return data

# check that outputFile is sorted and contains the same records as inputFile
def checkSort(inputFile, outputFile):
   countDict = {}
   recLen = 4
   recNum = 0

   with open(inputFile, "rb") as f:
      recNum = getUint(f.read(recLen))
      curNum = 0
      while curNum < recNum:
         key = getUint(f.read(recLen))
         dataNumStr = f.read(recLen)
         dataNum = getUint(dataNumStr)
         dataStr = f.read(recLen * dataNum)
         dataStr = dataNumStr + dataStr
         if key not in countDict:
            countDict[key] = {}
         if dataStr not in countDict[key]:
            countDict[key][dataStr] = 0
         countDict[key][dataStr] += 1
         curNum += 1

   try:
      with open(outputFile, "rb") as f:
         testRecNum = getUint(f.read(recLen))
         if testRecNum != recNum:
            sys.exit(1)
         curNum = 0
         prevKey = -1
         while curNum < testRecNum:
            key = getUint(f.read(recLen))
            if key < prevKey:
               sys.exit(1)
            dataNumStr = f.read(recLen)
            dataNum = getUint(dataNumStr)
            dataStr = f.read(recLen * dataNum)
            dataStr = dataNumStr + dataStr
            if key not in countDict: 
               sys.exit(1)
            elif dataStr not in countDict[key]:
               sys.exit(1)
            countDict[key][dataStr] -= 1
            if countDict[key][dataStr] == 0:
               del countDict[key][dataStr]
               if not countDict[key]:
                  del countDict[key]
            curNum += 1
            prevKey = key
 
         if f.read(recLen) != '' or countDict:
            sys.exit(1)
   except:
      sys.exit(1)
   sys.exit(0)

if __name__ == "__main__":
   if len(sys.argv) != 3:
      print "Usage: check.py inputfile outputfile"
      sys.exit(2)
   checkSort(sys.argv[1], sys.argv[2])
