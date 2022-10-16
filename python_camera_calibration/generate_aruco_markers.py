# import the necessary packages
import argparse
import imutils
import cv2
import sys
import numpy as np
import os

folderPath ='./aruco_markers/'
os.makedirs(folderPath, exist_ok=True)

dictionary = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_6X6_250)

markerImage = np.zeros((300, 300, 1), dtype="uint8")

for i in range(0, 250):
	cv2.aruco.drawMarker(dictionary, i, 300, markerImage, 1) #was 23 in the example at https://docs.opencv.org/4.x/d5/dae/tutorial_aruco_detection.html
	cv2.imwrite(os.path.join(folderPath, "marker_" + str(1000+i)[1:] + ".png"), markerImage)
