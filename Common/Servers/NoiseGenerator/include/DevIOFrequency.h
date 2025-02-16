#ifndef _DEVIOFREQUENCY_H_
#define _DEVIOFREQUENCY_H_

/** **************************************************************************************************** */
/* IRA Istituto di Radioastronomia                                                                      */
/* $Id: DevIOFrequency.h,v 1.1 2011-03-14 15:16:22 a.orlati Exp $									           */
/*                                                                                                      */
/* This code is under GNU General Public Licence (GPL).                                                 */
/*                                                                                                      */
/* Who                                when            What                                              */
/* Andrea Orlati(aorlati@ira.inaf.it)  	18/11/2008     Creation                                         */


#include <baciDevIO.h>
#include <IRA>
#include "CommandLine.h"

using namespace IRA;

/**
 * This class is derived from template DevIO and it is used by the frequency property  of the TotalPower
 * component. 
 * @author <a href=mailto:a.orlati@ira.inaf.it>Andrea Orlati</a>,
 * Istituto di Radioastronomia, Italia<br> 
*/
class DevIOFrequency : public DevIO<ACS::doubleSeq>
{
public:

	/** 
	 * Constructor
	 * @param Link pointer to a SecureArea that proctects a the command line socket. This object must be already initialized and configured.
	*/
	DevIOFrequency(CCommandLine* Link) :  m_pLink(Link)
	{		
		AUTO_TRACE("DevIOFrequency::DevIOFrequency()");		
	}

	/**
	 * Destructor
	*/ 
	~DevIOFrequency()
	{
		ACS_TRACE("DevIOFrequency::~DevIOFrequency()");		
	}

	/** 
	 * @return true to initialize the property with default value from CDB.
	*/
	bool initializeValue()
	{		
		AUTO_TRACE("DevIOFrequency::DevIOFrequency()");		
		return false;
	}
	
	/**
	 * Used to read the property value.
	 * @throw ComponentErrors::PropertyErrorketError
	 * @param timestamp epoch when the operation completes
	*/ 
	ACS::doubleSeq read(ACS::Time& timestamp) throw (ACSErr::ACSbaseExImpl)
	{
		try {
			m_pLink->getFrequency(m_val);
		}
		catch (ACSErr::ACSbaseExImpl& E) {
			_ADD_BACKTRACE(ComponentErrors::PropertyErrorExImpl,dummy,E,"DevIOFrequency::read()");
			dummy.setPropertyName("frequency");
			dummy.setReason("Property could not be read");
			//_IRA_LOGGUARD_LOG_EXCEPTION(m_logGuard,dummy,LM_DEBUG);
			throw dummy;
		} 				
		timestamp=getTimeStamp();  //complition time
		return m_val;
	}
	/**
	 * It writes values into controller. Unused because the properties are read-only.
	*/ 
	void write(const ACS::doubleSeq& value, ACS::Time& timestamp) throw (ACSErr::ACSbaseExImpl)
	{
		timestamp=getTimeStamp();
		return;
	}
	
private:
	CCommandLine* m_pLink;
	ACS::doubleSeq m_val;
	//CLogGuard m_logGuard;
};


#endif /*_DEVIOFREQUENCY_H_*/
