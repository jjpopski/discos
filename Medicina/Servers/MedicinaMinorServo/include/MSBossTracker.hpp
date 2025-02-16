/*******************************************************************************\
 *  Author Infos
 *  ============
 *  Name:         Marco Buttu, Marco Bartolini
 *  E-mail:       mbuttu@oa-cagliari.inaf.it
 *  Personal Web: http://www.pypeople.com/
\*******************************************************************************/
#ifndef __MSBOSS_TRACKER_H__
#define __MSBOSS_TRACKER_H__

#include <acsThread.h>
#include <acsncSimpleSupplier.h>
#include <baciCharacteristicComponentImpl.h>
#include <IRA>
#include <MinorServoS.h>
#include <MinorServoBossS.h>
#include <string>
#include <ManagmentDefinitionsS.h>
#include <AntennaBossC.h>
#include <MinorServoErrors.h>

#include "slamac.h" //for DR2D

#include <AntennaProxy.h>

#include "macros.def"
#include "MedMinorServoStatus.hpp"
#include "MedMinorServoControl.hpp"
#include "MedMinorServoOffset.hpp"
#include "MedMinorServoParameters.hpp"

// Sleep Time in 100ns
#define TRACKER_SLEEP_TIME 1000000 // 100 mseconds
#define READY_MAX_WAIT 3000000000 // 5 minutes

using namespace std;

class TrackerThreadParameters
{
    public:
        TrackerThreadParameters(MedMinorServoStatus *, 
                                MedMinorServoControl_sp,
                                MedMinorServoParameters **,
                                MedMinorServoOffset *,
	                            maci::ContainerServices *,
                                std::string);
        virtual ~TrackerThreadParameters();
        MedMinorServoStatus *m_status;
        MedMinorServoControl_sp m_control;
        MedMinorServoParameters **m_parameters;
        MedMinorServoOffset *m_offset;
	    maci::ContainerServices *m_services;
        std::string m_antennaBossInterface;
};

// This class updates the status property of minor servos.
class MSBossTracker : public ACS::Thread
{
public:

	MSBossTracker(
        const ACE_CString& name, 
        TrackerThreadParameters& params,
        const ACS::TimeInterval& responseTime=ThreadBase::defaultResponseTime,
        const ACS::TimeInterval& sleepTime=TRACKER_SLEEP_TIME,
        const bool del=true
    );

    ~MSBossTracker();

    /// This method is executed once when the thread starts.
    virtual void onStart();

    /// This method is executed once when the thread stops.
    virtual void onStop();

    /// This method overrides the thread implementation class.
    virtual void runLoop();

private:
    MedMinorServoStatus *m_status;
    MedMinorServoControl_sp m_control;
    MedMinorServoParameters **m_parameters;
    MedMinorServoOffset *m_offset;
    maci::ContainerServices * m_services;
    Antenna::AntennaBoss_proxy m_antennaBoss;
    std::string m_antennaBossInterface;
    bool m_antennaBossError;
};

#endif 

