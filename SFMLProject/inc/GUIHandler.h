#pragma once

#include "VRController.h"

#include "KinectSettings.h"
#include "KinectHandlerBase.h"
#include "KinectTrackedDevice.h"
#include "KinectJoint.h"
#include "ColorTracker.h"
#include "VRHelper.h"

#include "TrackingMethod.h"
#include "DeviceHandler.h"
#include "PSMoveHandler.h"
#include "VRDeviceHandler.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/OpenGL.hpp>
//GUI
#include <SFGUI\SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <string>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/access.hpp>

struct TempTrackerData {
    TempTrackerData() {}
    uint32_t positionGlobalDeviceId = 0;
    std::string posDeviceName = "INVALID";
    std::string posDeviceSerial = "INVALID";

    uint32_t rotationGlobalDeviceId = 0;
    std::string rotDeviceName = "INVALID";
    std::string rotDeviceSerial = "INVALID";

    KVR::KinectDeviceRole role = KVR::KinectDeviceRole::Unassigned;
    bool isController = false;


    friend class cereal::access;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(
            CEREAL_NVP(positionGlobalDeviceId),
            CEREAL_NVP(posDeviceName),
            CEREAL_NVP(posDeviceSerial),
            CEREAL_NVP(positionGlobalDeviceId),
            CEREAL_NVP(rotDeviceName),
            CEREAL_NVP(rotDeviceSerial),
            CEREAL_NVP(role),
            CEREAL_NVP(isController)
        );
    }
};
struct TempTracker {
    sfg::RadioButton::Ptr radioButton = sfg::RadioButton::Create("");
    int GUID = 404;

    KVR::JointPositionTrackingOption positionTrackingOption = KVR::JointPositionTrackingOption::Skeleton;


    KVR::JointRotationTrackingOption rotationTrackingOption = KVR::JointRotationTrackingOption::Skeleton;

    TempTrackerData data;
};
struct serialTest {
    int butt;
    template<class Archive>
    void serialize(Archive & archive,
        serialTest & t)
    {
        archive(butt);
    }
};

class GUIHandler {
private:

public:
    
    GUIHandler() {
        guiWindow->SetTitle("Main Window");

        setDefaultSignals();
        
        setLineWrapping();
        packElementsIntoMainBox();
        packElementsIntoAdvTrackerBox();
        packElementsIntoTrackingMethodBox();
        packElementsIntoCalibrationBox();
        packElementsIntoVirtualHipsBox();
        setRequisitions();

        mainNotebook->AppendPage(mainGUIBox, sfg::Label::Create("KinectToVR"));
        mainNotebook->AppendPage(advancedTrackerBox, sfg::Label::Create("Adv. Trackers"));
        mainNotebook->AppendPage(calibrationBox, sfg::Label::Create("Calibration"));
        mainNotebook->AppendPage(trackingMethodBox, sfg::Label::Create("Tracking Method"));
        mainNotebook->AppendPage(virtualHipsBox, sfg::Label::Create("Virtual Hips"));
        
        

        guiWindow->Add(mainNotebook);
        guiDesktop.Add(guiWindow);


        setScale();

        bool b = guiDesktop.LoadThemeFromFile("main_theme.theme");
        
        guiDesktop.Update(0.f);
    }
    ~GUIHandler() {}

    void display(sf::RenderWindow &window) {
        sfguiRef.Display(window);
    }

void desktopHandleEvents(sf::Event event) {
    guiDesktop.HandleEvent(event);
}
void updateDesktop(float d) {
    guiDesktop.Update(d);
}
void setRequisitions() {
    CalibrationEntryPosX->SetRequisition(sf::Vector2f(40.f, 0.f));
    CalibrationEntryPosY->SetRequisition(sf::Vector2f(40.f, 0.f));
    CalibrationEntryPosZ->SetRequisition(sf::Vector2f(40.f, 0.f));
    CalibrationEntryRotX->SetRequisition(sf::Vector2f(40.f, 0.f));
    CalibrationEntryRotY->SetRequisition(sf::Vector2f(40.f, 0.f));
    CalibrationEntryRotZ->SetRequisition(sf::Vector2f(40.f, 0.f));
}
void setScale() {
    guiWindow->SetAllocation(sf::FloatRect(0.f, 0.f, .4f*SFMLsettings::m_window_width, .4f*SFMLsettings::m_window_height));
    guiWindow->SetRequisition(sf::Vector2f(.2f*SFMLsettings::m_window_width, .2f*SFMLsettings::m_window_height));
    //Text scaling
    /*
    Window > * > * > Label{
        FontSize : 18;
    /*FontName: data/linden_hill.otf;*/
    /*
    float defaultFontSize = 10.f / 1920.f; // Percentage relative to 1080p
    float scaledFontSize = defaultFontSize * (SFMLsettings::m_window_width / SFMLsettings::windowScale);
    */
    float scaledFontSize = SFMLsettings::globalFontSize;
    guiDesktop.SetProperty("Window Label, Box, Button, Notebook, CheckButton, ToggleButton, Label, RadioButton, ComboBox, SpinButton", "FontSize", scaledFontSize);
}
void toggleRotButton() {
    KinectRotButton->SetActive(KinectSettings::adjustingKinectRepresentationRot);
}
void togglePosButton() {
    KinectPosButton->SetActive(KinectSettings::adjustingKinectRepresentationPos);
}

bool trackerConfigExists() {
    // NOTE: Does not necessarily mean that it is valid
    std::ifstream is(KVR::fileToDirPath(KVR::trackerConfig));
    return !is.fail();
}
void saveLastSpawnedTrackers(std::vector<TempTracker> v_trackers)
{
    std::vector<TempTrackerData> v_trackerData;
    for (TempTracker & t : v_trackers) {
        v_trackerData.push_back(t.data);
    }
    std::ofstream os(KVR::fileToDirPath(KVR::trackerConfig));
    if (os.fail()) {
        //FAIL!!!
        LOG(ERROR) << "ERROR: COULD NOT WRITE TO TRACKER CONFIG FILE\n";
    }
    else {
        cereal::JSONOutputArchive archive(os);
        LOG(INFO) << "Attempted to save last tracker settings to file";
        try {
            archive(
                CEREAL_NVP(v_trackerData)
            );
        }
        catch (cereal::RapidJSONException e) {
            LOG(ERROR) << "CONFIG FILE SAVE JSON ERROR: " << e.what();
        }

    }
}

bool retrieveLastSpawnedTrackers()
{
    std::ifstream is(KVR::fileToDirPath(KVR::trackerConfig));

    LOG(INFO) << "Attempted to load last set of spawned trackers at " << KVR::fileToDirPath(KVR::trackerConfig);

    std::vector<TempTrackerData> v_trackerData;
    //CHECK IF VALID
    if (is.fail()) {
        error_trackerCfgNotFound(KVR::trackerConfig);
        return false;
    }
    else {
        LOG(INFO) << KVR::trackerConfig << " load attempted!";
        try {
            cereal::JSONInputArchive archive(is);
            archive(CEREAL_NVP(v_trackerData));
        }
        catch (cereal::Exception e) {
            LOG(ERROR) << KVR::trackerConfig << "TRACKER FILE LOAD JSON ERROR: " << e.what();
        }
    }
    if (v_trackerData.size() == 0) {
        error_trackerCfgEmpty(KVR::trackerConfig);
        return false;
    }
    for (TempTrackerData & data : v_trackerData) {
        if (!addUserTrackerToList(data)) {
            error_lastTrackersRespawnFailure();
            return false;
        }
    }
    return true;
}

void error_lastTrackersRespawnFailure()
{
    LOG(ERROR) << "Attempted to respawn trackers, but at least one was invalid!";
    auto message = L"ERROR: INVALID TRACKERS DETECTED IN CFG: "
        + SFMLsettings::fileDirectoryPath
        + L"\n No trackers will be spawned"
        + L"\n Refer to K2VR.log, to see what went wrong";
    auto result = MessageBox(NULL, message.c_str(), L"ERROR!!!", MB_OK + MB_ICONWARNING);
}

void error_trackerCfgNotFound(std::wstring &trackerConfig)
{
    //FAIL!!!!
    LOG(ERROR) << "ERROR: COULD NOT OPEN " << trackerConfig << " FILE";
    // Does not need to create config file - as the vector is empty anyway
    LOG(ERROR) << "ERROR: Can't find last used custom tracker file!";
    auto message = L"WARNING: NO lastTrackers.cfg DETECTED: "
        + SFMLsettings::fileDirectoryPath
        + L"\n No trackers will be added to the menu";
    auto result = MessageBox(NULL, message.c_str(), L"WARNING!!!", MB_OK + MB_ICONWARNING);
}

void error_trackerCfgEmpty(std::wstring &trackerConfig)
{
    LOG(ERROR) << "WARNING: " << trackerConfig << " FILE IS EMPTY OF TRACKERS";

    auto message = L"WARNING: No trackers were found in file at all! "
        + SFMLsettings::fileDirectoryPath
        + L"\n No trackers will be added to the menu";
    auto result = MessageBox(NULL, message.c_str(), L"WARNING!!!", MB_OK + MB_ICONWARNING);
}
void connectPSMoveHandlerGUIEvents() {
    if (psMoveHandler.active) {
        PSMoveHandlerLabel->SetText("Status: Connected!");
    }
    else {
        PSMoveHandlerLabel->SetText("Status: Disconnected!");
    }

    StartPSMoveHandler->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        initialisePSMoveHandlerIntoGUI();
    });
    StopPSMoveHandler->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        if (!psMoveHandler.active)
            return;
        psMoveHandler.shutdown();
        updateDeviceLists();
        PSMoveHandlerLabel->SetText("Status: Disconnected!");
    });
}

void initialisePSMoveHandlerIntoGUI()
{
    if (psMoveHandler.active) {
        LOG(INFO) << "Tried to initialise PSMoveHandler in the GUI, but it was already active";
        return;
    }

    auto errorCode = psMoveHandler.initialise();

    if (psMoveHandler.active) {
        static bool addedToVector = false;
        if (!addedToVector) {
            v_deviceHandlersRef->push_back(std::make_unique<PSMoveHandler>(psMoveHandler));
            addedToVector = true;
            PSMoveHandlerLabel->SetText("Status: Connected!");
        }
        updateDeviceLists();
    }
    else {
        PSMoveHandlerLabel->SetText(psMoveHandler.connectionMessages[errorCode]);
    }
}

void setDefaultSignals() {
    //Post VR Tracker Initialisation
    hidePostTrackerInitUI();

    {
        // Font Size Scaling
        FontSizeScale->SetDigits(3);
        FontSizeScale->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
            // This checking is required due to some weird anomaly in Sfgui.
            // Without it, it will constantly reupdate the SpinButton event,
            // effectively lagging this for 2-10x as long as it should
            lastFontSizeValue = SFMLsettings::globalFontSize;
            SFMLsettings::globalFontSize = FontSizeScale->GetValue();
            if (lastFontSizeValue != SFMLsettings::globalFontSize) {
                setScale();
            }
        });
    }
    EnableGamepadButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        if (EnableGamepadButton->IsActive()) {
            SFMLsettings::usingGamepad = true;
        }
        else {
            SFMLsettings::usingGamepad = false;
        }
    });

	ShowSkeletonButton->GetSignal(sfg::Widget::OnLeftClick).Connect([] {
		KinectSettings::isSkeletonDrawn = !KinectSettings::isSkeletonDrawn;
	});

    KinectRotButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        if (KinectRotButton->IsActive()) {
            KinectSettings::adjustingKinectRepresentationRot = true;
        }
        else
            KinectSettings::adjustingKinectRepresentationRot = false;
    });
    KinectPosButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this]
    {    if (KinectPosButton->IsActive()) {
        KinectSettings::adjustingKinectRepresentationPos = true;
    }
    else
        KinectSettings::adjustingKinectRepresentationPos = false;
    });
    IgnoreInferredCheckButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        if (IgnoreInferredCheckButton->IsActive()) {
            KinectSettings::ignoreInferredPositions = true;    // No longer stops updating trackers when Kinect isn't sure about a position
        }
        else {
            KinectSettings::ignoreInferredPositions = false;
        }
    });
    
    showJointDevicesButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        kinectJointDevicesHiddenFromList = !showJointDevicesButton->IsActive();
        updateDeviceLists();
    });
    refreshDeviceListButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        updateDeviceLists();
    });
    identifyPosDeviceButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        if (PositionDeviceList->GetItemCount()) {
            auto globalIndex = selectedPositionDeviceIndex();
            auto d = TrackingPoolManager::getDeviceData(globalIndex);
            if (d.parentHandler) {
                d.parentHandler->identify(globalIndex, identifyPosDeviceButton->IsActive());
            }
            else LOG(ERROR) << "Attempted to identify a device with no valid parent handler bound";
        }
    });
    identifyRotDeviceButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        if (RotationDeviceList->GetItemCount()) {
            auto globalIndex = selectedRotationDeviceIndex();
            auto d = TrackingPoolManager::getDeviceData(globalIndex);

            if (d.parentHandler) {
                d.parentHandler->identify(globalIndex, identifyRotDeviceButton->IsActive());
            }
            else LOG(ERROR) << "Attempted to identify a device with no valid parent handler bound";
        }
    });

    PositionDeviceList->GetSignal(sfg::ComboBox::OnSelect).Connect([this] {
        // QOL Change to make selecting trackers easier
        if (userSelectedDeviceRotIndex) {
            // Don't auto change
        }
        else {
            RotationDeviceList->SelectItem(PositionDeviceList->GetSelectedItem());
            userSelectedDeviceRotIndex = false;
            userSelectedDevicePosIndex = true;
        }
    });
    RotationDeviceList->GetSignal(sfg::ComboBox::OnSelect).Connect([this] {
        // QOL Change to make selecting trackers easier
        if (userSelectedDevicePosIndex) {
            // Don't auto change
        }
        else {
            PositionDeviceList->SelectItem(RotationDeviceList->GetSelectedItem());
            userSelectedDevicePosIndex = false;
            userSelectedDeviceRotIndex = true;
        }
    });


    AddHandControllersToList->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        //Add a left and right hand tracker as a controller
        addTrackerToList(KVR::KinectJointType::HandLeft, KVR::KinectDeviceRole::LeftHand, true);
        addTrackerToList(KVR::KinectJointType::HandRight, KVR::KinectDeviceRole::RightHand, true);
    });
    AddLowerTrackersToList->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        addTrackerToList(KVR::KinectJointType::AnkleLeft, KVR::KinectDeviceRole::LeftFoot, false);
        addTrackerToList(KVR::KinectJointType::AnkleRight, KVR::KinectDeviceRole::RightFoot, false);
        addTrackerToList(KVR::KinectJointType::SpineBase, KVR::KinectDeviceRole::Hip, false);
    });
    AddTrackerToListButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        addUserTrackerToList();
        userSelectedDeviceRotIndex = false;
        userSelectedDevicePosIndex = false;
    });
    RemoveTrackerFromListButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this] {
        int i = 0;
        for (; i < TrackersToBeInitialised.size(); ++i) {
            if (TrackersToBeInitialised[i].radioButton->IsActive()) {
                TrackersToBeInitialised[i].radioButton->Show(false);
                TrackersToBeInitialised.erase(TrackersToBeInitialised.begin() + i);
                break;
            }
        }
        //updateTempTrackerIDs();
        //updateTempTrackerButtonGroups();
    });

    connectPSMoveHandlerGUIEvents();

    HipScale->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        // Update the Global hip offset
        KinectSettings::hipRoleHeightAdjust = HipScale->GetValue();
    }
    );
    setCalibrationSignal();

    
}
void setColorTrackerSignals(ColorTracker & colorTracker) {
    InitiateColorTrackingButton->GetSignal(sfg::Button::OnMouseLeftPress).Connect([this, &colorTracker] {
        colorTracker.initialise();
    });
    DestroyColorTrackingButton->GetSignal(sfg::Button::OnMouseLeftPress).Connect([this, &colorTracker] {
        colorTracker.terminate();
    });
}
int selectedPositionDeviceIndex() {
    
    int posIndex = PositionDeviceList->GetSelectedItem();
    if (kinectJointDevicesHiddenFromList && TrackingPoolManager::trackerIdInKinectRange(posIndex))
        posIndex += KVR::KinectJointCount;
    // Really need to find a less hacky way to do this - as without it, when the kinect joints are hidden,
    // selecting a PSMove (ID of 25) would still use the kinect joint because it's technically the 0th item in the list
    return posIndex;
}
int selectedRotationDeviceIndex() {
    int rotIndex = RotationDeviceList->GetSelectedItem();
    if (kinectJointDevicesHiddenFromList && TrackingPoolManager::trackerIdInKinectRange(rotIndex))
        rotIndex += KVR::KinectJointCount;
    // Really need to find a less hacky way to do this - as without it, when the kinect joints are hidden,
    // selecting a PSMove (ID of 25) would still use the kinect joint because it's technically the 0th item in the list
    return rotIndex;
}
void addUserTrackerToList() {
    TempTracker temp;
    temp.GUID = TrackersToBeInitialised.size();
    temp.data.isController = IsControllerButton->IsActive();

    // Obtain Position Information
    int posIndex = selectedPositionDeviceIndex();
    if (posIndex < 0 || posIndex == k_invalidTrackerID) return;
    KVR::TrackedDeviceInputData posData = TrackingPoolManager::getDeviceData(posIndex);
    
    temp.data.positionGlobalDeviceId = posIndex;
    temp.positionTrackingOption = posData.positionTrackingOption;
    temp.data.posDeviceName = posData.deviceName;
    temp.data.posDeviceSerial = posData.serial;

    // Obtain Rotation Information
    int rotIndex = selectedRotationDeviceIndex();
    if (rotIndex < 0 || rotIndex == k_invalidTrackerID) return;
    KVR::TrackedDeviceInputData rotData = TrackingPoolManager::getDeviceData(rotIndex);
    
    temp.data.rotationGlobalDeviceId = rotIndex;
    temp.rotationTrackingOption = rotData.rotationTrackingOption;
    temp.data.rotDeviceName = rotData.deviceName;
    temp.data.rotDeviceSerial = rotData.serial;

    temp.data.role = KVR::KinectDeviceRole(RolesList->GetSelectedItem());
    updateTrackerLists(temp);
}
bool validatedTrackerData(TempTrackerData & data) {
    // Verify that data is correct

    // Index bound checks to prevent array access errors
    bool posMismatched = false;
    bool rotMismatched = false;
    if (data.positionGlobalDeviceId >= TrackingPoolManager::count()) {
        // INVALID POS ID
        LOG(WARNING) << "POSITION ID " << data.positionGlobalDeviceId << " GREATER THAN THE SIZE OF TRACKING POOL";
        posMismatched = true;
    }
    if (data.rotationGlobalDeviceId >= TrackingPoolManager::count()) {
        // INVALID ROT ID
        LOG(WARNING) << "ROTATION ID " << data.rotationGlobalDeviceId << " GREATER THAN THE SIZE OF TRACKING POOL";
        rotMismatched = true;
    }

    KVR::TrackedDeviceInputData posData = TrackingPoolManager::getDeviceData(data.positionGlobalDeviceId);
    KVR::TrackedDeviceInputData rotData = TrackingPoolManager::getDeviceData(data.rotationGlobalDeviceId);

    // Mismatched Device Index Checks
    if (data.posDeviceName != posData.deviceName) {
        // POTENTIALLY MISMATCHED DEVICE
        LOG(WARNING) << "POTENTIALLY MISMATCHED POS DEVICE NAME, EXPECTED " << data.posDeviceName << " AND RECEIVED " << posData.deviceName;

        // If serial is also wrong, panic
        if (data.posDeviceSerial != posData.serial) {
            LOG(ERROR) << "MISMATCHED POS DEVICE SERIAL, EXPECTED " << data.posDeviceSerial << " AND RECEIVED " << posData.serial;
            posMismatched = true;
        }
    }
    if (data.rotDeviceName != rotData.deviceName) {
        // POTENTIALLY MISMATCHED DEVICE
        LOG(WARNING) << "POTENTIALLY MISMATCHED ROT DEVICE NAME, EXPECTED " << data.rotDeviceName << " AND RECEIVED " << rotData.deviceName;

        // If serial is also wrong, panic
        if (data.rotDeviceSerial != rotData.serial) {
            LOG(ERROR) << "MISMATCHED ROT DEVICE SERIAL, EXPECTED " << data.rotDeviceSerial << " AND RECEIVED " << rotData.serial;
            rotMismatched = true;
        }
    }


    // If incorrect, search for device
    if (posMismatched) {
        LOG(INFO) << "Attempting to find Pos ID from device info...";
        uint32_t potentialNewID = TrackingPoolManager::locateGlobalDeviceID(data.posDeviceSerial);
        if (potentialNewID != k_invalidTrackerID) {
            LOG(INFO) << "Replacement Pos ID successfully found!";
            posMismatched = false;
            data.positionGlobalDeviceId = potentialNewID;
        }
        else {
            LOG(ERROR) << "Could not relocate pos device ID to spawn!";
        }
    }
    if (rotMismatched) {
        LOG(INFO) << "Attempting to find Rot ID from device info...";
        uint32_t potentialNewID = TrackingPoolManager::locateGlobalDeviceID(data.rotDeviceSerial);
        if (potentialNewID != k_invalidTrackerID) {
            LOG(INFO) << "Replacement Rot ID successfully found!";
            rotMismatched = false;
            data.rotationGlobalDeviceId = potentialNewID;
        }
        else {
            LOG(ERROR) << "Could not relocate rot device ID to spawn!";
        }
    }

    bool failed = rotMismatched || posMismatched;
    // If could not be found, produce warning to cancel
    if (failed) {
        return false;
    }
    return true;
}
bool addUserTrackerToList(TempTrackerData & data) {
    bool dataIsValid = validatedTrackerData(data);
    if (!dataIsValid)
        return false;

    TempTracker temp;
    temp.data = data;
    temp.GUID = TrackersToBeInitialised.size();

    KVR::TrackedDeviceInputData posData = TrackingPoolManager::getDeviceData(data.positionGlobalDeviceId);
    KVR::TrackedDeviceInputData rotData = TrackingPoolManager::getDeviceData(data.rotationGlobalDeviceId);

    temp.positionTrackingOption = posData.positionTrackingOption;
    temp.rotationTrackingOption = rotData.rotationTrackingOption;

    updateTrackerLists(temp);

    return true;
}
void addTrackerToList(KVR::KinectJointType joint, KVR::KinectDeviceRole role, bool isController) {
	TempTracker temp;
    temp.GUID = TrackersToBeInitialised.size();
    temp.data.isController = isController;
    // Obtain Position Information
    int posIndex = TrackingPoolManager::globalDeviceIDFromJoint(joint);
    if (posIndex < 0 || posIndex == k_invalidTrackerID) return;
    KVR::TrackedDeviceInputData posData = TrackingPoolManager::getDeviceData(posIndex);

    temp.data.positionGlobalDeviceId = posIndex;
    temp.positionTrackingOption = posData.positionTrackingOption;
    temp.data.posDeviceName = posData.deviceName;
    temp.data.posDeviceSerial = posData.serial;

    // Obtain Rotation Information
    int rotIndex = TrackingPoolManager::globalDeviceIDFromJoint(joint);
    if (rotIndex < 0 || rotIndex == k_invalidTrackerID) return;
    KVR::TrackedDeviceInputData rotData = TrackingPoolManager::getDeviceData(rotIndex);

    temp.data.rotationGlobalDeviceId = rotIndex;
    temp.rotationTrackingOption = rotData.rotationTrackingOption;
    temp.data.rotDeviceName = rotData.deviceName;
    temp.data.rotDeviceSerial = rotData.serial;
    temp.data.role = role;

    updateTrackerLists(temp);
}
void updateTrackerLists(TempTracker &temp) {
    // Display a radio button menu where selecting each button selects that tracker
    // Displays the joint of each tracker and (Tracker)/(Controller)
    std::stringstream roleStrStream;
    if (temp.data.isController)
        roleStrStream << " (Tracked Controller) ";
    else
        roleStrStream << " (Tracker) ";
    roleStrStream << "(Role: " << KVR::KinectDeviceRoleName[int(temp.data.role)] << ") ";
    std::string posName = TrackingPoolManager::deviceGuiString(temp.data.positionGlobalDeviceId);
    std::string rotName = TrackingPoolManager::deviceGuiString(temp.data.rotationGlobalDeviceId);
    std::string finalTrackerName = "Position: " + posName + " | Rotation: " + rotName + " | " + roleStrStream.str();

    LOG(INFO) << "Adding tracker to list :: " << finalTrackerName;

    temp.radioButton = sfg::RadioButton::Create(finalTrackerName);
    if (TrackersToBeInitialised.size()) {
        auto group = TrackersToBeInitialised.back().radioButton->GetGroup();
        temp.radioButton->SetGroup(group);
    }

    TrackerList->Pack(temp.radioButton);

    TrackersToBeInitialised.push_back(temp);
}
void refreshCalibrationMenuValues() {
    using namespace KinectSettings;
    CalibrationEntryPosX->SetValue(KinectSettings::kinectRepPosition.v[0]);
    CalibrationEntryPosY->SetValue(KinectSettings::kinectRepPosition.v[1]);
    CalibrationEntryPosZ->SetValue(KinectSettings::kinectRepPosition.v[2]);

    CalibrationEntryRotX->SetValue(KinectSettings::kinectRadRotation.v[0]);
    CalibrationEntryRotY->SetValue(KinectSettings::kinectRadRotation.v[1]);
    CalibrationEntryRotZ->SetValue(KinectSettings::kinectRadRotation.v[2]);

}
void setCalibrationSignal() {
    CalibrationEntryPosX->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        KinectSettings::kinectRepPosition.v[0] = CalibrationEntryPosX->GetValue();
        KinectSettings::sensorConfigChanged = true;
    }
    );
    CalibrationEntryPosY->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        KinectSettings::kinectRepPosition.v[1] = CalibrationEntryPosY->GetValue();
        KinectSettings::sensorConfigChanged = true;
    }
    );
    CalibrationEntryPosZ->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        KinectSettings::kinectRepPosition.v[2] = CalibrationEntryPosZ->GetValue();
        KinectSettings::sensorConfigChanged = true;
    }
    );

    CalibrationEntryRotX->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        KinectSettings::kinectRadRotation.v[0] = CalibrationEntryRotX->GetValue();
        KinectSettings::updateKinectQuaternion();
        KinectSettings::sensorConfigChanged = true;
    }
    );
    CalibrationEntryRotY->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        KinectSettings::kinectRadRotation.v[1] = CalibrationEntryRotY->GetValue();
        KinectSettings::updateKinectQuaternion();
        KinectSettings::sensorConfigChanged = true;
    }
    );
    CalibrationEntryRotZ->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        KinectSettings::kinectRadRotation.v[2] = CalibrationEntryRotZ->GetValue();
        KinectSettings::updateKinectQuaternion();
        KinectSettings::sensorConfigChanged = true;
    }
    );
    CalibrationSaveButton->GetSignal(sfg::Button::OnLeftClick).Connect([this] {
        KinectSettings::updateKinectQuaternion();
        KinectSettings::writeKinectSettings();
    }
    );
}
void loadK2VRIntoBindingsMenu(vr::IVRSystem * & m_VRSystem) {
    // Only scene apps currently actually load into the Bindings menu
    // So, this momentarily opens the vrsystem as a scene, and closes it
    // Which actually allows the menu to stay open, while still functioning as normal
    do {
        vr::EVRInitError eError = vr::VRInitError_None;
        vr::VR_Shutdown();
        LOG(INFO) << "(Workaround/Hack) Loading K2VR into bindings menu...";
        m_VRSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);
        Sleep(100); // Necessary because of SteamVR timing occasionally being too quick to change the scenes
        vr::VR_Shutdown();
        m_VRSystem = vr::VR_Init(&eError, vr::VRApplication_Background);
        LOG_IF(eError != vr::EVRInitError::VRInitError_None, ERROR) << " (Workaround/Hack) VR System failed to reinitialise, attempting again...";
    } while (m_VRSystem == nullptr); // Potential Segfault if not actually initialised and used later on
    LOG(INFO) << "(Workaround/Hack) Successfully loaded K2VR into bindings menu!";
}
void setVRSceneChangeButtonSignal(vr::IVRSystem * & m_VRSystem) {
    ActivateVRSceneTypeButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &m_VRSystem] {
        loadK2VRIntoBindingsMenu(m_VRSystem);
    });

}
void setKinectButtonSignal(KinectHandlerBase& kinect) {
    reconKinectButton->GetSignal(sfg::Widget::OnLeftClick).Connect([&kinect] {
        kinect.initialise();
    });
}
void spawnAndConnectTracker(vrinputemulator::VRInputEmulator & inputE, std::vector<KVR::KinectTrackedDevice>& v_trackers, TempTracker t_tracker)
{
    KVR::KinectTrackedDevice device(inputE, t_tracker.data.positionGlobalDeviceId, t_tracker.data.rotationGlobalDeviceId, t_tracker.data.role);
    device.positionTrackingOption = t_tracker.positionTrackingOption;
    device.rotationTrackingOption = t_tracker.rotationTrackingOption;
    device.customModelName = TrackingPoolManager::getDeviceData(t_tracker.data.positionGlobalDeviceId).customModelName;
    device.init(inputE);
    v_trackers.push_back(device);
}
void setTrackerButtonSignals(vrinputemulator::VRInputEmulator &inputE, std::vector<KVR::KinectTrackedDevice> &v_trackers, vr::IVRSystem * & m_VRSystem) {
    calibrateOffsetButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this, & inputE, & v_trackers, & m_VRSystem]{
        // WARNING, SUPER HACKY!!!
        // Spawn device which sets it's vec position to 0
        vr::DriverPose_t pose = defaultReadyDriverPose();

        pose.vecPosition[0] = 0;
        pose.vecPosition[1] = 0;
        pose.vecPosition[2] = 0;
        pose.vecWorldFromDriverTranslation[0] -= KinectSettings::trackingOriginPosition.v[0];
        pose.vecWorldFromDriverTranslation[1] -= KinectSettings::trackingOriginPosition.v[1];
        pose.vecWorldFromDriverTranslation[2] -= KinectSettings::trackingOriginPosition.v[2];
        v_trackers[0].nextUpdatePoseIsSet = false;
        v_trackers[0].setPoseForNextUpdate(pose, true);
            // Get it's VR ID
        auto info = inputE.getVirtualDeviceInfo(0);
        uint32_t vrID = info.openvrDeviceId;
        // Get it's absolute tracking, set the secondary offset to that

        v_trackers[0].update(pose);

        vr::TrackedDevicePose_t devicePose[vr::k_unMaxTrackedDeviceCount];
        m_VRSystem->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, 0, devicePose, vr::k_unMaxTrackedDeviceCount);

        KinectSettings::secondaryTrackingOriginOffset = GetVRPositionFromMatrix(devicePose[vrID].mDeviceToAbsoluteTracking);
        LOG(INFO) << "SET THE SECONDARY OFFSET TO " << KinectSettings::secondaryTrackingOriginOffset.v[0] << ", " << KinectSettings::secondaryTrackingOriginOffset.v[1] << ", " << KinectSettings::secondaryTrackingOriginOffset.v[2];

});
    TrackerInitButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers, &inputE] {
        /*
        bool reuseLastTrackers = true;
        if (reuseLastTrackers) {
            LOG(INFO) << "SPAWNING TRACKERS FROM LAST OPEN, MAY BE ISSUES";

            // Load Last Set of trackers used

            // Spawn
        }
        */
        TrackerInitButton->SetLabel("Trackers Initialised");
        if (TrackersToBeInitialised.empty()) {
            spawnDefaultLowerBodyTrackers(inputE, v_trackers);
            spawnAndConnectKinectTracker(inputE, v_trackers);
        }
        else {
            TrackingPoolManager::leftFootDevicePosGID = k_invalidTrackerID;
            TrackingPoolManager::rightFootDevicePosGID = k_invalidTrackerID;
            TrackingPoolManager::leftFootDeviceRotGID = k_invalidTrackerID;
            TrackingPoolManager::rightFootDeviceRotGID = k_invalidTrackerID;

            for (TempTracker tracker : TrackersToBeInitialised) {
                spawnAndConnectTracker(inputE, v_trackers, tracker);

                if (tracker.data.role == KVR::KinectDeviceRole::LeftFoot) {
                    TrackingPoolManager::leftFootDevicePosGID = tracker.data.positionGlobalDeviceId;
                    TrackingPoolManager::leftFootDeviceRotGID = tracker.data.rotationGlobalDeviceId;
                }
                if (tracker.data.role == KVR::KinectDeviceRole::RightFoot) {
                    TrackingPoolManager::rightFootDevicePosGID = tracker.data.positionGlobalDeviceId;
                    TrackingPoolManager::rightFootDeviceRotGID = tracker.data.rotationGlobalDeviceId;
                }

                if (tracker.data.isController) {
                    setDeviceProperty(inputE, v_trackers.back().deviceId, vr::Prop_DeviceClass_Int32, "int32", "2"); // Device Class: Controller
                    if (tracker.data.role == KVR::KinectDeviceRole::LeftHand) {
                        setDeviceProperty(inputE, v_trackers.back().deviceId, vr::Prop_ControllerRoleHint_Int32, "int32", "1"); // ControllerRole Left
                    }
                    else if (tracker.data.role == KVR::KinectDeviceRole::RightHand) {
                        setDeviceProperty(inputE, v_trackers.back().deviceId, vr::Prop_ControllerRoleHint_Int32, "int32", "2"); // ControllerRole Right
                    }
                }
            }
            saveLastSpawnedTrackers(TrackersToBeInitialised);
        }

        showPostTrackerInitUI();

        TrackerInitButton->SetState(sfg::Widget::State::INSENSITIVE);
        TrackerLastInitButton->SetState(sfg::Widget::State::INSENSITIVE);
    });
    // Make sure that users don't get confused and hit the spawn last button when they don't need it
    bool foundCachedTrackers = trackerConfigExists() ? true : false;
    TrackerLastInitButton->Show(foundCachedTrackers);

    TrackerLastInitButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers, &inputE] {
        if (!retrieveLastSpawnedTrackers()) {
            return; // Don't actually spawn the trackers, as they will likely crash
        }
        TrackerLastInitButton->SetLabel("Trackers Initialised");
        if (TrackersToBeInitialised.empty()) {
            spawnDefaultLowerBodyTrackers(inputE, v_trackers);
            spawnAndConnectKinectTracker(inputE, v_trackers);
        }
        else {
            TrackingPoolManager::leftFootDevicePosGID = k_invalidTrackerID;
            TrackingPoolManager::rightFootDevicePosGID = k_invalidTrackerID;
            TrackingPoolManager::leftFootDeviceRotGID = k_invalidTrackerID;
            TrackingPoolManager::rightFootDeviceRotGID = k_invalidTrackerID;

            for (TempTracker tracker : TrackersToBeInitialised) {
                spawnAndConnectTracker(inputE, v_trackers, tracker);

                if (tracker.data.role == KVR::KinectDeviceRole::LeftFoot) {
                    TrackingPoolManager::leftFootDevicePosGID = tracker.data.positionGlobalDeviceId;
                    TrackingPoolManager::leftFootDeviceRotGID = tracker.data.rotationGlobalDeviceId;
                }
                if (tracker.data.role == KVR::KinectDeviceRole::RightFoot) {
                    TrackingPoolManager::rightFootDevicePosGID = tracker.data.positionGlobalDeviceId;
                    TrackingPoolManager::rightFootDeviceRotGID = tracker.data.rotationGlobalDeviceId;
                }

                if (tracker.data.isController) {
                    setDeviceProperty(inputE, v_trackers.back().deviceId, vr::Prop_DeviceClass_Int32, "int32", "2"); // Device Class: Controller
                    if (tracker.data.role == KVR::KinectDeviceRole::LeftHand) {
                        setDeviceProperty(inputE, v_trackers.back().deviceId, vr::Prop_ControllerRoleHint_Int32, "int32", "1"); // ControllerRole Left
                    }
                    else if (tracker.data.role == KVR::KinectDeviceRole::RightHand) {
                        setDeviceProperty(inputE, v_trackers.back().deviceId, vr::Prop_ControllerRoleHint_Int32, "int32", "2"); // ControllerRole Right
                    }
                }
            }
            saveLastSpawnedTrackers(TrackersToBeInitialised);
        }

        showPostTrackerInitUI();

        TrackerInitButton->SetState(sfg::Widget::State::INSENSITIVE);
        TrackerLastInitButton->SetState(sfg::Widget::State::INSENSITIVE);
    });

    SetJointsToFootRotationButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers] {
        using namespace KinectSettings;
        leftFootJointWithRotation = KVR::KinectJointType::FootLeft;
        rightFootJointWithRotation = KVR::KinectJointType::FootRight;
        leftFootJointWithoutRotation = KVR::KinectJointType::AnkleLeft;
        rightFootJointWithoutRotation = KVR::KinectJointType::AnkleRight;
        for (KVR::KinectTrackedDevice &d : v_trackers) {
            if (d.role == KVR::KinectDeviceRole::LeftFoot) {
                d.joint0 = leftFootJointWithRotation;
                d.joint1 = leftFootJointWithoutRotation;
            }
            if (d.role == KVR::KinectDeviceRole::RightFoot) {
                d.joint0 = rightFootJointWithRotation;
                d.joint1 = rightFootJointWithoutRotation;
            }
        }
    });
    SetJointsToAnkleRotationButton->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers] {
        using namespace KinectSettings;
        leftFootJointWithRotation = KVR::KinectJointType::AnkleLeft;
        rightFootJointWithRotation = KVR::KinectJointType::AnkleRight;
        leftFootJointWithoutRotation = KVR::KinectJointType::FootLeft;
        rightFootJointWithoutRotation = KVR::KinectJointType::FootRight;
        for (KVR::KinectTrackedDevice &d : v_trackers) {
            if (d.role == KVR::KinectDeviceRole::LeftFoot) {
                d.joint0 = leftFootJointWithRotation;
                d.joint1 = leftFootJointWithoutRotation;
            }
            if (d.role == KVR::KinectDeviceRole::RightFoot) {
                d.joint0 = rightFootJointWithRotation;
                d.joint1 = rightFootJointWithoutRotation;
            }
        }
    });

    SetAllJointsRotUnfiltered->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers] {
        for (KVR::KinectTrackedDevice &d : v_trackers) {
            if (d.isSensor()){}
            else {
                d.rotationFilterOption = KVR::JointRotationFilterOption::Unfiltered;
            }
        }
    });
    SetAllJointsRotFiltered->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers] {
        for (KVR::KinectTrackedDevice &d : v_trackers) {
            if (d.isSensor()) {}
            else {
                d.rotationFilterOption = KVR::JointRotationFilterOption::Filtered;
            }
        }
    });
    SetAllJointsRotHead->GetSignal(sfg::Widget::OnLeftClick).Connect([this, &v_trackers] {
        for (KVR::KinectTrackedDevice &d : v_trackers) {
            if (d.isSensor()) {}
            else {
                d.rotationFilterOption = KVR::JointRotationFilterOption::HeadLook;
            }
        }
    });
}
void updateTrackerInitButtonLabelFail() {
    TrackerInitButton->SetLabel("Input Emulator not connected! Can't init trackers");
}

void setReconnectControllerButtonSignal(VRcontroller& left, VRcontroller& right, vr::IVRSystem* &sys
) {
    ReconControllersButton->GetSignal(sfg::Button::OnLeftClick).Connect([&left, &right, &sys, this] {
        std::stringstream stream;
        stream << "If controller input isn't working, press this to reconnect them.\n Make sure both are on, and not in standby.\n";
        if (right.Connect(sys)) {
            stream << "RIGHT: OK!\t";
        }
        else {
            stream << "RIGHT: DISCONNECTED!\t";
        }
        if (left.Connect(sys)) {
            stream << "LEFT: OK!\t";
        }
        else {
            stream << "LEFT: DISCONNECTED!\t";
        }
        ReconControllersLabel->SetText(stream.str());
    });
}

void setLineWrapping() {
    InferredLabel->SetLineWrap(true);
    InferredLabel->SetRequisition(sf::Vector2f(600.f, 20.f));

    InstructionsLabel->SetLineWrap(true);
    InstructionsLabel->SetRequisition(sf::Vector2f(600.f, 50.f));

    CalibrationSettingsLabel->SetLineWrap(true);
    CalibrationSettingsLabel->SetRequisition(sf::Vector2f(600.f, 20.f));
}
void packElementsIntoMainBox() {
    //Statuses are at the top
    mainGUIBox->Pack(KinectStatusLabel);
    mainGUIBox->Pack(SteamVRStatusLabel);
    mainGUIBox->Pack(InputEmulatorStatusLabel);

    auto fontSizeBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
    fontSizeBox->Pack(FontSizeScaleLabel);
    fontSizeBox->Pack(FontSizeScale);
    mainGUIBox->Pack(fontSizeBox);

    mainGUIBox->Pack(reconKinectButton);
    mainGUIBox->Pack(TrackerInitButton);
    mainGUIBox->Pack(TrackerLastInitButton);
    mainGUIBox->Pack(InstructionsLabel);

    setHipScaleBox();
    mainGUIBox->Pack(ShowSkeletonButton);

    mainGUIBox->Pack(EnableGamepadButton);
    mainGUIBox->Pack(ReconControllersLabel);
    mainGUIBox->Pack(ReconControllersButton);

    mainGUIBox->Pack(KinectRotLabel);
    mainGUIBox->Pack(KinectRotButton);

    mainGUIBox->Pack(KinectPosLabel);
    mainGUIBox->Pack(KinectPosButton);

    
    mainGUIBox->Pack(InferredLabel);
    mainGUIBox->Pack(IgnoreInferredCheckButton);
    
}

void setHipScaleBox() {
    auto HipLabel = sfg::Label::Create("Vertical Hip Adjustment (metres)");
    HipScale->SetDigits(3);
    
    HipScaleBox->Pack(HipLabel, false, false);
    HipScaleBox->Pack(HipScale);
    mainGUIBox->Pack(HipScaleBox);
}
void packElementsIntoTrackingMethodBox() {
    //trackingMethodBox->Pack(InitiateColorTrackingButton);
    //trackingMethodBox->Pack(DestroyColorTrackingButton);

    trackingMethodBox->Pack(TrackingMethodLabel);

    sfg::Box::Ptr horizontalPSMBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 5.f);
    horizontalPSMBox->Pack(StartPSMoveHandler);
    horizontalPSMBox->Pack(StopPSMoveHandler);
    horizontalPSMBox->Pack(PSMoveHandlerLabel);

    trackingMethodBox->Pack(horizontalPSMBox);
}

void updateDeviceLists() {
    setDeviceListItems(PositionDeviceList);
    setDeviceListItems(RotationDeviceList);
}

void packElementsIntoAdvTrackerBox() {
    advancedTrackerBox->Pack(AddHandControllersToList);
    advancedTrackerBox->Pack(AddLowerTrackersToList);

    auto jointBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
    jointBox->Pack(SetJointsToFootRotationButton);
    jointBox->Pack(SetJointsToAnkleRotationButton);
    advancedTrackerBox->Pack(jointBox);

    advancedTrackerBox->Pack(SetAllJointsRotUnfiltered);
    advancedTrackerBox->Pack(SetAllJointsRotFiltered);
    advancedTrackerBox->Pack(SetAllJointsRotHead);

    advancedTrackerBox->Pack(TrackerList);
    advancedTrackerBox->Pack(showJointDevicesButton);

    TrackerList->Pack(TrackerListLabel);

    setBonesListItems();
    updateDeviceLists();
    setRolesListItems(RolesList);

    advancedTrackerBox->Pack(calibrateOffsetButton);

    auto positionBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 5.f);
    auto rotationBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 5.f);
    auto selectionBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 5.f);
    auto connectBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 5.f);
    
    //TrackerListOptionsBox->Pack(BonesList);

    positionBox->Pack(PositionDeviceList);
    positionBox->Pack(identifyPosDeviceButton);

    rotationBox->Pack(RotationDeviceList);
    rotationBox->Pack(identifyRotDeviceButton);

    selectionBox->Pack(refreshDeviceListButton);
    selectionBox->Pack(positionBox);
    selectionBox->Pack(rotationBox);

    connectBox->Pack(RolesList);
    connectBox->Pack(IsControllerButton);
    connectBox->Pack(AddTrackerToListButton);
    connectBox->Pack(RemoveTrackerFromListButton);

    TrackerListOptionsBox->Pack(selectionBox);
    TrackerListOptionsBox->Pack(connectBox);


    advancedTrackerBox->Pack(TrackerListOptionsBox);
}
void packElementsIntoCalibrationBox() {
    sfg::Box::Ptr verticalBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);

    verticalBox->Pack(CalibrationSettingsLabel);

    auto horizontalPosBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
    horizontalPosBox->Pack(CalibrationPosLabel);
    CalibrationEntryPosX->SetDigits(4);
    horizontalPosBox->Pack(CalibrationEntryPosX);
    CalibrationEntryPosY->SetDigits(4);
    horizontalPosBox->Pack(CalibrationEntryPosY);
    CalibrationEntryPosZ->SetDigits(4);
    horizontalPosBox->Pack(CalibrationEntryPosZ);
    verticalBox->Pack(horizontalPosBox);

    auto horizontalRotBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
    horizontalRotBox->Pack(CalibrationRotLabel);
    CalibrationEntryRotX->SetDigits(4);
    horizontalRotBox->Pack(CalibrationEntryRotX);
    CalibrationEntryRotY->SetDigits(4);
    horizontalRotBox->Pack(CalibrationEntryRotY);
    CalibrationEntryRotZ->SetDigits(4);
    horizontalRotBox->Pack(CalibrationEntryRotZ);
    verticalBox->Pack(horizontalRotBox);
    verticalBox->Pack(CalibrationSaveButton);

    verticalBox->Pack(ActivateVRSceneTypeButton);

    calibrationBox->Pack(verticalBox);
}
void setBonesListItems() {
    using namespace KVR;
    for (int i = 0; i < KinectJointCount; ++i) {
        BonesList->AppendItem(KinectJointName[i]);
    }
    // Set as default - to prevent garbage additions 
    BonesList->SelectItem(0);
}

void setDeviceListItems(sfg::ComboBox::Ptr comboBox) {
    comboBox->Clear();
    for (int i = 0; i < TrackingPoolManager::count(); ++i) {
        if (kinectJointDevicesHiddenFromList && TrackingPoolManager::trackerIdInKinectRange(i)) {
            continue;
        }
        comboBox->AppendItem(TrackingPoolManager::deviceGuiString(i));
    }
    // Set as default - to prevent garbage additions 
    comboBox->SelectItem(0);
}

void setRolesListItems(sfg::ComboBox::Ptr comboBox) {
    for (int i = 0; i < (int)KVR::KinectDeviceRole::Count; ++i) {
		comboBox->AppendItem(KVR::KinectDeviceRoleName[i]);
    }
    // Set as default - to prevent garbage additions 
	comboBox->SelectItem(0);
}

void updateKinectStatusLabel(KinectHandlerBase& kinect) {
    HRESULT status = kinect.getStatusResult();
    if (kinect.isInitialised()) {
        if (status == lastKinectStatus)
            return; // No need to waste time updating it;
        switch (status) {
        case S_OK:
            KinectStatusLabel->SetText("Kinect Status: Success!");
            break;
        default:
            KinectStatusLabel->SetText("Kinect Status: ERROR " + kinect.statusResultString(status));
            break;
        }
    }
    else
        updateKinectStatusLabelDisconnected();
    if (status != lastKinectStatus) {
        LOG(INFO) << "Kinect Status changed to: " << KinectStatusLabel->GetText().toAnsiString();
        lastKinectStatus = status;
    }
}


void updateEmuStatusLabelError(vrinputemulator::vrinputemulator_connectionerror e) {
    InputEmulatorStatusLabel->SetText("Input Emu Status: NOT Connected! Error " + std::to_string(e.errorcode) + " " + e.what() + "\n\n Is SteamVR open and InputEmulator installed?");
}
void updateEmuStatusLabelSuccess() {
    InputEmulatorStatusLabel->SetText("Input Emu Status: Success!");
}

void updateVRStatusLabel(vr::EVRInitError eError) {
    if (eError == vr::VRInitError_None)
        SteamVRStatusLabel->SetText("SteamVR Status: Success!");
    else
        SteamVRStatusLabel->SetText("SteamVR Status: ERROR " + std::to_string(eError) + "\nPlease restart K2VR with SteamVR successfully running!");
}

void setTrackingMethodsReference(std::vector<std::unique_ptr<TrackingMethod>> & ref) {
    v_trackingMethodsRef = &ref;
}
void setDeviceHandlersReference(std::vector<std::unique_ptr<DeviceHandler>> & ref) {
    v_deviceHandlersRef = &ref;
}

void updateWithNewWindowSize(sf::Vector2f size) {
    guiWindow->SetAllocation(sf::FloatRect(0.f, 0.f, .4f * size.x, .4f * size.y));
    //setScale();
    //guiWindow->SetAllocation(sf::FloatRect(size.x - width, 0.f, width, size.y));
    //mGUI.SideBar->SetAllocation(sf::FloatRect(0.f, 0.f, width, size.y));
}
void setVirtualHipsBoxSignals() {
    using namespace VirtualHips;

    VirtualHipHeightFromHMDButton->SetDigits(2);
    VirtualHipSittingThreshold->SetDigits(2);
    VirtualHipLyingThreshold->SetDigits(2);

    VirtualHipUseHMDYawButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        settings.followHmdYawRotation = (VirtualHipUseHMDYawButton->IsActive());
    });
    VirtualHipUseHMDPitchButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        settings.followHmdPitchRotation = (VirtualHipUseHMDPitchButton->IsActive());
    });
    VirtualHipUseHMDRollButton->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        settings.followHmdRollRotation = (VirtualHipUseHMDRollButton->IsActive());
    });

    VirtualHipLockToHeadButton->GetSignal(sfg::RadioButton::OnToggle).Connect([this] {
        settings.positionAccountsForFootTrackers = !VirtualHipLockToHeadButton->IsActive();
    }
    );
    VirtualHipLockToFeetButton->GetSignal(sfg::RadioButton::OnToggle).Connect([this] {
        settings.positionAccountsForFootTrackers = VirtualHipLockToFeetButton->IsActive();
    }
    );

    VirtualHipHeightFromHMDButton->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        settings.heightFromHMD = VirtualHipHeightFromHMDButton->GetValue();
    }
    );
    VirtualHipFollowHMDLean->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] {
        settings.positionFollowsHMDLean = (VirtualHipFollowHMDLean->IsActive());
    });

    VirtualHipSittingThreshold->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        float sittingThreshold = VirtualHipSittingThreshold->GetValue();
        float lyingThreshold = VirtualHipLyingThreshold->GetValue();
        if (sittingThreshold < lyingThreshold) {
            settings.sittingMaxHeightThreshold = lyingThreshold;
            VirtualHipSittingThreshold->SetValue(lyingThreshold);
        }
        else {
            settings.sittingMaxHeightThreshold = sittingThreshold;
        }
    }
    );
    VirtualHipLyingThreshold->GetSignal(sfg::SpinButton::OnValueChanged).Connect([this] {
        float sittingThreshold = VirtualHipSittingThreshold->GetValue();
        float lyingThreshold = VirtualHipLyingThreshold->GetValue();
        settings.lyingMaxHeightThreshold = lyingThreshold;       
        VirtualHipSittingThreshold->SetValue(sittingThreshold); // Necessary to ensure that sitting isn't lower than lying down. Curious as to what would happen.
    }
    );

    VirtualHipConfigSaveButton->GetSignal(sfg::Button::OnLeftClick).Connect([this] {
        VirtualHips::saveSettings();
    }
    );
}
void loadVirtualHipSettingsIntoGUIElements()
{
    // Retrieve the values from config
    using namespace VirtualHips;
    retrieveSettings();

    VirtualHipUseHMDYawButton->SetActive(settings.followHmdYawRotation);
    VirtualHipUseHMDPitchButton->SetActive(settings.followHmdPitchRotation);
    VirtualHipUseHMDRollButton->SetActive(settings.followHmdRollRotation);

    VirtualHipLockToHeadButton->SetActive(!settings.positionAccountsForFootTrackers);

    VirtualHipHeightFromHMDButton->SetValue(settings.heightFromHMD);
    VirtualHipFollowHMDLean->SetActive(settings.positionFollowsHMDLean);

    VirtualHipSittingThreshold->SetValue(settings.sittingMaxHeightThreshold);
    VirtualHipLyingThreshold->SetValue(settings.lyingMaxHeightThreshold);
}
// Virtual Hips Menu
void packElementsIntoVirtualHipsBox() {
    loadVirtualHipSettingsIntoGUIElements();
    setVirtualHipsBoxSignals();

    virtualHipsBox->SetSpacing(0.5f);
    auto rotationSettingsBox = sfg::Box::Create();
    rotationSettingsBox->Pack(sfg::Label::Create("Follow HMD Rotation: "));
    rotationSettingsBox->Pack(VirtualHipUseHMDYawButton);
    rotationSettingsBox->Pack(VirtualHipUseHMDPitchButton);
    rotationSettingsBox->Pack(VirtualHipUseHMDRollButton);

    virtualHipsBox->Pack(rotationSettingsBox);

    auto hipLockingBox = sfg::Box::Create();
    hipLockingBox->Pack(sfg::Label::Create("Hips lock to: "));
    std::shared_ptr<sfg::RadioButtonGroup> hipLockGroup = VirtualHipLockToHeadButton->GetGroup();
    VirtualHipLockToFeetButton->SetGroup(hipLockGroup);

    auto hipLockingRadioBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 20.f);
    hipLockingRadioBox->Pack(VirtualHipLockToHeadButton);
    hipLockingRadioBox->Pack(sfg::Label::Create("or"));
    hipLockingRadioBox->Pack(VirtualHipLockToFeetButton);

    hipLockingBox->Pack(hipLockingRadioBox);

    virtualHipsBox->Pack(hipLockingBox);

    
    auto modeTitleBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
    auto standingBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
    modeTitleBox->Pack(sfg::Label::Create("-- Standing Settings --"));
    standingBox->Pack(sfg::Label::Create("Hip distance from head (Meters)"));
    standingBox->Pack(VirtualHipHeightFromHMDButton);
    standingBox->Pack(VirtualHipFollowHMDLean);
    

    auto sittingBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
    modeTitleBox->Pack(sfg::Label::Create("-- Sitting Settings --"));
    sittingBox->Pack(sfg::Label::Create("Sitting mode cutoff height (Meters)"));
    sittingBox->Pack(VirtualHipSittingThreshold);

    auto lyingBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
    modeTitleBox->Pack(sfg::Label::Create("-- Lying Settings --"));
    lyingBox->Pack(sfg::Label::Create("Lying mode cutoff height (Meters)"));
    lyingBox->Pack(VirtualHipLyingThreshold);

    auto modeBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
    modeBox->Pack(standingBox);
    modeBox->Pack(sittingBox);
    modeBox->Pack(lyingBox);

    virtualHipsBox->Pack(modeTitleBox);
    virtualHipsBox->Pack(modeBox);
    virtualHipsBox->Pack(VirtualHipConfigSaveButton);
}



private:
    sf::Font mainGUIFont;
    sfg::SFGUI sfguiRef;
    sfg::Window::Ptr guiWindow = sfg::Window::Create();
    sfg::Notebook::Ptr mainNotebook = sfg::Notebook::Create();


    std::vector<std::unique_ptr<DeviceHandler>> * v_deviceHandlersRef;
    std::vector<std::unique_ptr<TrackingMethod>> * v_trackingMethodsRef;

    // All the device handlers
    PSMoveHandler psMoveHandler;

    HRESULT lastKinectStatus = E_FAIL;

    sfg::Desktop guiDesktop;

    sfg::Box::Ptr mainGUIBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5.f);
    sfg::Box::Ptr calibrationBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 5.f);
    sfg::Box::Ptr advancedTrackerBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5.f);
    sfg::Box::Ptr trackingMethodBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5.f);
    sfg::Box::Ptr virtualHipsBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5.f);
    
    sfg::Adjustment::Ptr fontSizeAdjustment = sfg::Adjustment::Create();
    sfg::Label::Ptr FontSizeScaleLabel = sfg::Label::Create("(WARNING, LAGS ON CHANGE) Font Size: ");
    sfg::SpinButton::Ptr FontSizeScale = sfg::SpinButton::Create(sfg::Adjustment::Create(SFMLsettings::globalFontSize, 5.f, 100.f, .5f));
    float lastFontSizeValue = SFMLsettings::globalFontSize;


    //Statuses
    sfg::Label::Ptr KinectStatusLabel = sfg::Label::Create();
    sfg::Label::Ptr SteamVRStatusLabel = sfg::Label::Create();
    sfg::Label::Ptr InputEmulatorStatusLabel = sfg::Label::Create();

    sfg::Button::Ptr reconKinectButton = sfg::Button::Create("Reconnect Kinect");
    sfg::Button::Ptr TrackerInitButton = sfg::Button::Create("**Please be in VR before hitting me!** Initialise SteamVR Kinect Trackers - HIT ME");
    sfg::Button::Ptr TrackerLastInitButton = sfg::Button::Create("**Please be in VR before hitting me!** Spawn same trackers as last session");

    sfg::Button::Ptr ShowSkeletonButton = sfg::CheckButton::Create("Show/Hide Skeleton Tracking: MAY CAUSE LAG IN TRACKERS");

    //Zeroing
    sfg::Label::Ptr KinectRotLabel = sfg::Label::Create("Calibrate the rotation of the Kinect sensor with the controller thumbsticks. Press the trigger to confirm.");
    sfg::CheckButton::Ptr KinectRotButton = sfg::CheckButton::Create("Enable Kinect Rotation Calibration");


    //Position Adjust
    sfg::Label::Ptr KinectPosLabel = sfg::Label::Create("Calibrate the position of the Kinect sensor with the controller thumbsticks. Press the trigger to confirm.");
    sfg::CheckButton::Ptr KinectPosButton = sfg::CheckButton::Create("Enable Kinect Position Calibration");


    // Controllers
    sfg::CheckButton::Ptr EnableGamepadButton = sfg::CheckButton::Create("Enable Gamepad Calibration Controls");
    sfg::Label::Ptr ReconControllersLabel = sfg::Label::Create("If controller input isn't working, press this to reconnect them.\n Make sure both are on, and not in standby.");
    sfg::Button::Ptr ReconControllersButton = sfg::Button::Create("Reconnect VR Controllers");


    sfg::Label::Ptr InferredLabel = sfg::Label::Create("Checking this stops the trackers if it's not absolutely 100% sure where they are. Leaving this disabled may cause better tracking in poorly lit environments, but at the cost of slight jerks aside sometimes.");
    sfg::CheckButton::Ptr IgnoreInferredCheckButton = sfg::CheckButton::Create("Disable Raw Positional Tracking");

    sfg::Button::Ptr SetJointsToFootRotationButton = sfg::Button::Create("Enable (buggy) foot rotation for 360 Kinect");
    sfg::Button::Ptr SetJointsToAnkleRotationButton = sfg::Button::Create("Disable (buggy) foot rotation for 360 Kinect");

    sfg::Button::Ptr SetAllJointsRotUnfiltered = sfg::Button::Create("Disable rotation smoothing for ALL joints (Rotation smoothing is in development!!!)");
    sfg::Button::Ptr SetAllJointsRotFiltered = sfg::Button::Create("Enable rotation smoothing for ALL joints (Rotation smoothing is in development!!!)");
    sfg::Button::Ptr SetAllJointsRotHead = sfg::Button::Create("Use Head orientation for ALL joints - may fix issues with jumping trackers at cost of limited rotation");

    sfg::Label::Ptr InstructionsLabel = sfg::Label::Create("Stand in front of the Kinect sensor.\n If the trackers don't update, then try crouching slightly until they move.\n\n Calibration: The arrow represents the position and rotation of the Kinect - match it as closely to real life as possible for the trackers to line up.\n\n The arrow pos/rot is set with the thumbsticks on the controllers, and confirmed with the trigger.");    //Blegh - There has to be a better way than this, maybe serialization?

    sfg::Label::Ptr CalibrationSettingsLabel = sfg::Label::Create("These settings are here for manual entry, and saving until a proper configuration system is implemented.\nYou can use this to quickly calibrate if your Kinect is in the same place. \n(Rotation is in radians, and Pos should be in meters roughly)");
    sfg::Label::Ptr CalibrationPosLabel = sfg::Label::Create("Position x, y, z");
    sfg::SpinButton::Ptr CalibrationEntryPosX = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::kinectRepPosition.v[0], -10.f, 10.f, .01f, .2f));
    sfg::SpinButton::Ptr CalibrationEntryPosY = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::kinectRepPosition.v[1], -10.f, 10.f, .01f, .2f));
    sfg::SpinButton::Ptr CalibrationEntryPosZ = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::kinectRepPosition.v[2], -10.f, 10.f, .01f, .2f));

    sfg::Label::Ptr CalibrationRotLabel = sfg::Label::Create("Rotation x, y, z");
    sfg::SpinButton::Ptr CalibrationEntryRotX = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::kinectRadRotation.v[0], -10.f, 10.f, .01f, .2f));
    sfg::SpinButton::Ptr CalibrationEntryRotY = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::kinectRadRotation.v[1], -10.f, 10.f, .01f, .2f));
    sfg::SpinButton::Ptr CalibrationEntryRotZ = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::kinectRadRotation.v[2], -10.f, 10.f, .01f, .2f));

    sfg::Button::Ptr CalibrationSaveButton = sfg::Button::Create("Save Calibration Values");

    sfg::Button::Ptr ActivateVRSceneTypeButton = sfg::Button::Create("Show K2VR in the VR Bindings Menu!");

    //Adv Trackers
    sfg::Button::Ptr calibrateOffsetButton = sfg::Button::Create("Calibrate VR Offset");
    sfg::Button::Ptr AddHandControllersToList = sfg::Button::Create("Add Hand Controllers");
    sfg::Button::Ptr AddLowerTrackersToList = sfg::Button::Create("Add Lower Body Trackers");

    bool userSelectedDeviceRotIndex = false;
    bool userSelectedDevicePosIndex = false;
    sfg::Box::Ptr TrackerList = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5);
    sfg::Label::Ptr TrackerListLabel = sfg::Label::Create("Trackers to be spawned:");

    sfg::Box::Ptr TrackerListOptionsBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 5);
    sfg::SpinButton::Ptr HipScale = sfg::SpinButton::Create(sfg::Adjustment::Create(KinectSettings::hipRoleHeightAdjust, -1.f, 1.f, .01f));
    sfg::Box::Ptr HipScaleBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);

    bool kinectJointDevicesHiddenFromList = true;
    sfg::CheckButton::Ptr showJointDevicesButton = sfg::CheckButton::Create("Show joints in devices");
    sfg::Button::Ptr refreshDeviceListButton = sfg::Button::Create("Refresh Devices");
    sfg::ComboBox::Ptr BonesList = sfg::ComboBox::Create();
    sfg::ComboBox::Ptr PositionDeviceList = sfg::ComboBox::Create();
    sfg::CheckButton::Ptr identifyPosDeviceButton = sfg::CheckButton::Create("Boop");
    sfg::ComboBox::Ptr RotationDeviceList = sfg::ComboBox::Create();
    sfg::CheckButton::Ptr identifyRotDeviceButton = sfg::CheckButton::Create("Beep");
    sfg::ComboBox::Ptr RolesList = sfg::ComboBox::Create();
    sfg::CheckButton::Ptr IsControllerButton = sfg::CheckButton::Create("Controller");
    sfg::Button::Ptr AddTrackerToListButton = sfg::Button::Create("Add");
    sfg::Button::Ptr RemoveTrackerFromListButton = sfg::Button::Create("Remove");

    std::vector<TempTracker> TrackersToBeInitialised;


    //Tracking Method Box
    sfg::Button::Ptr InitiateColorTrackingButton = sfg::Button::Create("Start Color Tracker");
    sfg::Button::Ptr DestroyColorTrackingButton = sfg::Button::Create("Destroy Color Tracker");
    sfg::Label::Ptr TrackingMethodLabel = sfg::Label::Create("Click the corresponding button for the devices you wish to use, and K2VR will try its best to connect to them. (Go to the 'Adv. Trackers' tab once these are connected.");

    sfg::Button::Ptr StartPSMoveHandler = sfg::Button::Create("Run PS Move Handler");
    sfg::Button::Ptr StopPSMoveHandler = sfg::Button::Create("Stop PS Move Handler");
    sfg::Label::Ptr PSMoveHandlerLabel = sfg::Label::Create("Status: Off");


    // Virtual Hips Box
    sfg::CheckButton::Ptr VirtualHipUseHMDYawButton = sfg::CheckButton::Create("Yaw");
    sfg::CheckButton::Ptr VirtualHipUseHMDPitchButton = sfg::CheckButton::Create("Pitch");
    sfg::CheckButton::Ptr VirtualHipUseHMDRollButton = sfg::CheckButton::Create("Roll");

    sfg::RadioButton::Ptr VirtualHipLockToHeadButton = sfg::RadioButton::Create("Head");
    sfg::RadioButton::Ptr VirtualHipLockToFeetButton = sfg::RadioButton::Create("Feet");

    sfg::SpinButton::Ptr VirtualHipHeightFromHMDButton = sfg::SpinButton::Create(sfg::Adjustment::Create(VirtualHips::settings.heightFromHMD, 0.f, 2.f, 0.01f));
    sfg::CheckButton::Ptr VirtualHipFollowHMDLean = sfg::CheckButton::Create("Follow HMD Lean");

    sfg::SpinButton::Ptr VirtualHipSittingThreshold = sfg::SpinButton::Create(sfg::Adjustment::Create(VirtualHips::settings.sittingMaxHeightThreshold, 0.f, 2.f, 0.01f));

    sfg::SpinButton::Ptr VirtualHipLyingThreshold = sfg::SpinButton::Create(sfg::Adjustment::Create(VirtualHips::settings.lyingMaxHeightThreshold, 0.f, 2.f, 0.01f));

    sfg::Button::Ptr VirtualHipConfigSaveButton = sfg::Button::Create("Save Settings");

    void updateKinectStatusLabelDisconnected() {
        KinectStatusLabel->SetText("Kinect Status: ERROR KINECT NOT DETECTED");
    }
    void showPostTrackerInitUI(bool show = true) {
        InstructionsLabel->Show(show);
        KinectRotLabel->Show(show);
        KinectRotButton->Show(show);
        KinectPosLabel->Show(show);
        KinectPosButton->Show(show);
        ReconControllersLabel->Show(show);
        ReconControllersButton->Show(show);
        InferredLabel->Show(show);
        IgnoreInferredCheckButton->Show(show);
        SetAllJointsRotUnfiltered->Show(show);
        HipScale->Show(show);
        HipScaleBox->Show(show);
        SetAllJointsRotHead->Show(show);
        SetAllJointsRotFiltered->Show(show);
        SetJointsToAnkleRotationButton->Show(show);
        SetJointsToFootRotationButton->Show(show);
        calibrateOffsetButton->Show(show);

        //calibrationBox->Show(show);
    }
    void hidePostTrackerInitUI() {
        showPostTrackerInitUI(false);
    }
};