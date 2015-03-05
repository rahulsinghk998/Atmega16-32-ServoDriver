# Atmega16-32-ServoDriver

This repo contains code for Servo Controller for Atmega16/32 where as many as 12 servos can be controlled.
To control a Servo, the user has to send a string of data containing ServoID and Servo Angle.Eg Suppose I want to control 
the 2nd servo and rotate it to an angle 67 degree. So the command which I will send is '2''6''7''ENTER'.The last character
will be ENTER to terminate the datasending and let the Controller know that it is the end of string.
        The ASCII value of string is 13.
