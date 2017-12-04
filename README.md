# qt_camera_calibration
A Qt5 GUI to simplify the camera calibration process using OpenCV

The software allows to create a camera from scratch or to refine parameters previously calculated.

The calibration process is "Real Time", you can see how the "undistorted" image changes during the calibration/refine process for each Chessboard detected and evaluate the correctness of the whole process while advancing.

The software support the "standard" Pinhole Camera Model (using 8 distorsion parameters) and the FishEye model for camera with optics with a FOV bigger then 140°
