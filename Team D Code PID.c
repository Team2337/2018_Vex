#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in1,    sensorGyro,     sensorGyro)
#pragma config(Sensor, dgtl1,  bottomArmPos,   sensorTouch)
#pragma config(Sensor, dgtl2,  topArmPos,      sensorTouch)
#pragma config(Sensor, dgtl3,  autonChooser,   sensorTouch)
#pragma config(Sensor, I2C_1,  rightEncoder,   sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Sensor, I2C_2,  leftEncoder,    sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Motor,  port1,           backLeft,      tmotorVex393_HBridge, openLoop, reversed)
#pragma config(Motor,  port2,           rightArm2,     tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port3,           leftFront,     tmotorVex393_MC29, openLoop, encoderPort, I2C_2)
#pragma config(Motor,  port4,           backRight,     tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port5,           rightFront,    tmotorVex393_MC29, openLoop, reversed, encoderPort, I2C_1)
#pragma config(Motor,  port6,           leftArm,       tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port7,           rightArm,      tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port8,           claw,          tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port9,           leftArm2,      tmotorVex393_MC29, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

/*******************************************************************/
/* Imports the Debuggar Window,allowing for values to be displayed */
/* while the Robot is running																			 */
/*  !!!!!  MUST HAVE THE USB -> CONTROLLER ADAPTER TO WORK  !!!!!  */
/*******************************************************************/
#pragma platform(VEX2)

#pragma DebuggerWindows("DebugStream")

#pragma competitionControl(Competition)

#include "Vex_Competition_Includes.c"

// PID using optical shaft encoder
//
// Shaft encoder has 360 pulses per revolution
//



/**********************************/
/* ------------------------------ */
/* -----------  VOCAB  ---------- */
/* ------------------------------ */
/**********************************/

/*
* PID: Preportional Integral Derivative
* Used to move the motors until the desired position has been reached
*/

//Setpoint: The desired ditance
//Error: Desired distance - current distance
//Ticks: value given off by encoders to track distance
//Constant: Value that remains the same



/*************************/
/* ----- CONSTANTS ----- */
/*************************/

//Chassis PID Encoders
#define PID_SENSOR_INDEX_L		leftEncoder
#define PID_SENSOR_INDEX_R    rightEncoder
#define PID_SENSOR_SCALE      1

//Chassis PID Motors
#define PID_MOTOR_INDEX_L			leftFront
#define PID_MOTOR_INDEX_R     rightFront
#define PID_MOTOR_SCALE       -1

#define PID_DRIVE_MAX       120
#define PID_DRIVE_MIN     (-120)

#define PID_INTEGRAL_LIMIT  50

//Arm Motors
#define ARM_MOTOR_INDEX_R		rightArm
#define ARM_MOTOR_INDEX_L		leftArm

//Arm Encoders
#define ARM_SENSOR_INDEX_R
#define ARM_SENSOR_INDEX_L

//Claw Motors
#define CLAW_MOTOR_INDEX		claw

/********************/
/* -- PID VALUES -- */
/********************/

//"K" tells us it's a constant value, and should not be changed insidee the code
float  pid_Kp = 0.5;
float  pid_Ki = 0.00;
float  pid_Kd = 0.0;

int pidRunTimes = 0;
int commandTimes = 0;

//static int   pidRunning = 1;
static float pidRequestedValueRight;
static float pidRequestedValueLeft;

//Gyro values
static float degrees = 0;

static string armPosition = "DOWN";

bool pidTaskEnded;

/********************************/
/* ---------------------------- */
/* --- USER CONTROL METHODS --- */
/* ---------------------------- */
/********************************/

//Sets the set point on the encoders by inches
//In goes # of inches, out comes encoder value
void setDriveDistance(float distRight, float distLeft) {
	//commandTimes++;
	//Sets the setpoint on the encoder
	pidRequestedValueRight = distRight * 585;
	pidRequestedValueLeft  = distLeft  * 585;
}

//Sets the set point on the encoders by rotations
//In goes # of rotations, out comes encoder value
void setDriveRotation(float rotationsRight, float rotationsLeft) {
	//Sets the setpoint on the encoder
	pidRequestedValueRight = (rotationsRight * 12.4) * 53.5;
	pidRequestedValueLeft  = (rotationsLeft  * 12.4) * 53.5;
}

void setRotationDegree(float distRight, float distLeft) {
	//Sets the setpoint on the encoder
	//5,5 = 180 //Need to change later
	pidRequestedValueRight = distRight * 585;
	pidRequestedValueLeft  = distLeft  * -585;
}





/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  pid control task                                                           */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

task pidController() {
	float  pidSensorCurrentValueRight;
	float  pidSensorCurrentValueLeft;

	float  pidErrorRight;
	float  pidErrorLeft;

	float  pidLastErrorRight;
	float  pidLastErrorLeft;

	float  pidIntegralRight;
	float  pidIntegralLeft;

	float  pidDerivativeRight;
	float  pidDerivativeLeft;

	float  pidDriveRight;
	float  pidDriveLeft;

	stopTask(autonomous);

	// If we are using an encoder then clear it
	if( SensorType[ PID_SENSOR_INDEX_R ] == sensorQuadEncoderOnI2CPort &&
		SensorType[ PID_SENSOR_INDEX_L ] == sensorQuadEncoderOnI2CPort) {
		SensorValue[ PID_SENSOR_INDEX_R ] = 0;
		SensorValue[ PID_MOTOR_INDEX_L ] = 0;
	}

	// Init the variables
	pidLastErrorRight = 0;
	pidLastErrorLeft  = 0;

	pidIntegralRight  = 0;
	pidIntegralLeft   = 0;

	pidTaskEnded = false;

	while( true ) {
		// Is PID control active ?
		//Left is negative right is positive
		if( !(SensorValue[ PID_SENSOR_INDEX_R ] > ((abs(pidRequestedValueRight)) - 40))
			|| !(SensorValue[ PID_SENSOR_INDEX_L ] < ((abs(pidRequestedValueLeft)) - 40))) {
			// Read the sensor value and scale
			pidSensorCurrentValueRight = -1 * (SensorValue[ PID_SENSOR_INDEX_R ] * PID_SENSOR_SCALE);
			pidSensorCurrentValueLeft = SensorValue[ PID_SENSOR_INDEX_L ] * PID_SENSOR_SCALE;


			// calculate error
			pidErrorRight = pidSensorCurrentValueRight - pidRequestedValueRight;
			pidErrorLeft  = pidSensorCurrentValueLeft  - pidRequestedValueLeft;

			// integral - if Ki is not 0
			if( pid_Ki != 0 ) {
				// If we are inside controlable window then integrate the error
				if( abs(pidErrorRight) < PID_INTEGRAL_LIMIT ) {
					pidIntegralRight += pidErrorRight;
					} else {
					pidIntegralRight = 0;
				}
				} else {
				pidIntegralRight = 0;
			}

			if( pid_Ki != 0 ) {
				// If we are inside controlable window then integrate the error
				if( abs(pidErrorLeft) < PID_INTEGRAL_LIMIT ) {
					pidIntegralLeft += pidErrorLeft;
					} else {
					pidIntegralLeft = 0;
				}
				} else {
				pidIntegralLeft = 0;
			}

			// calculate the derivative
			pidDerivativeRight = pidErrorRight - pidLastErrorRight;
			pidLastErrorRight  = pidErrorRight;

			pidDerivativeLeft = pidErrorLeft - pidLastErrorLeft;
			pidLastErrorLeft  = pidErrorLeft;


			// calculate drive
			pidDriveRight = (pid_Kp * pidErrorRight) + (pid_Ki * pidIntegralRight) + (pid_Kd * pidDerivativeRight);
			pidDriveLeft = (pid_Kp * pidErrorLeft) + (pid_Ki * pidIntegralLeft) + (pid_Kd * pidDerivativeLeft);

			// limit drive
			if( pidDriveRight > PID_DRIVE_MAX )
				pidDriveRight = PID_DRIVE_MAX;
			if( pidDriveRight < PID_DRIVE_MIN )
				pidDriveRight = PID_DRIVE_MIN;

			if( pidDriveLeft > PID_DRIVE_MAX )
				pidDriveLeft = PID_DRIVE_MAX;
			if( pidDriveLeft < PID_DRIVE_MIN )
				pidDriveLeft = PID_DRIVE_MIN;


			// send to motor
			//sets speed of the motor
			motor[ PID_MOTOR_INDEX_R ] = pidDriveRight * PID_MOTOR_SCALE;
			motor[ backRight] = pidDriveRight * PID_MOTOR_SCALE;

			motor[ PID_MOTOR_INDEX_L ] = pidDriveLeft * PID_MOTOR_SCALE;
			motor[ backLeft] = pidDriveLeft * PID_MOTOR_SCALE;
			} else {
			// clear all
			pidErrorRight      = 0;
			pidLastErrorRight  = 0;
			pidIntegralRight   = 0;
			pidDerivativeRight = 0;

			pidErrorLeft       = 0;
			pidLastErrorLeft   = 0;
			pidIntegralLeft    = 0;
			pidDerivativeLeft  = 0;

			pidTaskEnded = true;
			pidRunTimes ++;

			motor[ PID_MOTOR_INDEX_R ] = 0;
			motor[ PID_MOTOR_INDEX_L ] = 0;

			writeDebugStreamLine("COMMAND - FINISHED");

			startTask(autonomous);
		}

		writeDebugStreamLine("Right Encoder: %d", SensorValue(PID_SENSOR_INDEX_R));
		writeDebugStreamLine("Left Encoder: %d", SensorValue(PID_SENSOR_INDEX_L));

		/*
		if(SensorValue(PID_SENSOR_INDEX_R) >= pidRequestedValueRight - 50 ||
		SensorValue(PID_SENSOR_INDEX_R) <= pidRequestedValueRight + 50) {
		stopTask(pidController);
		}*/
		// Run at 50Hz
		wait1Msec( 25 );
	}
}

/*
Moves the arm to the desired position
Uses two touch sensors to tell where the arm should go
The bottom touch sensor tells when the arm is in the down position
The top touch sensor tells when the arm is in the up position
When it's at the top, the motors move at 10% to hold the arm in the air
*/
task moveArm() {
	while(true) {
		if(armPosition == "DOWN") {
			if(!SensorValue(bottomArmPos)) {
				motor[ARM_MOTOR_INDEX_R] = -100;
				motor[ARM_MOTOR_INDEX_L] = -100;
				} else {
				motor[ARM_MOTOR_INDEX_R] = 0;
				motor[ARM_MOTOR_INDEX_L] = 0;
			}
			} else if(armPosition == "UP") {
			if(!SensorValue(topArmPos)) {
				motor[ARM_MOTOR_INDEX_R] = 100;
				motor[ARM_MOTOR_INDEX_L] = 100;
				} else {
				motor[ARM_MOTOR_INDEX_R] = 10;
				motor[ARM_MOTOR_INDEX_L] = 10;
			}
		}
	}
}

/*******************************************/
/*-----------------------------------------*/
/*--------  AUTON COMMAND METHODS  --------*/
/*-----------------------------------------*/
/*******************************************/


/**
* desiredDistRight: distance in feet on the right side of the chassis
* desiredDistLeft: distance in feet on the left side of the chassis
*/
void driveForwardToPosition(float desiredDistRight, float desiredDistLeft) {

	//sets the encoders to zero
	//may change for the future to provide more accurate headings.
	SensorValue[ PID_SENSOR_INDEX_R ] = 0;
	SensorValue[ PID_MOTOR_INDEX_L ] = 0;

	setDriveDistance(desiredDistRight, desiredDistLeft);

	startTask(pidController);
}


/**
* angle: the angle the robot will turn to
* encoderSide: the side of the chassis that is read
*(Read left when turning right, read right when turning left)
*/
void driveToAngle(float angle, bool encoderSide) {
	SensorValue[ PID_SENSOR_INDEX_R ] = 0;
	SensorValue[ PID_MOTOR_INDEX_L ] = 0;

	if(encoderSide) {
		setDriveDistance(angle/90, angle/-90);
		} else {
		setDriveDistance(angle/-90, angle/90);
	}
	startTask(pidController);
}

//stops the chassis
void stopDrive() {
	motor[rightFront] = 0;
	motor[backRight] = 0;
	motor[leftFront] = 0;
	motor[backLeft] = 0;
}

//set the desired position of the arm
void setArmPosition(string position) {
	armPosition = position;
	startTask(moveArm);
}

/*The chassis moves to the given position while the arm moves at the same time*/
void driveToPositionWithArm(float rightDist, float leftDist, string armPos) {
	setArmPosition(armPos);
	driveForwardToPosition(rightDist, leftDist);
}

task gyroControl() {

//Completely clear out any previous sensor readings by setting the port to "sensorNone"
 SensorType[in1] = 0;
 wait1Msec(1000);
 //Reconfigure Analog Port 1 as a Gyro sensor and allow time for ROBOTC to calibrate it
 SensorType[in1] = sensorGyro;
 wait1Msec(2000);
 degrees = SensorValue(sensorGyro);
 writeDebugStreamLine("Gyro Ticks: %d", SensorValue(sensorGyro));
}

void pre_auton() {

	bStopTasksBetweenModes = true;
	//used for setting values before auton
}

task autonomous() {

	stopTask(pidController);
	stopTask(usercontrol);

	startTask(gyroControl);

	writeDebugStreamLine("AUTON - STARTED");

	motor[leftArm] = -100;
	motor[rightArm] = -100;
	wait1Msec(250);
	motor[leftArm] = 100;
	motor[rightArm] = 100;
	wait1Msec(250);

	if(autonChooser) {
		writeDebugStreamLine("AUTON - BLUE");
		if (pidRunTimes == 0) {
			driveForwardToPosition(2, 2);
			wait1Msec(2000);
			motor[claw] = -65;
			wait1Msec(250);
			motor[claw] = 0;
			wait1Msec(1);
			driveForwardToPosition(-0.5, -0.5);
			} else if(pidTaskEnded && pidRunTimes == 1) {
			writeDebugStreamLine("COMMAND --- 1 --- FINISHED");
			driveToAngle(90, true);
			} else if(pidTaskEnded && pidRunTimes == 2) {
			writeDebugStreamLine("COMMAND --- 2 --- FINISHED");
			string top = "TOP";
			driveToPositionWithArm(3, 3, top); //cannot input raw string value, must be assigned to a variable
			} else if(pidTaskEnded && pidRunTimes == 3) {
			writeDebugStreamLine("COMMAND --- 3 --- FINISHED");
			stopDrive();
			writeDebugStreamLine("ENDED");
		}
		} else {
		writeDebugStreamLine("AUTON - RED");
		if (pidRunTimes == 0) {
			driveForwardToPosition(2, 2);
			wait1Msec(2000);
			motor[claw] = -65;
			wait1Msec(250);
			motor[claw] = 0;
			wait1Msec(1);
			driveForwardToPosition(-0.5, -0.5);
			} else if(pidTaskEnded && pidRunTimes == 1) {
			writeDebugStreamLine("COMMAND --- 1 --- FINISHED");
			driveToAngle(-90, true);
			} else if(pidTaskEnded && pidRunTimes == 2) {
			writeDebugStreamLine("COMMAND --- 2 --- FINISHED");
			string top = "TOP";
			driveToPositionWithArm(3, 3, top); //cannot input raw string value, must be assigned to a variable
			} else if(pidTaskEnded && pidRunTimes == 3) {
			writeDebugStreamLine("COMMAND --- 3 --- FINISHED");
			stopDrive();
			writeDebugStreamLine("ENDED");
		}
	}

}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Teleop                                                                     */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

task usercontrol() {

	while( true ) {
		motor[rightFront] = vexRT[Ch2];
		motor[backRight] = -vexRT[Ch2];
		motor[leftFront] = vexRT[Ch3];
		motor[backLeft] = -vexRT[Ch3];
		if (vexRT[Btn7U] == 1){
			startTask(autonomous);
			stopTask(usercontrol);
		}
		if (vexRT[Btn6U] == 1){
			motor[leftArm] = -120;
			motor[rightArm] = -120;
			motor[leftArm2] = -120;
			motor[rightArm2] = -120;
		}
		else if (vexRT[Btn6D] == 1){
			motor[leftArm] = 120;
			motor[rightArm] = 120;
			motor[leftArm2] = 120;
			motor[rightArm2] = 120;
		}
		else {
			motor[leftArm] = 0;
			motor[rightArm] = 0;
			motor[leftArm2] = 0;
			motor[rightArm2] = 0;
		}
		if(vexRT[Btn8L] == 1) { //left to one side, right to the other
			//turn the claw to a certain position, not holding
			motor[claw] = -25;
			} else if(vexRT[Btn8R] == 1) {
			motor[claw] = 25;
			} else {
			motor[claw] = 0;
		}

	}
}




/*****************************/
/*---------------------------*/
/*------ GYROSCOPE CODE -----*/
/*---------------------------*/
/*****************************/

//Source for help: http://www.robotc.net/blog/2011/10/13/programming-the-vex-gyro-in-robotc/






/*****************************/
/*---------------------------*/
/*- ULTRA SONIC SENSOR CODE -*/
/*---------------------------*/
/*****************************/







/*****************************/
/*---------------------------*/
/*-------- TEST CODE --------*/
/*---------------------------*/
/*****************************/


/*-- used for driveForwardToPosition method --*/
/*
while(true) {
if(encoderSide) {
currentDist = SensorValue(PID_SENSOR_INDEX_R);
} else if(!encoderSide) {
currentDist = SensorValue(PID_SENSOR_INDEX_L);
}
// Prints the encoder values
writeDebugStreamLine("Right Encoder: %d", SensorValue(PID_SENSOR_INDEX_R));
writeDebugStreamLine("Left Encoder: %d", SensorValue(PID_SENSOR_INDEX_L));
if(currentDist == desiredDist || currentDist >= desiredDist - 40) {
break;
}
wait1Msec(50);
}
*/

/*---------------------------------------------*/
