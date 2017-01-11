# USAGE
# python object_movement.py --video object_tracking_example.mp4
# python object_movement.py

# import the necessary packages
from collections import deque
import numpy as np
import argparse
import imutils
import cv2
import serial, time
import socket
import sys
from time import sleep

#Choose COM port and baud rate accordingly
arduino = serial.Serial('COM5', 115200, timeout=.1)
time.sleep(1) #give the connection a second to settle

# Assumes host server has set up on port 5003 for UDP communication
host = '10.109.203.182'      #server's IPv4 goes here Harsha: 10.109.159.191
serverport = 5003

clientport = 5007
clientip = '10.109.146.22'  #this machine's IPV4 goes here

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #non blocking
s.settimeout(2)
print ('Created socket')
s.bind((clientip, clientport))
print(str(host) + " binded to port 5007...")

# for the server to identify the bot
license_plate = '1'
s.sendto(str.encode(license_plate), (host, serverport))
#follow = s.recvfrom(1048576)
#print("Following: " + str(follow))
#s.setblocking(0)
    
# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-v", "--video",
    help="path to the (optional) video file")
ap.add_argument("-b", "--buffer", type=int, default=32,
    help="max buffer size")
args = vars(ap.parse_args())

# define the lower and upper boundaries of the "green"
# ball in the HSV color space
greenLower = (29, 86, 6)
greenUpper = (64, 255, 255)

# initialize the list of tracked points, the frame counter,
# and the coordinate deltas
pts = deque(maxlen=args["buffer"])
counter = 0
radius = 0
(dX, dY, dZ) = (0, 0, 0)
direction = ""
sFilter = 0
scan = False
turn = False
switch = ""
# if a video path was not supplied, grab the reference
# to the webcam
if not args.get("video", False):
    camera = cv2.VideoCapture(0)

# otherwise, grab a reference to the video file
else:
    camera = cv2.VideoCapture(args["video"])

print("Running...")
# keep looping



while True:
##############################
    print("Before Client Message")
#    s.setblocking(0)
    try:
        switch = s.recv(1048576)
        d = str(switch)
        if d == "b'1'":
            turn = True
            print("Got it!")
        else:
            print("Not what I want!!!!!!!!!!!!!!!")
        print("Data: " + str(switch))
    except socket.timeout as e:
        err = e.args[0]
        # this next if/else is a bit redundant, but illustrates how the
        # timeout exception is setup
        if err == 'timed out':
            sleep(1)
            print ("recv timed out, retry later")
        else:
            print (e)
            sys.exit(1)
    except socket.error as e:
        # Something else happened, handle error, exit, etc.
        print (e)
        sys.exit(1)
    else:
        if len(switch) == 0:
            print ("orderly shutdown on server end")
            sys.exit(0)
        else:        
            print("Client Message: " + str(switch))
            print("After Client Message")

    # grab the current frame
    (grabbed, frame) = camera.read()
    ballFound = False
    (dX, dY, dz) = (0,0,0)
    (x, y, radius) = (0,0,0)
    # if we are viewing a video and we did not grab a frame,
    # then we have reached the end of the video
    if args.get("video") and not grabbed:
        break

    # resize the frame, blur it, and convert it to the HSV
    # color space
    frame = imutils.resize(frame, width=600)
    # blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # construct a mask for the color "green", then perform
    # a series of dilations and erosions to remove any small
    # blobs left in the mask
    mask = cv2.inRange(hsv, greenLower, greenUpper)
    mask = cv2.erode(mask, None, iterations=2)
    mask = cv2.dilate(mask, None, iterations=2)

    # find contours in the mask and initialize the current
    # (x, y) center of the ball
    cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,
        cv2.CHAIN_APPROX_SIMPLE)[-2]
    center = None

    # only proceed if at least one contour was found
    if len(cnts) > 0:
        # find the largest contour in the mask, then use
        # it to compute the minimum enclosing circle and
        # centroid
        c = max(cnts, key=cv2.contourArea)
        ((x, y), radius) = cv2.minEnclosingCircle(c)
        M = cv2.moments(c)
        center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))

        # only proceed if the radius meets a minimum size
        if radius > 10:
            ballFound = True
            # draw the circle and centroid on the frame,
            # then update the list of tracked points
            cv2.circle(frame, (int(x), int(y)), int(radius),
                (0, 255, 255), 2)
            cv2.circle(frame, center, 5, (0, 0, 255), -1)
            pts.appendleft(center)

    # loop over the set of tracked points
    for i in np.arange(1, len(pts)):
        # if either of the tracked points are None, ignore
        # them
        if pts[i - 1] is None or pts[i] is None:
            continue
        
        try:
            pts[-10]
        except IndexError:
            ballFound = False
            
        # check to see if enough points have been accumulated in
        # the buffer
        if ballFound == True and counter >= 10 and i == 1 and pts[-10] is not None:
            # compute the difference between the x and y
            # coordinates and re-initialize the direction
            # text variables
            dX = pts[-10][0] - pts[i][0]
            dY = pts[-10][1] - pts[i][1]
            (dirX, dirY) = ("", "")

            # ensure there is significant movement in the
            # x-direction
            if np.abs(dX) > 20:
                dirX = "East" if np.sign(dX) == 1 else "West"

            # ensure there is significant movement in the
            # y-direction
            if np.abs(dY) > 20:
                dirY = "North" if np.sign(dY) == 1 else "South"

            # handle when both directions are non-empty
            if dirX != "" and dirY != "":
                direction = "{}-{}".format(dirY, dirX)

            # otherwise, only one direction is non-empty
            else:
                direction = dirX if dirX != "" else dirY

        # otherwise, compute the thickness of the line and
        # draw the connecting lines
        if ballFound == True:
            thickness = int(np.sqrt(args["buffer"] / float(i + 1)) * 2.5)
            cv2.line(frame, pts[i - 1], pts[i], (0, 0, 255), thickness)

    # show the movement deltas and the direction of movement on
    # the frame
    #calculate the int radius, int x, int y
    rad = int(round(radius))
    xAxis = int(round(x))
    yAxis = int(round(y))
    radiusInRange = rad < 45 and rad >= 10
    
    #Turn states for arduino. 
    #Stop = 0
    #Straight = 1
    #left = 2
    #right = 3
    #scan = 4
    #transition = 5
    movement = 0
    
    if rad >= 45:
        movement = 0
        
    if xAxis >= 250 and xAxis <= 350 and radiusInRange:
        movement = 1
        
    if xAxis <= 250 and radiusInRange:
        movement = 2
        
    if xAxis >= 350 and radiusInRange:
        movement = 3
    
    if rad < 10:
        movement = 4
    
    if turn == True:
        movement = 5
        i = 0
        print("Sending 80 times bro....")
        while i < 80:
            i = i + 1
            arduino.write("%d".encode('ascii') % movement)
        turn = False
        continue
    
    movement = 5
    #arduino will process this state accordingly
    arduino.write("%d".encode('ascii') % movement)
   
   ####### DEBUG ON SERIAL ########
   # data = arduino.readline()
   # if data:
   #     print(data)
   ################################
   
    #Place direction info on screen
    cv2.putText(frame, direction, (10, 30), cv2.FONT_HERSHEY_SIMPLEX,
        0.65, (0, 0, 255), 3)
    cv2.putText(frame, "x: {}, y: {}, z: {}".format(xAxis, yAxis, rad),
        (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX,
        0.35, (0, 0, 255), 1)
    
# Track movement. dX, dY, Z    
#    cv2.putText(frame, "dx: {}, dy: {}, z: {}".format(dX, dY, rad),
#        (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX,
#        0.35, (0, 0, 255), 1)

    # show the frame to our screen and increment the frame counter
    cv2.imshow("Frame", frame)
    key = cv2.waitKey(1) & 0xFF
    counter += 1

    # if the 'q' key is pressed, stop the loop
    if key == ord("q"):
        break

# cleanup the camera and close any open windows
camera.release()
cv2.destroyAllWindows()