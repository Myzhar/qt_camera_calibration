# qt_camera_calibration
A Qt5 GUI to simplify the camera calibration process using OpenCV

The software allows to create a camera model from scratch or to refine the parameters of a model previously calculated.

The calibration process is "Real Time", you can see how the "undistorted" image changes during the calibration/refine process for each Chessboard detected, so you can evaluate the correctness of the whole process while advancing.

The software support the "standard" Pinhole Camera Model (using 8 distorsion parameters) and the FishEye model for camera with optics with a FOV bigger then 140°

1. Launch the application.
2. If necessary, select a camera.
3. If necessary, select Resolution and FPS.
4. Click the "Start Camera" button to start.
5. Check / uncheck "Fisheye" checkbox according to the mode required.
6. If necessary, modify Chessboard parameters (Number of inner corners per a chessboard row and column etc.).
7. Click "Calibrate" to start calibration.
8. Click "Calibrate" again when you have finished collecting projection images.
9. Click "Save" to save the camera settings.

On Camera Calibration with OpenCV: https://docs.opencv.org/4.x/d4/d94/tutorial_camera_calibration.html
