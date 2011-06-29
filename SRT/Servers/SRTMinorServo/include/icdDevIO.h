/*******************************************************************************\
 *
 *  Author: Marco Buttu, mbuttu@oa-cagliari.inaf.it
 *  Last Modified: Wed Apr  8 15:55:28 CEST 2009
 *   
\*******************************************************************************/

#ifndef __ICDDEVIO__H
#define __ICDDEVIO__H

#ifndef __cplusplus
#error This is a C++ include file and cannot be used from plain C
#endif

#include <baciDevIO.h>
#include <IRA>
#include "icdSocket.h"
#include <ComponentErrors.h>
#include <LogFilter.h>

// #define DEBUG_TSS // If defined the "TEST STATUS SIGNALS" block turns on
// #define DEBUG_WRT // If defined the "WRONG RESPONSE TEST" block turns on
// #define DEBUG_SPE // If defined the "SWITCH POLLING ERROR" block turns on

using namespace IRA;

template <class T> class icdDevIO : public DevIO<T>
{
public:
    enum TLinkedProperty {
        POSITION,          /*!< the devio will be used to read/write the ICD position */
        VERBOSE_STATUS,    /*!< the devio will be used to read the ICD verbose status */
        SUMMARY_STATUS     /*!< the devio will be used to read the ICD summary status */
    };
    
    icdDevIO(
            CSecureArea<icdSocket>* link, 
            TLinkedProperty Property,
            DDWORD guardInterval=1000000
            ) : m_icdLink(link), m_Property(Property), m_logGuard(guardInterval)
    {
        switch (m_Property) {
            case POSITION : {
                m_PropertyName=CString("ICD Position");
                break;
            }
            case VERBOSE_STATUS : {
                m_PropertyName=CString("ICD Verbose Status");
                break;
            }
            case SUMMARY_STATUS : {
                m_PropertyName=CString("ICD Summary Status");
                break;
            }
        }

        CString trace("icdDevIO::icdDevIO() ");
        trace += m_PropertyName;
        AUTO_TRACE((const char*)trace);     
    }

    ~icdDevIO()
    {
        CString trace("icdDevIO::~icdDevIO() ");
        trace += m_PropertyName;
        ACS_TRACE((const char*)trace);      
    }

    bool initializeValue()
    {       
        CString trace("icdDevIO::initializeValue() ");
        trace += m_PropertyName;
        AUTO_TRACE((const char*)trace);     
        return false;
    }
    
    /**
     * Used to read the property value.
     * @throw ComponentErrors::PropertyError
     *       @arg \c ComponentErrors::Timeout</a>
     *       @arg \c AntennaErrors::Connection</a>   
     *       @arg \c ComponentErrors::SocketError
     * @param timestamp epoch when the operation completes
    */ 
    T read(ACS::Time& timestamp) throw (ACSErr::ACSbaseExImpl)
    {
        CString trace("icdDevIO::read() ");
        trace += m_PropertyName;
        ACS_TRACE((const char*)trace);      
        CSecAreaResourceWrapper <icdSocket> icd_socket = m_icdLink->Get();
        try {
            switch (m_Property) {
                case POSITION : {
                    m_Value = (T)icd_socket->getPosition();
                    break;
                }
                case VERBOSE_STATUS : {
                    m_Value = (T)icd_socket->getVerboseStatus();
                    break;
                }
                case SUMMARY_STATUS : {
                    m_Value = (T)icd_socket->getSummaryStatus();
                    break;
                }
            }
        }
        catch (ACSErr::ACSbaseExImpl& E) {
            _ADD_BACKTRACE(
                    ComponentErrors::PropertyErrorExImpl, 
                    dummy, 
                    E, 
                    "icdDevIO::read()"
                    );
            dummy.setPropertyName((const char *)m_PropertyName);
            dummy.setReason("Property could not be read");
            _IRA_LOGGUARD_LOG_EXCEPTION(m_logGuard,dummy, LM_DEBUG);
            throw dummy;
        }               
        timestamp = getTimeStamp();  // Complition time
        return m_Value;
    }

    // It writes values into controller. Unused because the properties are read-only.
    // void write(const T& value, ACS::Time& timestamp) throw (ACSErr::ACSbaseExImpl)
    void write(const double &value, ACS::Time& timestamp) throw (ACSErr::ACSbaseExImpl)
    {
        CString trace("icdDevIO::read() ");
        trace += m_PropertyName;
        ACS_TRACE((const char*)trace);      
        CSecAreaResourceWrapper <icdSocket> icd_socket = m_icdLink->Get();
        try {
            switch (m_Property) {
                case POSITION : {
                    icd_socket->setPosition(value);
                    break;
                }
                case VERBOSE_STATUS : {
                     break;
                 }
                case SUMMARY_STATUS : {
                     break;
                 }
            }
        }
        catch (ACSErr::ACSbaseExImpl& E) {
            _ADD_BACKTRACE(
                    ComponentErrors::PropertyErrorExImpl, 
                    dummy, 
                    E, 
                    "icdDevIO::write()"
                    );
            dummy.setPropertyName((const char *)m_PropertyName);
            dummy.setReason("Property could not be ");
            _IRA_LOGGUARD_LOG_EXCEPTION(m_logGuard,dummy, LM_DEBUG);
            throw dummy;
        }               
        catch (MinorServoErrors::PositioningErrorEx E) {
            _ADD_BACKTRACE(
                ComponentErrors::PropertyErrorExImpl, 
                dummy, 
                E, 
                "icdDevIO::write()"
            );
            dummy.setPropertyName((const char *)m_PropertyName);
            dummy.setReason("Property could not be write");
            _IRA_LOGGUARD_LOG_EXCEPTION(m_logGuard,dummy, LM_DEBUG);
            throw dummy;
        }               
        timestamp = getTimeStamp();  // Complition time
        return;
    }
    
private:
    CSecureArea<icdSocket>* m_icdLink;
    T m_Value;
    TLinkedProperty m_Property;
    CString m_PropertyName;
    CLogGuard m_logGuard;
};

#endif 
