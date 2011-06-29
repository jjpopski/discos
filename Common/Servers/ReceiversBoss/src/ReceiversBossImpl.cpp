// $Id: ReceiversBossImpl.cpp,v 1.8 2011-02-23 08:37:04 a.orlati Exp $

#include "ReceiversBossImpl.h"
#include <ComponentErrors.h>
#include <ManagementErrors.h>
#include "DevIOLO.h"
#include "DevIORecvCode.h"
#include "DevIOStatus.h"
#include "DevIOFeeds.h"
#include "DevIOIFs.h"
#include "DevIOPolarization.h"
#include "DevIOBandWidth.h"
 #include "DevIOInitialFrequency.h"

static char *rcsId="@(#) $Id: ReceiversBossImpl.cpp,v 1.8 2011-02-23 08:37:04 a.orlati Exp $";
static void *use_rcsId = ((void)&use_rcsId,(void *) &rcsId);

using namespace SimpleParser;

ReceiversBossImpl::ReceiversBossImpl(const ACE_CString &CompName,maci::ContainerServices *containerServices) : 
	CharacteristicComponentImpl(CompName,containerServices),
	m_plocalOscillator(this),
	m_pactualSetup(this),
	m_pfeeds(this),
	m_pIFs(this),
	m_pinitialFrequency(this),
	m_pbandWidth(this),
	m_ppolarization(this),
	m_pstatus(this)
{	
	AUTO_TRACE("ReceiversBossImpl::ReceiversBossImpl()");
	m_core=NULL;
}

ReceiversBossImpl::~ReceiversBossImpl()
{
	AUTO_TRACE("ReceiversBossImpl::~ReceiversBossImpl()");
}

void ReceiversBossImpl::initialize() throw (ACSErr::ACSbaseExImpl)
{
	AUTO_TRACE("ReceiversBossImpl::initialize()");
	ACS_LOG(LM_FULL_INFO,"ReceiversBossImpl::initialize()",(LM_INFO,"ReceiversBossImpl::COMPSTATE_INITIALIZING"));
	try {
		m_core=new CRecvBossCore();
		m_plocalOscillator=new ROdoubleSeq(getContainerServices()->getName()+":LO",getComponent(),new DevIOLO(m_core),true);
		m_pactualSetup=new ROstring(getContainerServices()->getName()+":actualSetup",getComponent(),new DevIORecvCode(m_core),true);
		m_pstatus=new ROEnumImpl<ACS_ENUM_T(Management::TSystemStatus),POA_Management::ROTSystemStatus> (getContainerServices()->getName()+":status",getComponent(),new DevIOStatus(m_core),true);
		m_pfeeds=new ROlong(getContainerServices()->getName()+":feeds",getComponent(),new DevIOFeeds(m_core),true);
		m_pIFs=new ROlong(getContainerServices()->getName()+":IFs",getComponent(),new DevIOIFs(m_core),true);
		m_ppolarization=new ROlongSeq(getContainerServices()->getName()+":polarization",getComponent(),new DevIOPolarization(m_core),true);
		
		m_pinitialFrequency=new ROdoubleSeq(getContainerServices()->getName()+":initialFrequency",getComponent(),new DevIOInitialFrequency(m_core),true);
		m_pbandWidth=new ROdoubleSeq(getContainerServices()->getName()+":bandWidth",getComponent(),new DevIOBandWidth(m_core),true);
		
		// create the parser for command line execution
		m_parser= new SimpleParser::CParser<CRecvBossCore>(m_core,10);
	}
	catch (std::bad_alloc& ex) {
		_EXCPT(ComponentErrors::MemoryAllocationExImpl,dummy,"ReceiversBossImpl::initialize()");
		throw dummy;
	}
	m_core->initialize(getContainerServices());
	m_parser->add<0>("receiversPark",new function0<CRecvBossCore,non_constant,void_type >(m_core,&CRecvBossCore::park));
	m_parser->add<1>("receiversSetup",new function1<CRecvBossCore,non_constant,void_type,I<string_type> >(m_core,&CRecvBossCore::setup));
	m_parser->add<1>("receiversMode",new function1<CRecvBossCore,non_constant,void_type,I<string_type> >(m_core,&CRecvBossCore::setMode));
	m_parser->add<0>("calOn",new function0<CRecvBossCore,non_constant,void_type >(m_core,&CRecvBossCore::calOn));
	m_parser->add<0>("calOff",new function0<CRecvBossCore,non_constant,void_type >(m_core,&CRecvBossCore::calOff));
	m_parser->add<1>("setLO",new function1<CRecvBossCore,non_constant,void_type,I<doubleSeq_type> >(m_core,&CRecvBossCore::setLO));	
	ACS_LOG(LM_FULL_INFO,"ReceiversBossImpl::initialize()",(LM_INFO,"COMPSTATE_INITIALIZED"));
}

void ReceiversBossImpl::execute() throw (ACSErr::ACSbaseExImpl)
{
	AUTO_TRACE("ReceiversBossImpl::execute()");
	m_core->execute(); //could throw exceptions
	try {
		startPropertiesMonitoring();
	}
	catch (acsthreadErrType::CanNotStartThreadExImpl& E) {
		_ADD_BACKTRACE(ComponentErrors::ThreadErrorExImpl,__dummy,E,"ReceiversBossImpl::execute()");
		throw __dummy;
	}
	catch (ACSErrTypeCommon::NullPointerExImpl& E) {
		_ADD_BACKTRACE(ComponentErrors::ThreadErrorExImpl,__dummy,E,"ReceiversBossImpl::execute()");
		throw __dummy;		
	}
	ACS_LOG(LM_FULL_INFO,"ReceiversBossImpl::execute()",(LM_INFO,"COMPSTATE_OPERATIONAL"));
}

void ReceiversBossImpl::cleanUp()
{
	AUTO_TRACE("ReceiversBossImpl::cleanUp()");
	stopPropertiesMonitoring();
	if (m_parser) delete m_parser;
	m_core->cleanUp();
	if (m_core) delete m_core;
	CharacteristicComponentImpl::cleanUp();	
}

void ReceiversBossImpl::aboutToAbort()
{
	AUTO_TRACE("ReceiversBossImpl::aboutToAbort()");
	if (m_parser) delete m_parser;
	m_core->cleanUp();
	if (m_core) delete m_core;
}

void ReceiversBossImpl::calOn() throw (CORBA::SystemException,ComponentErrors::ComponentErrorsEx)
{	
	try {
		m_core->calOn();
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::calOn()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}
}

void ReceiversBossImpl::calOff() throw (CORBA::SystemException,ComponentErrors::ComponentErrorsEx)
{
	try {
		m_core->calOff();
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::calOff()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}
}

void ReceiversBossImpl::setLO(const ACS::doubleSeq& lo) throw (CORBA::SystemException,ComponentErrors::ComponentErrorsEx)
{
	try {
		m_core->setLO(lo);
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::setLO()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}
}

void ReceiversBossImpl::setup(const char * code) throw (CORBA::SystemException,ManagementErrors::ConfigurationErrorEx)
{
	try {
		m_core->setup(code);
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::setup()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}	
}

void ReceiversBossImpl::setMode(const char * mode) throw (CORBA::SystemException,ComponentErrors::ComponentErrorsEx,ManagementErrors::ConfigurationErrorEx)
{
	try {
		m_core->setMode(mode); 
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();
	}
	catch (ManagementErrors::ConfigurationErrorExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getConfigurationErrorEx();		
	}
}

void ReceiversBossImpl::park() throw (CORBA::SystemException,ManagementErrors::ParkingErrorEx)
{
	m_core->park();
}

ACS::doubleSeq *ReceiversBossImpl::getCalibrationMark(const ACS::doubleSeq& freqs, const ACS::doubleSeq& bandwidths, const ACS::longSeq& feeds,const ACS::longSeq& ifs) throw (CORBA::SystemException,
	ComponentErrors::ComponentErrorsEx)
{
	ACS::doubleSeq_var result=new ACS::doubleSeq;
	try {
		m_core->getCalibrationMark(result,freqs,bandwidths,feeds,ifs);
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::getCalibrationMark()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}
	return result._retn();
}

CORBA::Long ReceiversBossImpl::getFeeds(ACS::doubleSeq_out X,ACS::doubleSeq_out Y,ACS::doubleSeq_out power) throw (CORBA::SystemException,ComponentErrors::ComponentErrorsEx)
{
	ACS::doubleSeq_var tempX=new ACS::doubleSeq;
	ACS::doubleSeq_var tempY=new ACS::doubleSeq;
	ACS::doubleSeq_var tempPower=new ACS::doubleSeq;
	long res;
	try {
		res=m_core->getFeeds(tempX,tempY,tempPower);
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::getFeeds()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}
	X=tempX._retn();
	Y=tempY._retn();
	power=tempPower._retn();
	return res;
}

CORBA::Double ReceiversBossImpl::getTaper(CORBA::Double freq,CORBA::Double bandWidth,CORBA::Long feed,CORBA::Long ifNumber,CORBA::Double_out waveLen) throw (CORBA::SystemException,ComponentErrors::ComponentErrorsEx)
{
	CORBA::Double res;
	double wL;
	try {
		res=(CORBA::Double)m_core->getTaper(freq,bandWidth,feed,ifNumber,wL);
		waveLen=wL;
		return res;
	}
	catch (ComponentErrors::ComponentErrorsExImpl& ex) {
		ex.log(LM_DEBUG);
		throw ex.getComponentErrorsEx();		
	}
	catch (...) {
		_EXCPT(ComponentErrors::UnexpectedExImpl,impl,"ReceiversBossImpl::getTaper()");
		impl.log(LM_DEBUG);
		throw impl.getComponentErrorsEx();
	}	
}

char *ReceiversBossImpl::command(const char *cmd) throw (CORBA::SystemException,ManagementErrors::CommandLineErrorEx)
{
	AUTO_TRACE("ReceiversBossImpl::command()");
	IRA::CString out;
	IRA::CString in;
	in=IRA::CString(cmd);
	try {
		m_parser->run(in,out);
	}
	catch (ParserErrors::ParserErrorsExImpl &ex) {
		_ADD_BACKTRACE(ManagementErrors::CommandLineErrorExImpl,impl,ex,"ReceiversBossImpl::command()");
		impl.setCommand(cmd);
		impl.setErrorMessage((const char *)out);
		impl.log(LM_DEBUG);
		throw impl.getCommandLineErrorEx();
	}
	return CORBA::string_dup((const char *)out);	
}


_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROdoubleSeq,m_plocalOscillator,LO);
_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROstring,m_pactualSetup,actualSetup);
_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,Management::ROTSystemStatus,m_pstatus,status);
_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROlongSeq,m_ppolarization,polarization);
_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROlong,m_pfeeds,feeds);
_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROlong,m_pIFs,IFs);

_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROdoubleSeq,m_pbandWidth,bandWidth);
_PROPERTY_REFERENCE_CPP(ReceiversBossImpl,ACS::ROdoubleSeq,m_pinitialFrequency,initialFrequency);


/* --------------- [ MACI DLL support functions ] -----------------*/
#include <maciACSComponentDefines.h>
MACI_DLL_SUPPORT_FUNCTIONS(ReceiversBossImpl)
