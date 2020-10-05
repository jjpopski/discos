#include "ComponentCore.h"
#include <LogFilter.h>

_IRA_LOGFILTER_IMPORT;

// speed of light in meters per second
#define LIGHTSPEED 299792458.0

CComponentCore::CComponentCore()
{
}

CComponentCore::~CComponentCore()
{
}

void CComponentCore::initialize(maci::ContainerServices* services)
{
    m_services=services;
    m_control=NULL;
    
    m_lo_1st.m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
    m_lo_1st.m_localOscillatorFault=false;
    m_lo_1st.m_localOscillatorValue = 0.0;
    
    m_lo_2st.m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
    m_lo_2st.m_localOscillatorFault=false;
    m_lo_2st.m_localOscillatorValue = 0.0;

    m_environmentTemperature.temperature = 20.0;
    
    m_vacuum=0.0;
    m_calDiode=false;
    m_fetValues.VDL=m_fetValues.IDL=m_fetValues.VGL=m_fetValues.VDR=m_fetValues.IDR=m_fetValues.VGR=0.0;
    
    m_statusWord=0;
    m_ioMarkError = false;
}

CConfiguration const * const  CComponentCore::execute() throw (ComponentErrors::CDBAccessExImpl,ComponentErrors::MemoryAllocationExImpl,ComponentErrors::SocketErrorExImpl)
{
    /*
     * This Call sets default Recevier Configuration at CCC_Normal 
     * User has to call for Activate() and setupMode() to be more specific about receiver conf
     */
    m_configuration.init(m_services);  //throw (ComponentErrors::CDBAccessExImpl);
    try {
        m_control=new IRA::ReceiverControl(
                (const char *)m_configuration.getDewarIPAddress(),
                m_configuration.getDewarPort(),
                (const char *)m_configuration.getLNAIPAddress(),
                m_configuration.getLNAPort(),
                m_configuration.getLNASamplingTime(),
                m_configuration.getFeeds()
        );
    }
    catch (std::bad_alloc& ex) {
        _EXCPT(ComponentErrors::MemoryAllocationExImpl,dummy,"CComponentCore::execute()");
        throw dummy;
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ComponentErrors::SocketErrorExImpl,dummy,"CComponentCore::execute()");
        throw dummy;
    }
    m_setupMode= m_configuration.m_con_hnd.getActualConfStr();
    return &m_configuration;
}

void CComponentCore::cleanup()
{
    //make sure no one is using the object
    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_control) {
        m_control->closeConnection();
        delete m_control;
    }
}

void CComponentCore::activate(const char *mode) throw (ReceiversErrors::ModeErrorExImpl,ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl,
        ComponentErrors::CouldntGetComponentExImpl,ComponentErrors::CORBAProblemExImpl,ReceiversErrors::LocalOscillatorErrorExImpl,ReceiversErrors::NoRemoteControlErrorExImpl,
        ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    /* activate mode */
    baci::ThreadSyncGuard guard(&m_mutex);
    bool l_res= m_configuration.m_conf_hnd.setMode(mode);
    if (! l_res){
        _EXCPT(ReceiversErrors::ModeErrorExImpl,impl,"CComponentErrors::setMode()");
        throw impl;
    }    
    guard.release();
    //members initialization        
    m_startFreq.length(m_configuration.getIFs());
    m_bandwidth.length(m_configuration.getIFs());
    m_polarization.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        m_startFreq[i]=m_configuration.getIFMin()[i];
        m_bandwidth[i]=m_configuration.getIFBandwidth()[i];
        m_polarization[i]=(long)m_configuration.getPolarizations()[i];
        /** @todo comporre un oggetto LO con un unica interfaccia come clsse a parte ? */
        m_localOscillatorValue=m_configuration.getDefaultLO()[i];
    }
    // Basic operations
    lnaOn(); // throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
    externalCalOff();
    // Remote control check
    bool answer;
    try {
        answer=m_control->isRemoteOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::activate()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    if (answer) {
        _IRA_LOGFILTER_LOG(LM_NOTICE, "CComponentCore::activate()", "RECEIVER_COMMUNICATION_MODE_REMOTE");
        clearStatusBit(LOCAL);
    }
    else {
        _IRA_LOGFILTER_LOG(LM_NOTICE, "CComponentCore::activate()", "RECEIVER_COMMUNICATION_MODE_LOCAL");
        setStatusBit(LOCAL);
    }
}


void CComponentCore::deactivate() throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // no guard needed.
    lnaOff(); // throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
}


void CComponentCore::setMode(const char * mode) throw (ReceiversErrors::ModeErrorExImpl,ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl,
        ComponentErrors::CouldntGetComponentExImpl,ComponentErrors::CORBAProblemExImpl,ReceiversErrors::LocalOscillatorErrorExImpl)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    IRA::CString cmdMode(mode);
    cmdMode.MakeUpper();
    /**@todo CHC - CCC modes allowed? */
    
    /**@todo NarrowBandiwth vs NormalSetupMode ? */
    if (cmdMode!=m_configuration.getSetupMode()) { 
        _EXCPT(ReceiversErrors::ModeErrorExImpl,impl,"CComponentErrors::setMode()");
        throw impl;
    }
    // 
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        m_startFreq[i]=m_configuration.getIFMin()[i];
        m_bandwidth[i]=m_configuration.getIFBandwidth()[i];
        m_polarization[i]=(long)m_configuration.getPolarizations()[i];
    }
    // the set the default LO for the default LO for the selected mode.....
    ACS::doubleSeq lo;
    lo.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        lo[i]=m_configuration.getDefaultLO()[i];
    }
    setLO(lo); // throw (ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl,ComponentErrors::CouldntGetComponentExImpl,ComponentErrors::CORBAProblemExImpl,ReceiversErrors::LocalOscillatorErrorExImpl)
    m_setupMode=mode;
    m_calDiode=false;
    ACS_LOG(LM_FULL_INFO,"CComponentCore::setMode()",(LM_NOTICE,"RECEIVER_MODE %s",mode));
}

const IRA::CString& CComponentCore::getSetupMode()
{
    baci::ThreadSyncGuard guard(&m_mutex);
    return m_setupMode;
}

void CComponentCore::getLO(ACS::doubleSeq& lo)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    lo.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        lo[i]=m_localOscillatorValue;
    }
}

void CComponentCore::getBandwidth(ACS::doubleSeq& bw)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    bw.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        bw[i]=m_bandwidth[i];
    }
}

void CComponentCore::getStartFrequency(ACS::doubleSeq& sf)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    sf.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        sf[i]=m_startFreq[i];
    }
}

void CComponentCore::getPolarization(ACS::longSeq& pol)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    pol.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        pol[i]=m_polarization[i];
    }
}

const DWORD& CComponentCore::getIFs()
{
    baci::ThreadSyncGuard guard(&m_mutex);
    return m_configuration.getIFs();
}

const Management::TSystemStatus& CComponentCore::getComponentStatus()
{
    baci::ThreadSyncGuard guard(&m_mutex);
    return m_componentStatus;
}

const DWORD& CComponentCore::getFeeds()
{
    baci::ThreadSyncGuard guard(&m_mutex);
    return m_configuration.getFeeds();
}



void CComponentCore::externalCalOn() throw (
        ReceiversErrors::NoRemoteControlErrorExImpl,
        ComponentErrors::ValidationErrorExImpl,
        ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::externalCalOn()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    guard.release();
    // if (checkStatusBit(LOCAL)) {
    //     _EXCPT(ReceiversErrors::NoRemoteControlErrorExImpl,impl,"CComponentCore::externalCalOn()");
    //     throw impl;
    // }
    try {
        m_control->setExtCalibrationOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::externalCalOn()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    try {
        m_control->isExtCalibrationOn() ? setStatusBit(EXTNOISEMARK) : clearStatusBit(EXTNOISEMARK);
        clearStatusBit(CONNECTIONERROR); // The communication was ok so clear the CONNECTIONERROR bit
    }    
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::externalCalOn()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }

}


void CComponentCore::externalCalOff() throw (
        ReceiversErrors::NoRemoteControlErrorExImpl,
        ComponentErrors::ValidationErrorExImpl,
        ReceiversErrors::ReceiverControlBoardErrorExImpl
        )
{
    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::externalCalOff()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    guard.release();
    // if (checkStatusBit(LOCAL)) {
    //     _EXCPT(ReceiversErrors::NoRemoteControlErrorExImpl,impl,"CComponentCore::externalCalOff()");
    //     throw impl;
    // }
    try {
        m_control->setExtCalibrationOff();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::externalCalOff()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    try {
        m_control->isExtCalibrationOn() ? setStatusBit(EXTNOISEMARK) : clearStatusBit(EXTNOISEMARK);
        clearStatusBit(CONNECTIONERROR); // The communication was ok so clear the CONNECTIONERROR bit
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::externalCalOff()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
}




void CComponentCore::deactivate() throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // no guard needed.
    lnaOff(); // throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
}


void CComponentCore::setMode(const char * mode) throw (ReceiversErrors::ModeErrorExImpl,ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl,
        ComponentErrors::CouldntGetComponentExImpl,ComponentErrors::CORBAProblemExImpl,ReceiversErrors::LocalOscillatorErrorExImpl)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    IRA::CString cmdMode(mode);
    cmdMode.MakeUpper();
    if (cmdMode!=m_configuration.getSetupMode()) { // in this case i have just one allowed mode...so no need to do many checks and settings
        _EXCPT(ReceiversErrors::ModeErrorExImpl,impl,"CComponentErrors::setMode()");
        throw impl;
    }
    //
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        m_startFreq[i]=m_configuration.getIFMin()[i];
        m_bandwidth[i]=m_configuration.getIFBandwidth()[i];
        m_polarization[i]=(long)m_configuration.getPolarizations()[i];
    }
    // the set the default LO for the default LO for the selected mode.....
    ACS::doubleSeq lo;
    lo.length(m_configuration.getIFs());
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        lo[i]=m_configuration.getDefaultLO()[i];
    }
    setLO(lo); // throw (ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl,ComponentErrors::CouldntGetComponentExImpl,ComponentErrors::CORBAProblemExImpl,ReceiversErrors::LocalOscillatorErrorExImpl)
    m_setupMode=mode;
    m_calDiode=false;
    ACS_LOG(LM_FULL_INFO,"CComponentCore::setMode()",(LM_NOTICE,"RECEIVER_MODE %s",mode));
}

void CComponentCore::calOn() throw (ReceiversErrors::NoRemoteControlErrorExImpl,ComponentErrors::ValidationErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::calOn()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    // guard.release();
    // if (checkStatusBit(LOCAL)) {
    //     _EXCPT(ReceiversErrors::NoRemoteControlErrorExImpl,impl,"CComponentCore::calOn()");
    //     throw impl;
    // }
    try {
        m_control->setCalibrationOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::calOn()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    setStatusBit(NOISEMARK);
    m_calDiode=true;
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::calOff() throw (ReceiversErrors::NoRemoteControlErrorExImpl,ComponentErrors::ValidationErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::calOff()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    // guard.release();
    // if (checkStatusBit(LOCAL)) {
    //     _EXCPT(ReceiversErrors::NoRemoteControlErrorExImpl,impl,"CComponentCore::calOff()");
    //     throw impl;
    // }
    try {
        m_control->setCalibrationOff();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::calOff()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    clearStatusBit(NOISEMARK);
    m_calDiode=false;
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::lnaOff() throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // if (checkStatusBit(LOCAL)) {
    //     _EXCPT(ReceiversErrors::NoRemoteControlErrorExImpl,impl,"CComponentCore::lnaOff()");
    //     throw impl;
    // }
    try {
        m_control-> turnRightLNAsOff();
        m_control-> turnLeftLNAsOff();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::lnaOff()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::lnaOn() throw (ReceiversErrors::NoRemoteControlErrorExImpl,ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // if (checkStatusBit(LOCAL)) {
    //     _EXCPT(ReceiversErrors::NoRemoteControlErrorExImpl,impl,"CComponentCore::lnaOn()");
    //     throw impl;
    // }
    try {
        m_control-> turnRightLNAsOn();
        m_control-> turnLeftLNAsOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::lnaOn()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::setLO(const ACS::doubleSeq& lo) throw (ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl,
        ComponentErrors::CouldntGetComponentExImpl,ComponentErrors::CORBAProblemExImpl,ReceiversErrors::LocalOscillatorErrorExImpl)
{
    double trueValue,amp;
    double *freq=NULL;
    double *power=NULL;
    DWORD size;
    baci::ThreadSyncGuard guard(&m_mutex);
    if (lo.length()==0) {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::setLO");
        impl.setReason("at least one value must be provided");
        throw impl;
    }
    // in case -1 is given we keep the current value...so nothing to do
    if (lo[0]==-1) {
        ACS_LOG(LM_FULL_INFO,"CComponentCore::setLO()",(LM_NOTICE,"KEEP_CURRENT_LOCAL_OSCILLATOR %lf",m_localOscillatorValue));
        return;
    }
    // now check if the requested value match the limits
    if (lo[0]<m_configuration.getLOMin()[0]) {
        _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::setLO");
        impl.setValueName("local oscillator lower limit");
        impl.setValueLimit(m_configuration.getLOMin()[0]);
        throw impl;
    }
    else if (lo[0]>m_configuration.getLOMax()[0]) {
        _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::setLO");
        impl.setValueName("local oscillator upper limit");
        impl.setValueLimit(m_configuration.getLOMax()[0]);
        throw impl;
    }
    //computes the synthesizer settings
    trueValue=lo[0]+m_configuration.getFixedLO2()[0];
    size=m_configuration.getSynthesizerTable(freq,power);
    amp=round(linearFit(freq,power,size,trueValue));
    if (power) delete [] power;
    if (freq) delete [] freq;
    ACS_LOG(LM_FULL_INFO,"CComponentCore::setLO()",(LM_DEBUG,"SYNTHESIZER_VALUES %lf %lf",trueValue,amp));
    // make sure the synthesizer component is available
    loadLocalOscillator(); // throw (ComponentErrors::CouldntGetComponentExImpl)
    try {
        m_localOscillatorDevice->set(amp,trueValue);
    }
    catch (CORBA::SystemException& ex) {
        m_localOscillatorFault=true;
        _EXCPT(ComponentErrors::CORBAProblemExImpl,impl,"CComponentCore::setLO()");
        impl.setName(ex._name());
        impl.setMinor(ex.minor());
        throw impl;
    }
    catch (ReceiversErrors::ReceiversErrorsEx& ex) { 
        _ADD_BACKTRACE(ReceiversErrors::LocalOscillatorErrorExImpl,impl,ex,"CComponentCore::setLO()");
        throw impl;
    }
    // now that the local oscillator has been properly set...let's do some easy computations
    m_localOscillatorValue=lo[0];
    for (WORD i=0;i<m_configuration.getIFs();i++) {
        m_bandwidth[i]=m_configuration.getRFMax()[i]-(m_startFreq[i]+m_localOscillatorValue);
        // the if bandwidth could never be larger than the max IF bandwidth:
        if (m_bandwidth[i]>m_configuration.getIFBandwidth()[i]) m_bandwidth[i]=m_configuration.getIFBandwidth()[i];
    }
    ACS_LOG(LM_FULL_INFO,"CComponentCore::setLO()",(LM_NOTICE,"LOCAL_OSCILLATOR %lf",m_localOscillatorValue));
}

void CComponentCore::getCalibrationMark(ACS::doubleSeq& result,ACS::doubleSeq& resFreq,ACS::doubleSeq& resBw,const ACS::doubleSeq& freqs,const ACS::doubleSeq& bandwidths,const ACS::longSeq& feeds,
            const ACS::longSeq& ifs,bool& onoff,double &scaleFactor) throw (ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl)
{
    double realFreq,realBw;
    double *tableLeftFreq=NULL;
    double *tableLeftMark=NULL;
    double *tableRightFreq=NULL;
    double *tableRightMark=NULL;
    DWORD sizeL=0;
    DWORD sizeR=0;
    baci::ThreadSyncGuard guard(&m_mutex);
    /* Checking receiver setup */
    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::getCalibrationMark()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    /* Checking start frequnency input seq length*/
    unsigned stdLen=freqs.length();
    if ((stdLen!=bandwidths.length()) || (stdLen!=feeds.length()) || (stdLen!=ifs.length())) {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::getCalibrationMark()");
        impl.setReason("sub-bands definition is not consistent");
        throw impl;
    }
    /* For every start frequency checks IF input seq */
    for (unsigned i=0;i<stdLen;i++) {
        if ((ifs[i]>=(long)m_configuration.getIFs()) || (ifs[i]<0)) {
            _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::getCalibrationMark()");
            impl.setValueName("IF identifier");
            throw impl;
        }
    }
    /* For every start frequency checks Feeds input seq */
    for (unsigned i=0;i<stdLen;i++) {
        if ((feeds[i]>=(long)m_configuration.getFeeds()) || (feeds[i]<0)) {
            _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::getCalibrationMark()");
            impl.setValueName("feed identifier");
            throw impl;
        }
    }
    /* Setting output data containers lenght */
    result.length(stdLen);
    resFreq.length(stdLen);
    resBw.length(stdLen);
    /* Ask for polynomial coeffs from configuration layer */
    SetupParams l_receiver_setup;



    sizeL=m_configuration.getLeftMarkTable(tableLeftFreq,tableLeftMark);
    sizeR=m_configuration.getRightMarkTable(tableRightFreq,tableRightMark);
    for (unsigned i=0;i<stdLen;i++) {
        // now computes the mark for each input band....considering the present mode and configuration of the receiver.
        if (!IRA::CIRATools::skyFrequency(freqs[i],bandwidths[i],m_startFreq[ifs[i]],m_bandwidth[ifs[i]],realFreq,realBw)) {
                realFreq=m_startFreq[ifs[i]];
                realBw=0.0;
        }
        ACS_LOG(LM_FULL_INFO,"CComponentCore::getCalibrationMark()",(LM_DEBUG,"SUB_BAND %lf %lf",realFreq,realBw));
        realFreq+=m_localOscillatorValue;
        resFreq[i]=realFreq;
        resBw[i]=realBw;
        realFreq+=realBw/2.0;
        ACS_LOG(LM_FULL_INFO,"CComponentCore::getCalibrationMark()",(LM_DEBUG,"REFERENCE_FREQUENCY %lf",realFreq));
        if (m_polarization[ifs[i]]==(long)Receivers::RCV_LCP) {
            result[i]=linearFit(tableLeftFreq,tableLeftMark,sizeL,realFreq);
            ACS_LOG(LM_FULL_INFO,"CComponentCore::getCalibrationMark()",(LM_DEBUG,"LEFT_MARK_VALUE %lf",result[i]));
        }
        else { //RCV_RCP
            result[i]=linearFit(tableRightFreq,tableRightMark,sizeR,realFreq);
            ACS_LOG(LM_FULL_INFO,"CComponentCore::getCalibrationMark()",(LM_DEBUG,"RIGHT_MARK_VALUE %lf",result[i]));
        }
    }
    scaleFactor=1.0;
    onoff=m_calDiode;
    if (tableLeftFreq) delete [] tableLeftFreq;
    if (tableLeftMark) delete [] tableLeftMark;
    if (tableRightFreq) delete [] tableRightFreq;
    if (tableRightMark) delete [] tableRightMark;
}

void CComponentCore::getIFOutput(
        const ACS::longSeq& feeds,
        const ACS::longSeq& ifs,
        ACS::doubleSeq& freqs,
        ACS::doubleSeq& bw,
        ACS::longSeq& pols,
        ACS::doubleSeq& LO
        ) throw (ComponentErrors::ValidationErrorExImpl, ComponentErrors::ValueOutofRangeExImpl)
{

    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::getIFOutput()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    // let's do some checks about input data
    unsigned stdLen=feeds.length();
    if ((stdLen!=ifs.length())) {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::getIFOutput()");
        impl.setReason("sub-bands definition is not consistent");
        throw impl;
    }
    for (unsigned i=0;i<stdLen;i++) {
        if ((ifs[i]>=(long)m_configuration.getIFs()) || (ifs[i]<0)) {
            _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::getIFOutputMark()");
            impl.setValueName("IF identifier");
            throw impl;
        }
    }
    for (unsigned i=0;i<stdLen;i++) {
        if ((feeds[i]>=(long)m_configuration.getFeeds()) || (feeds[i]<0)) {
            _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::getIFOutput()");
            impl.setValueName("feed identifier");
            throw impl;
        }
    }
    freqs.length(stdLen);
    bw.length(stdLen);
    pols.length(stdLen);
    LO.length(stdLen);
    for (unsigned i=0;i<stdLen;i++) {
        freqs[i] = m_startFreq[ifs[i]];
        bw[i] = m_bandwidth[ifs[i]];
        pols[i] = m_polarization[ifs[i]];
        LO[i] = m_localOscillatorValue;
    }
}

double CComponentCore::getTaper(const double& freq,const double& bw,const long& feed,const long& ifNumber,double& waveLen) throw (
        ComponentErrors::ValidationErrorExImpl,ComponentErrors::ValueOutofRangeExImpl)
{
    double centralFreq;
    double taper;
    double realFreq,realBw;
    double *freqVec=NULL;
    double *taperVec=NULL;
    DWORD size;

    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_setupMode=="") {
        _EXCPT(ComponentErrors::ValidationErrorExImpl,impl,"CComponentCore::getTaper()");
        impl.setReason("receiver not configured yet");
        throw impl;
    }
    if ((ifNumber>=(long)m_configuration.getIFs()) || (ifNumber<0)) {
        _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::getTaper()");
        impl.setValueName("IF identifier");
        throw impl;
    }
    if ((feed>=(long)m_configuration.getFeeds()) || (feed<0)) {
        _EXCPT(ComponentErrors::ValueOutofRangeExImpl,impl,"CComponentCore::getTaper()");
        impl.setValueName("feed identifier");
        throw impl;
    }
    // take the real observed bandwidth....the correlation between detector device and the band provided by the receiver
    if (!IRA::CIRATools::skyFrequency(freq,bw,m_startFreq[ifNumber],m_bandwidth[ifNumber],realFreq,realBw)) {
        realFreq=m_startFreq[ifNumber];
        realBw=0.0;
    }
    centralFreq=realFreq+m_localOscillatorValue+realBw/2.0;
    ACS_LOG(LM_FULL_INFO,"CComponentCore::getTaper()",(LM_DEBUG,"CENTRAL_FREQUENCY %lf",centralFreq));
    waveLen=LIGHTSPEED/(centralFreq*1000000);
    ACS_LOG(LM_FULL_INFO,"CComponentCore::getTaper()",(LM_DEBUG,"WAVELENGTH %lf",waveLen));
    size=m_configuration.getTaperTable(freqVec,taperVec);
    taper=linearFit(freqVec,taperVec,size,centralFreq);
    ACS_LOG(LM_FULL_INFO,"CComponentCore::getTaper()",(LM_DEBUG,"TAPER %lf",taper));
    if (freqVec) delete [] freqVec;
    if (taperVec) delete [] taperVec;
    return taper;
}

long CComponentCore::getFeeds(ACS::doubleSeq& X,ACS::doubleSeq& Y,ACS::doubleSeq& power)
{
    DWORD size;
    WORD *code;
    double *xOffset;
    double *yOffset;
    double *rPower;
    baci::ThreadSyncGuard guard(&m_mutex);
    size=m_configuration.getFeedInfo(code,xOffset,yOffset,rPower);
    X.length(size);
    Y.length(size);
    power.length(size);
    for (DWORD j=0;j<size;j++) {
        X[j]=xOffset[j];
        Y[j]=yOffset[j];
        power[j]=rPower[j];
    }
    if (xOffset) delete [] xOffset;
    if (yOffset) delete [] yOffset;
    if (rPower) delete [] rPower;
    return size;
}

double CComponentCore::getFetValue(const IRA::ReceiverControl::FetValue& control,const DWORD& ifs)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    if (ifs>=m_configuration.getIFs()) {
        return 0.0;
    }
    else if (m_polarization[ifs]==(long)Receivers::RCV_LCP) {
        if (control==IRA::ReceiverControl::DRAIN_VOLTAGE) return m_fetValues.VDL;
        else if (control==IRA::ReceiverControl::DRAIN_CURRENT) return m_fetValues.IDL;
        else return m_fetValues.VGL;
    }
    else { //RCV_RCP
        if (control==IRA::ReceiverControl::DRAIN_VOLTAGE) return m_fetValues.VDR;
        else if (control==IRA::ReceiverControl::DRAIN_CURRENT) return m_fetValues.IDR;
        else return m_fetValues.VGR;
    }
}

void CComponentCore::checkLocalOscillator() throw (ComponentErrors::CORBAProblemExImpl,ComponentErrors::CouldntGetAttributeExImpl)
{
    baci::ThreadSyncGuard guard(&m_mutex);
    if (m_setupMode=="") { // if the receiver is not configured the check makes no sense
        return;
    }
    // make sure the synthesizer component is available
    loadLocalOscillator(); // throw (ComponentErrors::CouldntGetComponentExImpl)
    ACSErr::Completion_var comp;
    ACS::ROlong_var isLockedRef;
    CORBA::Long isLocked;
    try {
        isLockedRef=m_localOscillatorDevice->isLocked();
    }
    catch (CORBA::SystemException& ex) {
        m_localOscillatorFault=true;
        _EXCPT(ComponentErrors::CORBAProblemExImpl,impl,"CComponentCore::checkLocalOscillator()");
        impl.setName(ex._name());
        impl.setMinor(ex.minor());
        throw impl;
    }
    isLocked=isLockedRef->get_sync(comp.out());
    ACSErr::CompletionImpl complImpl(comp);
    if (!complImpl.isErrorFree()) {
        _ADD_BACKTRACE(ComponentErrors::CouldntGetAttributeExImpl,impl,complImpl,"CComponentCore::checkLocalOscillator()");
        impl.setAttributeName("isLocked");
        impl.setComponentName((const char *)m_configuration.getLocalOscillatorInstance());
        throw impl;
    }
    if (!isLocked) setStatusBit(UNLOCKED);
    else clearStatusBit(UNLOCKED);
}

void CComponentCore::updateComponent()
{
    baci::ThreadSyncGuard guard(&m_mutex);
    m_componentStatus=Management::MNG_OK;
    // if (checkStatusBit(LOCAL)) {
    //     setComponentStatus(Management::MNG_FAILURE);
    //     _IRA_LOGFILTER_LOG(LM_CRITICAL,"CComponentCore::updateComponent()","RECEIVER_NOT_REMOTELY_CONTROLLABLE");
    // }
    if (checkStatusBit(VACUUMPUMPFAULT)) {
        setComponentStatus(Management::MNG_WARNING);
        _IRA_LOGFILTER_LOG(LM_WARNING,"CComponentCore::updateComponent()","VACUUM_PUMP_FAILURE");
    }
    if (checkStatusBit(NOISEMARKERROR)) {
        setComponentStatus(Management::MNG_FAILURE);
        _IRA_LOGFILTER_LOG(LM_CRITICAL,"CComponentCore::updateComponent()","NOISE_MARK_ERROR");
    }
    if (checkStatusBit(CONNECTIONERROR)) {
        setComponentStatus(Management::MNG_FAILURE);
        _IRA_LOGFILTER_LOG(LM_CRITICAL,"CComponentCore::updateComponent()","RECEIVER_CONNECTION_ERROR");
    }
    if (checkStatusBit(UNLOCKED)) {
        setComponentStatus(Management::MNG_FAILURE);
        _IRA_LOGFILTER_LOG(LM_CRITICAL,"CComponentCore::updateComponent()","LOCAL_OSCILLATOR_NOT_LOCKED");
    }
}

void CComponentCore::updateVacuum() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    bool vacuumSensor;
    try {
        vacuumSensor=m_control->isVacuumSensorOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateVacuum()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    if (vacuumSensor) {
        try {
            m_vacuum=m_control->vacuum(CComponentCore::voltage2mbar);
        }
        catch (IRA::ReceiverControlEx& ex) {
            _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateVacuum()");
            impl.setDetails(ex.what().c_str());
            setStatusBit(CONNECTIONERROR);
            throw impl;
        }
    }
    else {
        m_vacuum=m_vacuumDefault;
    }
    if (!vacuumSensor) setStatusBit(VACUUMSENSOR);
    else clearStatusBit(VACUUMSENSOR);
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::updateVacuumPump() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    bool answer;
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        answer=m_control->isVacuumPumpOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateVacuumPump()");
        impl.setDetails(ex.what().c_str());
        baci::ThreadSyncGuard guard(&m_mutex);
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    if (!answer) setStatusBit(VACUUMPUMPSTATUS);
    else clearStatusBit(VACUUMPUMPSTATUS);
    //**********************************************************************************/
    // VACUUM PUMP FAULT MISSING (VACUUMPUMPFAULT)
    //************************************************************************************
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::updateNoiseMark() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    bool answer;
    baci::ThreadSyncGuard guard(&m_mutex);
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        answer=m_control->isCalibrationOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateNoiseMark()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    if(answer!=checkStatusBit(NOISEMARK)) {
        if(m_ioMarkError) {
            setStatusBit(NOISEMARKERROR);
        }
        else {
            m_ioMarkError = true;
        }
    }
    else {
        clearStatusBit(NOISEMARKERROR);
        m_ioMarkError = false;
    }
    //*********************************************************************************************/
    // EXTNOISEMARK is missing
    /**********************************************************************************************/
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::updateVacuumValve() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    bool answer;
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        answer=m_control->isVacuumValveOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateVacuumValve()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    if (!answer) setStatusBit(VACUUMVALVEOPEN);
    else clearStatusBit(VACUUMVALVEOPEN);
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::updateIsRemote() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    bool answer;
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        answer=m_control->isRemoteOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateIsRemote()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }

    if (checkStatusBit(LOCAL) && answer) {
        _IRA_LOGFILTER_LOG(
            LM_NOTICE,
            "CComponentCore::updateIsRemote()",
            "RECEIVER_SWITCHED_FROM_LOCAL_TO_REMOTE"
        );
    }
    else if (!checkStatusBit(LOCAL) && !answer) {
        _IRA_LOGFILTER_LOG(
            LM_NOTICE,
            "CComponentCore::updateIsRemote()",
            "RECEIVER_SWITCHED_FROM_REMOTE_TO_LOCAL"
        );
    }

    if (!answer) setStatusBit(LOCAL);
    else clearStatusBit(LOCAL);
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::updateCoolHead()  throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    bool answer;
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        answer=m_control->isCoolHeadOn();
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateCoolHead()->isCoolHeadOn()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    if (!answer) {
        setStatusBit(COOLHEADON);
        try {
            answer=m_control->isCoolHeadSetOn();
        }
        catch (IRA::ReceiverControlEx& ex) {
            _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateCoolHead() - isCoolHeadSetOn()");
            impl.setDetails(ex.what().c_str());
            setStatusBit(CONNECTIONERROR);
            throw impl;
        }
        if(!answer)
            setStatusBit(COMPRESSORFAULT);
        else
            clearStatusBit(COMPRESSORFAULT);
    }
    else 
        clearStatusBit(COOLHEADON);

    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::updateEnvironmentTemperature() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        m_environmentTemperature.temperature = m_control->vertexTemperature(CComponentCore::voltage2Celsius);
        m_environmentTemperature.timestamp = getTimeStamp();
    }
    catch (IRA::ReceiverControlEx& ex) {
        m_environmentTemperature.temperature = CEDUMMY;
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateEnvironmentTemperature()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}


void CComponentCore::updateLNAControls() throw (ReceiversErrors::ReceiverControlBoardErrorExImpl)
{
    // not under the mutex protection because the m_control object is thread safe (at the micro controller board stage)
    try {
        m_fetValues=m_control->fetValues(0,1,CComponentCore::currentConverter, CComponentCore::voltageConverter);
    }
    catch (IRA::ReceiverControlEx& ex) {
        _EXCPT(ReceiversErrors::ReceiverControlBoardErrorExImpl,impl,"CComponentCore::updateCryoLNAWin()");
        impl.setDetails(ex.what().c_str());
        setStatusBit(CONNECTIONERROR);
        throw impl;
    }
    clearStatusBit(CONNECTIONERROR); // the communication was ok so clear the CONNECTIONERROR bit
}

void CComponentCore::loadLocalOscillator() throw (ComponentErrors::CouldntGetComponentExImpl)
{
    if ((!CORBA::is_nil(m_localOscillatorDevice)) && (m_localOscillatorFault)) { // if reference was already taken, but an error was found....dispose the reference
        try {
            m_services->releaseComponent((const char*)m_localOscillatorDevice->name());
        }
        catch (...) { //dispose silently...if an error...no matter
        }
        m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
    }
    if (CORBA::is_nil(m_localOscillatorDevice)) {  //only if it has not been retrieved yet
        try {
            m_localOscillatorDevice=m_services->getComponent<Receivers::LocalOscillator>((const char*)m_configuration.getLocalOscillatorInstance());
            ACS_LOG(LM_FULL_INFO,"CCore::loadLocalOscillator()",(LM_INFO,"LOCAL_OSCILLATOR_OBTAINED"));
            m_localOscillatorFault=false;
        }
        catch (maciErrType::CannotGetComponentExImpl& ex) {
            _ADD_BACKTRACE(ComponentErrors::CouldntGetComponentExImpl,Impl,ex,"CComponentCore::loadLocalOscillator()");
            Impl.setComponentName((const char*)m_configuration.getLocalOscillatorInstance());
            m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
            throw Impl;
        }
        catch (maciErrType::NoPermissionExImpl& ex) {
            _ADD_BACKTRACE(ComponentErrors::CouldntGetComponentExImpl,Impl,ex,"CComponentCore::loadLocalOscillator()");
            Impl.setComponentName((const char*)m_configuration.getLocalOscillatorInstance());
            m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
            throw Impl;
        }
        catch (maciErrType::NoDefaultComponentExImpl& ex) {
            _ADD_BACKTRACE(ComponentErrors::CouldntGetComponentExImpl,Impl,ex,"CComponentCore::loadLocalOscillator()");
            Impl.setComponentName((const char*)m_configuration.getLocalOscillatorInstance());
            m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
            throw Impl;
        }
    }
}

void CComponentCore::unloadLocalOscillator()
{
    if (!CORBA::is_nil(m_localOscillatorDevice)) {
        try {
            m_services->releaseComponent((const char*)m_localOscillatorDevice->name());
        }
        catch (maciErrType::CannotReleaseComponentExImpl& ex) {
            _ADD_BACKTRACE(ComponentErrors::CouldntReleaseComponentExImpl,Impl,ex,"CComponentCore::unloadLocalOscillator()");
            Impl.setComponentName((const char *)m_configuration.getLocalOscillatorInstance());
            Impl.log(LM_WARNING);
        }
        catch (...) {
            _EXCPT(ComponentErrors::UnexpectedExImpl,impl,"CComponentCore::unloadLocalOscillator()");
            impl.log(LM_WARNING);
        }
        m_localOscillatorDevice=Receivers::LocalOscillator::_nil();
    }
}

double CComponentCore::linearFit(double *X,double *Y,const WORD& size,double x)
{
    int low=-1,high=-1;
    for (WORD j=0;j<size;j++) {
        if (x==X[j]) {
            return Y[j];
        }
        else if (x<X[j]) {
            if (high==-1) high=j;
            else if (X[j]<X[high]) high=j;
        }
        else if (x>X[j]) { // X value is lower
            if (low==-1) low=j;
            else if (X[j]>X[low]) low=j;
        }
    }
    if ((high!=-1) && (low!=-1)) {
        double slope=X[low]-X[high];
        return ((x-X[high])/slope)*Y[low]-((x-X[low])/slope)*Y[high];
    }
    else if (high==-1) {
        return Y[low];
    }
    else if (low==-1) {
        return Y[high];
    }
    else return 0.0; //this will never happen if size!=0
}
