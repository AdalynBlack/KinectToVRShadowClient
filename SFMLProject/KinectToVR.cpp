#include "stdafx.h"
#include "KinectToVR.h"
#include "VRHelper.h"

#include "wtypes.h"
#include "Windows.h"

#include "KinectSettings.h"
#include "VRController.h"
#include "VRHelper.h"
#include "GamepadController.h"
#include "GUIHandler.h"
#include "ManualCalibrator.h"
#include "PlayspaceMovementAdjuster.h"
#include "ColorTracker.h"

#include <SFML\Audio.hpp>

#include <locale>
#include <codecvt>
#include <iostream>
#include <string>
//GUI
#include <SFGUI\SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

//OpenCV
#include <opencv2\opencv.hpp>



using namespace KVR;

void toEulerAngle(vr::HmdQuaternion_t q, double& roll, double& pitch, double& yaw)
{
    // roll (x-axis rotation)
    double sinr = +2.0 * (q.w * q.x + q.y * q.z);
    double cosr = +1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    roll = atan2(sinr, cosr);

    // pitch (y-axis rotation)
    double sinp = +2.0 * (q.w * q.y - q.z * q.x);
    if (fabs(sinp) >= 1)
        pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        pitch = asin(sinp);

    // yaw (z-axis rotation)
    double siny = +2.0 * (q.w * q.z + q.x * q.y);
    double cosy = +1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    yaw = atan2(siny, cosy);
}



void processKeyEvents(sf::Event event) {
    switch (event.key.code) {
    case sf::Keyboard::A:
        toggle(KinectSettings::isKinectDrawn);
        break;
    default:
        break;
    }
}
void toggle(bool &b) {
    b = !b;
}
// Get the horizontal and vertical screen sizes in pixel
//  https://stackoverflow.com/questions/8690619/how-to-get-screen-resolution-in-c
void getDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}
sf::VideoMode getScaledWindowResolution() {
    int h;
    int v;
    getDesktopResolution(h, v);
    
    sf::VideoMode mode = sf::VideoMode(SFMLsettings::windowScale*float(h), SFMLsettings::windowScale*float(v));
    //std::cerr << "desktop: " << h << ", " << v << '\n';
    //std::cerr << "scaled: " << mode.width << ", " << mode.height << '\n';
    return mode;
}
void updateKinectWindowRes(const sf::RenderWindow& window) {
    SFMLsettings::m_window_width = window.getSize().x;
    SFMLsettings::m_window_height = window.getSize().y;
    //std::cerr << "w: " << SFMLsettings::m_window_width << " h: " << SFMLsettings::m_window_height << "\n";
}

void updateFilePath() {
    HMODULE module = GetModuleHandleW(NULL);
    WCHAR exeFilePath[MAX_PATH];
    GetModuleFileNameW(module, exeFilePath, MAX_PATH);
     
    //Get rid of exe from name
    WCHAR directory[_MAX_PATH];
    WCHAR drive[_MAX_DRIVE];
    WCHAR dir[_MAX_DIR];
    WCHAR fname[_MAX_FNAME];
    WCHAR ext[_MAX_EXT];
    _wsplitpath_s(exeFilePath, drive, _MAX_DRIVE, dir, _MAX_DIR, fname,
        _MAX_FNAME, ext, _MAX_EXT);

    WCHAR filename[_MAX_FNAME]{};
    WCHAR extension[_MAX_EXT]{};
    WCHAR directoryFilePath[MAX_PATH];
    _wmakepath_s(directoryFilePath, _MAX_PATH, drive, dir, filename, extension);
    std::wstring string_to_convert(directoryFilePath);

    //setup converter
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    std::string converted_str = converter.to_bytes(string_to_convert);
    SFMLsettings::fileDirectoryPath = converted_str;
}

void processLoop(KinectHandlerBase& kinect) {
    updateFilePath();
    sf::RenderWindow renderWindow(getScaledWindowResolution(), "KinectToVR: " + KinectSettings::KVRversion, sf::Style::Titlebar | sf::Style::Close);
    updateKinectWindowRes(renderWindow);
    renderWindow.setFramerateLimit(30);   //Prevents ridiculous overupdating and high CPU usage - plus 90Hz is the recommended refresh rate for most VR panels 

    sf::Clock clock;

    //Initialise Settings
    KinectSettings::serializeKinectSettings();
    sf::Font font;
    sf::Text debugText;
    // Global Debug Font
#if _DEBUG
    std::cout << "Attemping Debug Font Load: " << KVR::fileToDirPath("arial.ttf") << '\n';
    font.loadFromFile(KVR::fileToDirPath("arial.ttf"));
    debugText.setFont(font);
#endif
    debugText.setString("");
    debugText.setCharacterSize(40);
    debugText.setFillColor(sf::Color::Red);

    debugText.setString(SFMLsettings::debugDisplayTextStream.str());

    //SFGUI Handling -------------------------------------- 
    GUIHandler guiRef;
    // ----------------------------------------------------

    //Initialise Kinect
    KinectSettings::kinectRepRotation = vrmath::quaternionFromYawPitchRoll(KinectSettings::kinectRadRotation.v[1], KinectSettings::kinectRadRotation.v[0], KinectSettings::kinectRadRotation.v[2]);
    kinect.update();

    guiRef.updateKinectStatusLabel(kinect);
    // Reconnect Kinect Event Signal
    guiRef.setKinectButtonSignal(kinect);

    //Initialise InputEmu and Trackers
    std::vector<KVR::KinectTrackedDevice> v_trackers{};
    vrinputemulator::VRInputEmulator inputEmulator;
    try {
        inputEmulator.connect();
    }
    catch (vrinputemulator::vrinputemulator_connectionerror e) {
        guiRef.updateEmuStatusLabelError(e);
        std::cerr << "Attempted connection to Input Emulator" << std::to_string(e.errorcode) + " " + e.what() + "\n\n Is SteamVR open and InputEmulator installed?" << std::endl;   
    }

    // Tracker Initialisation Lambda
    if (inputEmulator.isConnected()) {
        guiRef.setTrackerButtonSignals(inputEmulator, v_trackers);
        guiRef.updateEmuStatusLabelSuccess();
    }
    else {
        guiRef.updateTrackerInitButtonLabelFail();
    }

    //v_trackers.push_back(kinectTrackerRef);
    std::cerr << "Attempting connection to vrsystem.... " << std::endl;    // DEBUG
                                                                           //Initialise VR System
    VRcontroller rightController(vr::TrackedControllerRole_RightHand);
    VRcontroller leftController(vr::TrackedControllerRole_LeftHand);
    
    //GamepadController gamepad;

    vr::EVRInitError eError = vr::VRInitError_None;
    vr::IVRSystem *m_VRSystem = vr::VR_Init(&eError, vr::VRApplication_Utility);
    if (eError == vr::VRInitError_None) {
        std::cerr << "Attempting connection to controllers.... " << std::endl;    // DEBUG
        leftController.Connect(m_VRSystem);
        rightController.Connect(m_VRSystem);
        std::cerr << "Attempted connection to controllers! " << std::endl;    // DEBUG
    }
    guiRef.updateVRStatusLabel(eError);
    std::cerr << "Attempted connection to vrsystem! " << eError << std::endl;    // DEBUG

    guiRef.setReconnectControllerButtonSignal(leftController, rightController, m_VRSystem);

    KinectSettings::userChangingZero = true;
    PlayspaceMovementAdjuster playspaceMovementAdjuster(&inputEmulator);
    guiRef.setPlayspaceResetButtonSignal(playspaceMovementAdjuster);

    ColorTracker mainColorTracker(KinectSettings::kinectV2Width, KinectSettings::kinectV2Height);

    while (renderWindow.isOpen())
    {
        //Clear the debug text display
        SFMLsettings::debugDisplayTextStream.str(std::string());
        SFMLsettings::debugDisplayTextStream.clear();

        std::stringstream ss;
        double currentTime = clock.restart().asSeconds();
        double deltaT = currentTime;
        ss << "FPS = " << 1.0 / deltaT << '\n';

        updateKinectWindowRes(renderWindow);

        sf::Event event;
        while (renderWindow.pollEvent(event))
        {
            guiRef.desktopHandleEvents(event);

            if (event.type == sf::Event::Closed)
                renderWindow.close();
            if (event.type == sf::Event::KeyPressed) {
                processKeyEvents(event);
                //Debug Commands for testing
                if (event.key.code == sf::Keyboard::Q) {
                    kinect.initialiseColor();
                }
                if (event.key.code == sf::Keyboard::W) {
                    kinect.terminateColor();
                }

                if (event.key.code == sf::Keyboard::S) {
                    kinect.initialiseDepth();
                }
                if (event.key.code == sf::Keyboard::D) {
                    kinect.terminateDepth();
                }

                if (event.key.code == sf::Keyboard::X) {
                    kinect.initialiseSkeleton();
                }
                if (event.key.code == sf::Keyboard::C) {
                    kinect.terminateSkeleton();
                }
            }
        }

        //Clear ---------------------------------------
        renderWindow.clear();


        //Process -------------------------------------
        //Update GUI
        guiRef.updateDesktop(deltaT);
        
        //VR Controllers
        if (eError == vr::VRInitError_None) {
            rightController.update(deltaT);
            leftController.update(deltaT);
            updateHMDPosAndRot(m_VRSystem);
            /*
            if (rightController.isOutOfTrackingRange()) {
                SFMLsettings::debugDisplayTextStream << "RIGHT CONTROLLER POSE IS INVALID!\n";
            }
            */
        }
        else {
            std::cerr << "Error updating controllers: Could not connect to the SteamVR system! OpenVR init error-code " << std::to_string(eError) << std::endl;
        }

        
        // Update Kinect Status
        guiRef.updateKinectStatusLabel(kinect);
        if (kinect.isInitialised()) {
            kinect.update();
            if (KinectSettings::adjustingKinectRepresentationPos
                || KinectSettings::adjustingKinectRepresentationRot)
                ManualCalibrator::Calibrate(deltaT, leftController, rightController, guiRef);

            kinect.updateTrackersWithSkeletonPosition(inputEmulator, v_trackers);
            mainColorTracker.update(kinect.colorMat, kinect.depthMat);
            sf::Vector2i position = mainColorTracker.getTrackedPoints()[0];
            kinect.updateTrackersWithColorPosition(inputEmulator, v_trackers,position);
            //Draw
            kinect.drawKinectData(renderWindow);
        }
        std::vector<uint32_t> virtualDeviceIndexes;
        for (KinectTrackedDevice d : v_trackers) {
            vrinputemulator::VirtualDeviceInfo info = inputEmulator.getVirtualDeviceInfo(d.deviceId);
            virtualDeviceIndexes.push_back(info.openvrDeviceId); // needs to be converted into openvr's id - as inputEmulator has it's own Id's starting from zero
        }
        playspaceMovementAdjuster.update(leftController, rightController, virtualDeviceIndexes);
        
        renderWindow.pushGLStates();

        //Draw debug font
        debugText.setString(SFMLsettings::debugDisplayTextStream.str());
        renderWindow.draw(debugText);

        // Draw GUI
        renderWindow.setActive(true);

        guiRef.display(renderWindow);

        renderWindow.popGLStates();
        //End Frame
        renderWindow.display();

    }
    for (KinectTrackedDevice d : v_trackers) {
        d.destroy();
    }
    KinectSettings::writeKinectSettings();

    playspaceMovementAdjuster.resetPlayspaceAdjustments();

    vr::VR_Shutdown();
}

void spawnAndConnectHandTrackers(vrinputemulator::VRInputEmulator & inputE, std::vector<KinectTrackedDevice>& v_trackers) {
    spawnAndConnectTracker(inputE, v_trackers, KVR::KinectJointType::WristLeft, KVR::KinectJointType::HandLeft, KinectDeviceRole::LeftHand);
    spawnAndConnectTracker(inputE, v_trackers, KVR::KinectJointType::WristRight, KVR::KinectJointType::HandRight, KinectDeviceRole::RightHand);
}

void spawnAndConnectTracker(vrinputemulator::VRInputEmulator & inputE, std::vector<KinectTrackedDevice>& v_trackers, KVR::KinectJointType mainJoint, KVR::KinectJointType secondaryJoint, KinectDeviceRole role)
{
    KinectTrackedDevice device(inputE, mainJoint, secondaryJoint, role);
	device.init(inputE);
    v_trackers.push_back(device);
}

void spawnDefaultLowerBodyTrackers(vrinputemulator::VRInputEmulator & inputE, std::vector<KinectTrackedDevice>& v_trackers)
{
    spawnAndConnectTracker(inputE, v_trackers, KVR::KinectJointType::AnkleLeft, KVR::KinectJointType::FootLeft, KinectDeviceRole::LeftFoot);
    spawnAndConnectTracker(inputE, v_trackers, KVR::KinectJointType::AnkleRight, KVR::KinectJointType::FootRight, KinectDeviceRole::RightFoot);
    spawnAndConnectTracker(inputE, v_trackers, KVR::KinectJointType::SpineBase, KVR::KinectJointType::SpineMid, KinectDeviceRole::Hip);
}

void spawnAndConnectKinectTracker(vrinputemulator::VRInputEmulator &inputE, std::vector<KinectTrackedDevice> &v_trackers)
{
    KinectTrackedDevice kinectTrackerRef(inputE, KVR::KinectJointType::Head, KVR::KinectJointType::Head, KinectDeviceRole::KinectSensor);
	kinectTrackerRef.init(inputE);
    setKinectTrackerProperties(inputE, kinectTrackerRef.deviceId);
    v_trackers.push_back(kinectTrackerRef);
}

