#include "WPILib.h"
#include "Joystick.h"
#include "CameraServer.h"
#include <cmath>
#include <iostream>


//creates the robot class
class Robot: public SampleRobot {
	const int leftMotorPWM = 5;
	const int rightMotorPWM = 4;
	const int windshieldWasherPWM = 6;
	const int intakeMotorPWM = 7;
	const int liftMotorPWM = 3;
	const int limitSwitchDIO = 0;
	double leftValue = 0.0;
	double rightValue = 0.0;

	VictorSP leftMotor;
	VictorSP rightMotor;
	VictorSP windshieldWasher;
	VictorSP intakeMotor;
	VictorSP liftMotor;

	DigitalInput limitSwitch;
	DoubleSolenoid launchL;
	DoubleSolenoid launchR;
	Compressor compressor;
	RobotDrive myDrive;	//line is suddenly causing errors when lift motor is added, can't tell why

	//Can't find these axis numbers, they match with the driving station...
	int leftYaxis = 1;
	int rightYaxis = 3;

	//Logitech Buttons
	int yButton_logitech = 4;
	int aButton_logitech = 2;
	int xButton_logitech = 1;
	int bButton_logitech = 3;
	int leftTrigger_logitech = 7;
	int rightTrigger_logitech = 8;
	int leftBumper_logitech = 5;
	int rightBumper_logitech = 6;

	//Xbox Buttons
	int yButton_xbox = 4;
	int aButton_xbox = 1;
	int xButton_xbox = 3;
	int bButton_xbox = 2;
	int leftTrigger_xbox = 2;
	int rightTrigger_xbox = 3;
	int leftBumper_xbox = 5;
	int rightBumper_xbox = 6;

	//creates a robot drive and pointers to the two controllers
	RobotDrive* driver = new RobotDrive(&leftMotor, &rightMotor);
	Joystick* logitechController = new Joystick(0);
	Joystick* xboxController = new Joystick(1);

	//misc. variables
	double kupdateperiod = 0.05;
	bool launchEnable = false;


public:
	//initializes the robot class' parameters and functions
	Robot() :
			leftMotor(leftMotorPWM),
			rightMotor(rightMotorPWM),
			windshieldWasher(windshieldWasherPWM),
			intakeMotor(intakeMotorPWM),
			liftMotor(liftMotorPWM),
			limitSwitch(limitSwitchDIO),
			launchL(0,1),
			launchR(2,3),
			compressor(0),
			myDrive(&leftMotor, &rightMotor)			//line is suddenly causing errors when lift motor is added, can't tell why
	{
		//chassis.SetExpiration(0.1);
	}


	//custom functions

	//exactly what it says on the tin
	void driveforward(double power, int time)
	{
		leftMotor.Set(-power);
		rightMotor.Set(-power);
		Wait(time);
		leftMotor.Set(0);
		rightMotor.Set(0);
	}
	//also what it says on the tin, turns
	void turn(char direction, double power, int time){
		//l stands for left, r stands for right
		if(direction == 'l' || direction == 'L'){
			leftMotor.Set(-power);
			rightMotor.Set(power);
			Wait(time);
			leftMotor.Set(0);
			rightMotor.Set(0);
		}
		else if(direction == 'r' || direction == 'R'){
			leftMotor.Set(power);
			rightMotor.Set(-power);
			Wait(time);
			leftMotor.Set(0);
			rightMotor.Set(0);
		}
	}

	bool windowLock = false;

	void handleWindshield()
	{
		//Add values together, so if both buttons are pressed nothing happens
		int value = 0;

		if(limitSwitch.Get() == 0)
		{
			windowLock = true;
		}

		//Move up if Y button is pressed
		if (xboxController->GetRawButton(yButton_xbox))
		{
			value -= 1; //up
			windowLock = false;
		}

		//Move down if X button is pressed
		if (xboxController->GetRawButton(aButton_xbox) && windowLock == false)
		{
			value +=1; //down
		}

		windshieldWasher.Set(value);
	}

	//Controls the intake spinner, variable speed based on how far the triggers have been pulled
	//subtracts the value of both triggers so that if both are pulled it does not move
	void handleIntake()
	{
		double speed = 0.0;

		speed = xboxController->GetRawAxis(rightTrigger_xbox) - xboxController->GetRawAxis(leftTrigger_xbox);
		//remnant from when we didn't know the logitech triggers were buttons and not axes
		//speed = logitechController->GetRawAxis(rightTrigger_logitech) - logitechController->GetRawAxis(leftTrigger_logitech);

		intakeMotor.Set(speed);
	}

	//Driving controls, creates a divider to reduce the speed of the robot to manageable levels for regular driving
	//Logitech triggers change the divider to increase or decrease speed in different situations
	void handleDriving()
	{
		int divider = 2;

		leftValue = logitechController->GetRawAxis(leftYaxis);
		rightValue = logitechController->GetRawAxis(rightYaxis);

		if (logitechController->GetRawButton(leftTrigger_logitech))
		{
			divider = 3;
		}
		if (logitechController->GetRawButton(rightTrigger_logitech))
		{
			divider = 1;
		}

		if(fabs(logitechController->GetRawAxis(leftYaxis)) < 0.2)
		{
			leftValue = 0.0;
		}
		//Scott's code, I'm not quite sure what it's meant to do, ask him
/*		else
*		{
*			if(leftBumper_logitech){
*				leftValue = -1 * logitechController->GetRawAxis(leftYaxis);
*			}
*			leftValue = logitechController->GetRawAxis(leftYaxis);
*		}
*/
		if(fabs(logitechController->GetRawAxis(rightYaxis)) < 0.2)
		{
			rightValue = 0.0;
		}
		//Also Scott's code, also not too sure what it was supposed to do
/*		else
*		{
*			if(leftBumper_logitech){
*				rightValue = -1 * logitechController->GetRawAxis(rightYaxis);
*			}
*			rightValue = logitechController->GetRawAxis(rightYaxis);
*		}
*/
		driver->TankDrive(-leftValue/divider, -rightValue/divider);

	}

	//function to simplify launching code
	void launchSet(DoubleSolenoid::Value val)
	{
		launchL.Set(val);
		launchR.Set(val);
	}

	//launches the boulders, uses Xbox controller right bumper with b button as a security measure
	//Releases pressure when b button is not pressed and the robot has launched
	void handleLaunching()
	{

		if (xboxController->GetRawButton(rightBumper_xbox) && xboxController->GetRawButton(bButton_xbox))
		{
			launchSet(DoubleSolenoid::kReverse);
			launchEnable = true;
		}
		else if ((!xboxController->GetRawButton(bButton_xbox) && launchEnable) || xboxController->GetRawButton(leftBumper_xbox))
		{
			launchSet(DoubleSolenoid::kForward);
			launchEnable = false;
		}

	}

	//Lifts the robot off the ground, uses Logitech controller y and a
	void handleLift()
	{
		if(logitechController->GetRawButton(yButton_logitech))
		{
			liftMotor.Set(0.3);
		}
		else if(logitechController->GetRawButton(aButton_logitech))
		{
			liftMotor.Set(-0.3);
		}
		else
		{
			liftMotor.Set(0);
		}
	}

//Autonomous
	int autoProgramChooser(){
		int program = 0;
		SmartDashboard::PutNumber("auto", 0.0);
		program = (int)SmartDashboard::GetNumber("auto", 0.0);
		return program;
	}

	void autonomous(){
		int key = autoProgramChooser();
		switch(key){
			case 0:{
				break;
			}
			case 1:{
				driveforward(0.5, 1000);
				turn('l', 0.2, 500);
				Wait(500);
				break;
			}
			case 2:{
				driveforward(-0.5, 500);
				break;
			}
			case 3:{
				turn('l', 0.2, 500);
				break;
			}
			default:{
				Wait(15000);
			}
		}
	}


//SmartDashBoard Data
	void smartDashBoard(){
		SmartDashboard::PutNumber("leftMotor", leftMotor.Get());
		SmartDashboard::PutNumber("rightMotor", rightMotor.Get());

	}





//Robot Code
	void RobotInit() override
	{
		compressor.Start();
	}

	/**
	 * Drive left & right motors for 2 seconds then stop
	 */
	void Autonomous()
	{
		autonomous();
		smartDashBoard();
	}

	/**
	 * Runs the motors with arcade steering.
	 */
	void OperatorControl()
	{
		CameraServer::GetInstance()->StartAutomaticCapture();
		while(IsOperatorControl() && IsEnabled())
		{
			handleDriving();
			handleWindshield();
			handleIntake();
			handleLaunching();
			handleLift();
			smartDashBoard();
		}
	}
	};

START_ROBOT_CLASS(Robot);
