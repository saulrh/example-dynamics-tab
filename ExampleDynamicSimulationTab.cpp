/*
 * Copyright (c) 2012, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Humanoid Robotics Lab      Georgia Institute of Technology
 * Director: Mike Stilman     http://www.golems.org
 */

#include <wx/wx.h>
#include <GUI/GUI.h>
#include <GUI/GRIPSlider.h>
#include <GUI/GRIPFrame.h>
#include <Tabs/GRIPTab.h>
#include <Tabs/AllTabs.h>
#include <GRIPApp.h>

#include <iostream>
#include <iomanip>
#include <cstdio>

#include <robotics/World.h>
#include <dynamics/BodyNodeDynamics.h>
#include <kinematics/BodyNode.h>

#include "ExampleDynamicSimulationTab.hpp"

#include <GUI/Viewer.h>

/////////////////////////////////////////////////////////////////////////////////////////////
//#########################################################################################//
//# wx events stuff                                                                       #//
//#########################################################################################//
/////////////////////////////////////////////////////////////////////////////////////////////

// Control IDs (used for event handling - be sure to start with a non-conflicted id)
enum ExampleExampleDynamicSimulationTabEvents {
    id_button_RunSim = 8345, // just to be safe
    id_button_RunFrame,
    id_button_StopSim,
    id_button_WriteHistory,
    id_button_InitDynamics,
    id_timer_Simulation
};

//Add a handler for any events that can be generated by the widgets you add here (sliders, radio, checkbox, etc)
BEGIN_EVENT_TABLE(ExampleDynamicSimulationTab, wxPanel)
EVT_BUTTON(id_button_RunSim, ExampleDynamicSimulationTab::OnButton)
EVT_BUTTON(id_button_RunFrame, ExampleDynamicSimulationTab::OnButton)
EVT_BUTTON(id_button_StopSim, ExampleDynamicSimulationTab::OnButton)
EVT_BUTTON(id_button_WriteHistory, ExampleDynamicSimulationTab::OnButton)
EVT_BUTTON(id_button_InitDynamics, ExampleDynamicSimulationTab::OnButton)
EVT_COMMAND(wxID_ANY, wxEVT_GRIP_SLIDER_CHANGE, ExampleDynamicSimulationTab::OnSlider)
EVT_TIMER(id_timer_Simulation, ExampleDynamicSimulationTab::OnTimer)
END_EVENT_TABLE()

// Class constructor for the tab: Each tab will be a subclass of RSTTab
IMPLEMENT_DYNAMIC_CLASS(ExampleDynamicSimulationTab, GRIPTab)

/////////////////////////////////////////////////////////////////////////////////////////////
//#########################################################################################//
//# ExampleDynamicSimulationTab constructor                                                      #//
//#########################################################################################//
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @function RipTabPlanner
 * @brief Constructor
 */
ExampleDynamicSimulationTab::ExampleDynamicSimulationTab( wxWindow *parent, const wxWindowID id,
                                            const wxPoint& pos, const wxSize& size, long style) :
GRIPTab(parent, id, pos, size, style)
{
    sizerFull = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBox* tabBox = new wxStaticBox(this, -1, wxT("Dynamics"));
    wxStaticBoxSizer* tabBoxSizer = new wxStaticBoxSizer(tabBox, wxHORIZONTAL);
    wxBoxSizer* simulationControlSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* stateSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* stateControlSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* simulationPropertySizer = new wxBoxSizer(wxVERTICAL);

    mSimTimer = new wxTimer(this, id_timer_Simulation);
    mCurrentSimState = NULL;

    simulationPropertySizer->Add(new wxButton(this, id_button_InitDynamics, wxT("Init Dynamics")),
                                0,     // do not resize to fit proportions vertically
                                wxALL, // border all around
                                1);    // border width is 1 so buttons are close together

    simulationControlSizer->Add(new wxButton(this, id_button_RunSim, wxT("Toggle Simulation")),
                                0,     // do not resize to fit proportions vertically
                                wxALL, // border all around
                                1);    // border width is 1 so buttons are close together
    simulationControlSizer->Add(new wxButton(this, id_button_RunFrame, wxT("Simulate One Frame")),
                                0,     // do not resize to fit proportions vertically
                                wxALL, // border all around
                                1);    // border width is 1 so buttons are close together
    simulationControlSizer->Add(new wxButton(this, id_button_WriteHistory, wxT("Save History")),
                                0,     // do not resize to fit proportions vertically
                                wxALL, // border all around
                                1);    // border width is 1 so buttons are close together

    stateSizer->Add(stateControlSizer,
                    1,                          // take up 1/4 of stateSizer
                    wxEXPAND | wxALIGN_CENTER,  // expand and center
                    0);                         // no border

    tabBoxSizer->Add(simulationPropertySizer,
                     1,         // take up 2/6 of the tab
                     wxEXPAND | wxALIGN_CENTER | wxALL,
                     1);
    tabBoxSizer->Add(simulationControlSizer,
                     1,         // take up 1/6 of the tab
                     wxEXPAND | wxALIGN_CENTER | wxALL,
                     1);
    tabBoxSizer->Add(stateSizer,
                     4,         // take up 3/6 of the tab
                     wxEXPAND | wxALIGN_CENTER | wxALL,
                     1);

    sizerFull->Add(tabBoxSizer, 1, wxEXPAND | wxALL, 6);

    SetSizer(sizerFull);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//#########################################################################################//
//# ExampleDynamicSimulationTab event handlers                                                   #//
//#########################################################################################//
/////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// normal buttons
////////////////////////////////////////////////////////////////

/**
 * @function OnButton
 * @brief Handle Button Events
 */
void ExampleDynamicSimulationTab::OnButton(wxCommandEvent &evt) {
    int button_num = evt.GetId();
  
    switch (button_num)
    {
    case id_button_RunSim:         /** Start or stop Simulating */
        {
        if ( mWorld == NULL ) {
            std::cout << "(!) Must have a world loaded to simulate (!)" << std::endl;
            break;
        }

        int n = 1800;
        std::cout << "(I) Simulating to frame " << n << std::endl;
        while(mCurrentSimState->mId < n)
            SimulateFrame();
        std::cout << "(I) Simulated to frame " << n << std::endl;
        viewer->UpdateCamera();
        
        PopulateTimeline();

        // if (mSimTimer->IsRunning())
        // {
        //     std::cout << "(I) Stopping simulation." << std::endl;
        //     mSimTimer->Stop();
        //     std::cout << "(I) Stopped simulation." << std::endl;
        // }
        // else
        // {
        //     std::cout << "(I) Starting simulation." << std::endl;
        //     // save the start
        //     if (mCurrentSimState == NULL)
        //         mCurrentSimState = new WorldState(mWorld);
        //     mSavedStates.push_back(mCurrentSimState);
        //     UpdateListBox();
        //     // start timer, milliseconds
        //     mTimerSetAt = clock();
        //     bool result = mSimTimer->Start(mWorld->mTimeStep * 5000, true);
        //     if (!result)
        //         std::cout << "(!) Could not start simulation." << std::endl;
        //     else
        //     std::cout << "(I) Started simulation." << std::endl;
        // }
        break;
    }
    case id_button_RunFrame:         /** Simulate one step */
    {
        if ( mWorld == NULL ) {
            std::cout << "(!) Must have a world loaded to simulate (!)" << std::endl;
            break;
        }
        std::cout << "(I) Simulating one frame." << std::endl;
        SimulateFrame();
        std::cout << "(I) Simulated one frame." << std::endl;
        break;
    }
    case id_button_WriteHistory:
    {
        if ( mWorld == NULL ) {
            std::cout << "(!) Must have a world loaded and some simulation done (!)" << std::endl;
            break;
        }
        PopulateTimeline();
        break;
    }
    case id_button_InitDynamics:
    {
        if ( mWorld == NULL ) {
            std::cout << "(!) Must have a world loaded (!)" << std::endl;
            break;
        }
        mCurrentSimState = new WorldState(mWorld);
        mWorld->mTimeStep = .0015;
        mWorld->mGravity = Eigen::Vector3d(0, 0, -9.8);
        for(int i = 0; i < mWorld->getNumSkeletons(); i++)
        {
            dynamics::SkeletonDynamics* skel = mWorld->getSkeleton(i);
            skel->initDynamics();
            skel->setPose(mCurrentSimState->mPosVects[i], true, true);
            skel->computeDynamics(mWorld->mGravity, mCurrentSimState->mVelVects[i], true);
        }
        // skeletons MUST be set immobile here and nowhere else, AFAICT
        mWorld->getSkeleton(0)->setImmobileState(true);
        mWorld->rebuildCollision();
        break;
    }
    }
}

////////////////////////////////////////////////////////////////
// slider changed
////////////////////////////////////////////////////////////////

/**
 * @function OnSlider
 * @brief Handle slider changes
 */
void ExampleDynamicSimulationTab::OnSlider(wxCommandEvent &evt) {
    int slnum = evt.GetId();
    double pos = *(double*) evt.GetClientData();
  
    switch (slnum) {
    // case slider_Time:
    //     break;
    default:
        break;
    }

    // if (frame != NULL)
    //     frame->SetStatusText(wxString(numBuf, wxConvUTF8));
}

////////////////////////////////////////////////////////////////
// timer tick
////////////////////////////////////////////////////////////////

/**
 * @function OnTimer
 * @brief Handle timer ticks
 */
void ExampleDynamicSimulationTab::OnTimer(wxTimerEvent &evt) {
    SimulateFrame();

    int msToWait = 1000;
    std::cout << "Waiting " << msToWait << " ms for next tick" << std::endl;
    mSimTimer->Start(msToWait, true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//#########################################################################################//
//# helper functions                                                                      #//
//#########################################################################################//
/////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// simulate one tick forward
////////////////////////////////////////////////////////////////

void ExampleDynamicSimulationTab::SimulateFrame()
{
    if (mCurrentSimState == NULL)
    {
        std::cout << "(!) must init dynamics before simulating (!)" << std::endl;
        return;
    }

    if(mCurrentSimState->mT > .5 && mCurrentSimState->mT < 1)
    {
        static_cast<dynamics::BodyNodeDynamics*>(mWorld->getSkeleton(1)->getNode(0))->
            addExtForce(Vector3d(0.0, 0.0, 0.0), Vector3d(-8, -50, 0));
        static_cast<dynamics::BodyNodeDynamics*>(mWorld->getSkeleton(2)->getNode(0))->
            addExtForce(Vector3d(0.0, 0.0, 0.0), Vector3d(16, 40, 0));
    }
        

    WorldIntegrator wi = WorldIntegrator(mWorld);
    mCurrentSimState = new WorldState(mCurrentSimState);

    wi.mWorldState = mCurrentSimState;
    mEuIntegrator.integrate(&wi, mWorld->mTimeStep);
    wi.mWorldState->writeToWorld(mWorld, true);
    viewer->UpdateCamera();

    mSimHistory.push_back(wi.mWorldState);
}

////////////////////////////////////////////////////////////////
// timeline population
////////////////////////////////////////////////////////////////

/**
 * @function PopulateTimeline
 * @brief Copies the simulation history into the timeline for easy review
 */
void ExampleDynamicSimulationTab::PopulateTimeline()
{
    std::cout
        << "(+) Populating Timeline. dt: "
        << mWorld->mTimeStep
        << " T: "
        << mSimHistory.back()->mT
        << std::endl;

    frame->InitTimer(string("Simulation_History"), mWorld->mTimeStep);

    for( std::vector<WorldState*>::iterator it = mSimHistory.begin(); it != mSimHistory.end(); it++)
    {
        // set each robot and object to the position recorded for that frame
        (*it)->writeToWorld(mWorld, false);

        // and record that world configuration
        frame->AddWorld( mWorld );
    }

    std::cout
        << "(+) Populated Timeline."
        << std::endl;
}

////////////////////////////////////////////////////////////////
// grip selector state change
////////////////////////////////////////////////////////////////

/**
 * @function GRIPStateChange
 * @brief This function is called when an object is selected in the Tree View or other
 *        global changes to the RST world. Use this to capture events from outside the tab.
 */
void ExampleDynamicSimulationTab::GRIPStateChange() {
    if ( selectedTreeNode == NULL ) {
        return;
    }
  
    std::string statusBuf;
    std::string buf, buf2;
  
    switch (selectedTreeNode->dType) {
    
    case Return_Type_Object:
        mSelectedObject = (robotics::Object*) ( selectedTreeNode->data );
        mSelectedRobot = NULL;
        mSelectedNode = NULL;
        statusBuf = " Selected Object: " + mSelectedObject->getName();
        buf = "You clicked on object: " + mSelectedObject->getName();
        
        // Enter action for object select events here:
    
        break;
    case Return_Type_Robot:
        mSelectedObject = NULL;
        mSelectedRobot = (robotics::Robot*) ( selectedTreeNode->data );
        mSelectedNode = NULL;
        statusBuf = " Selected Robot: " + mSelectedRobot->getName();
        buf = " You clicked on robot: " + mSelectedRobot->getName();

        // Enter action for Robot select events here:
    
        break;
    case Return_Type_Node:
        mSelectedObject = NULL;
        mSelectedRobot = NULL;
        mSelectedNode = (dynamics::BodyNodeDynamics*) ( selectedTreeNode->data );
        statusBuf = " Selected Body Node: " + string(mSelectedNode->getName()) + " of Robot: "
            + ( (robotics::Robot*) mSelectedNode->getSkel() )->getName();
        buf = " Node: " + std::string(mSelectedNode->getName()) + " of Robot: " + ( (robotics::Robot*) mSelectedNode->getSkel() )->getName();
    
        // Enter action for link select events here:
    
        break;
    default:
        fprintf(stderr, "--( :D ) Someone else's problem!\n");
        assert(0);
        exit(1);
    }
  
    //cout << buf << endl;
    frame->SetStatusText(wxString(statusBuf.c_str(), wxConvUTF8));
    sizerFull->Layout();
}
