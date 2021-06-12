// KinectV1Process.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "KinectV1Handler.h"
#include <Windows.h>
#include "Networking.cpp"

// Logging Definitions
INITIALIZE_EASYLOGGINGPP

const char* logConfigFileName = "logging.conf";
const char* logConfigDefault =
"* GLOBAL:\n"
"	FORMAT = \"[%level] %datetime{%Y-%M-%d %H:%m:%s}: %msg\"\n"
"	FILENAME = \"K2VR.log\"\n"
"	ENABLED = true\n"
"	TO_FILE = true\n"
"	TO_STANDARD_OUTPUT = true\n"
"	MAX_LOG_FILE_SIZE = 2097152 ## 2MB\n"
"* TRACE:\n"
"	ENABLED = false\n"
"* DEBUG:\n"
"	ENABLED = false\n";

void init_logging()
{
	el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
	el::Configurations conf(logConfigFileName);
	conf.parseFromText(logConfigDefault);
	conf.parseFromFile(logConfigFileName);
	conf.setRemainingToDefault();
	el::Loggers::reconfigureAllLoggers(conf);
}

int main(int argc, char* argv[])
{
	START_EASYLOGGINGPP(argc, argv);
	init_logging();
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_SHOW); //Always show the terminal, it is the only input method for now
	KinectV1Handler kinect;
	KinectSettings::leftFootJointWithRotation = KVR::KinectJointType::AnkleLeft;
	KinectSettings::rightFootJointWithRotation = KVR::KinectJointType::AnkleRight;
	KinectSettings::leftFootJointWithoutRotation = KVR::KinectJointType::FootLeft;
	KinectSettings::rightFootJointWithoutRotation = KVR::KinectJointType::FootRight;

	if (kinect.isInitialised())
	{
		Networking networking;
		netLoop(networking, kinect);
	}
	else {
		printf("The Kinect has not been initialised. The program wille automatically close in 30 seconds.");
		Sleep(30000);
	}

	return 0;
}

/*
#ifdef _WIN32
// This disables the console window from appearing on windows only if the Project Settings->Linker->System->SubSystem is set to Windows (rather than Console).
int WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCMDShow)
{
	KinectV1Handler kinect;

	processLoop(kinect);

	return 0;
}
#endif
*/
