// $Id: DataCollection.cpp,v 1.9 2011-04-22 18:51:48 a.orlati Exp $

#include "DataCollection.h"
#include "FitsTools.h"
#include <SecureArea.h>
#include <libgen.h>
#include <ReceiversModule.h>

void FitsWriter_private::getTsysFromBuffer(char *& buffer,const DWORD& channels ,double *tsys) {
	for (DWORD i=0;i<channels;i++) {
		tsys[i]=*((double *)buffer);
		buffer+=sizeof(double);
	}
}

using namespace FitsWriter_private;

CDataCollection::CDataCollection()
{
	m_running=m_ready=m_start=m_stop=false;
	m_scanHeader=m_subScanHeader=false;
	m_reset=false;
	m_writeSummary=false;
	m_sectionH=NULL;
	m_fileName=m_summaryFileName=m_fullPath="";
	m_project="";
	m_observer="";
	m_backendName="";
	m_scheduleName="";
	m_logName="";
	m_status=Management::MNG_OK;
	m_siteName="";
	m_pressure=m_temperature=m_humidity=0.0;
	m_scanTag=-1;
	m_deviceID=0;
	m_scanAxis=Management::MNG_NO_AXIS;
	m_startUTTime=0; // initialized to..."no time"
	m_stopUTTime=0;
	m_fakeUTTime=0;
	m_scanID=m_subScanID=0;
	m_telecopeTacking=m_prevTelescopeTracking=false;
	m_telescopeTrackingTime=0;
}
	
CDataCollection::~CDataCollection()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	if (m_sectionH!=NULL) delete[] m_sectionH;
}

void CDataCollection::forceReset()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	if 	(!m_dumpCollection.isEmpty()) {
		m_dumpCollection.flushAll();
		ACS_LOG(LM_FULL_INFO, "CDataCollection::forceReset()",(LM_WARNING,"POSSIBLE_LOSS_OF_DATA"));
	}
	m_running=m_ready=m_start=m_stop=false;
	m_scanHeader=m_subScanHeader=false;
	m_writeSummary=false;
	m_reset=true;
	m_status=Management::MNG_OK;
	m_stopUTTime=m_startUTTime=m_fakeUTTime=0;
	IRA::CIRATools::Wait(200000); // 200 milliseconds...give some time to the Engine Thread to settle down
}

void CDataCollection::saveMainHeaders(Backends::TMainHeader const * h,Backends::TSectionHeader const * ch)
{
	baci::ThreadSyncGuard guard(&m_mutex);
	memcpy(&m_mainH,h,sizeof(Backends::TMainHeader));
	//cout << "Ricevuto backend header " << m_mainH.integration << endl;
	if (m_sectionH!=NULL) delete[] m_sectionH;
	m_sectionH=new Backends::TSectionHeader[m_mainH.sections];
	memcpy(m_sectionH,ch,sizeof(Backends::TSectionHeader)*m_mainH.sections);

	/*printf("sections: %ld\n",(long)m_mainH.sections);
	printf("beams: %ld\n",(long)m_mainH.beams);
	printf("integration: %ld\n",(long)m_mainH.integration);
	printf("sampleSize: %ld\n",(long)m_mainH.sampleSize);

	for (long j=0;j<m_mainH.sections;j++) {
		printf("id: %ld\n",(long)m_sectionH[j].id);
		printf("bins: %ld\n",(long)m_sectionH[j].bins);
		printf("polarization: %d\n",m_sectionH[j].polarization);
		printf("bandWidth: %lf\n",m_sectionH[j].bandWidth);
		printf("frequency: %lf\n",m_sectionH[j].frequency);
		printf("attenuation: %lf, %lf\n",m_sectionH[j].attenuation[0],m_sectionH[j].attenuation[1]);
		printf("sampleRate: %lf\n",m_sectionH[j].sampleRate);
		printf("feed: %ld\n",(long)m_sectionH[j].feed);
		printf("inputs: %ld\n",(long)m_sectionH[j].inputs);
		printf("IF: %ld %ld\n",(long)m_sectionH[j].IF[0],(long)m_sectionH[j].IF[1]);
	}*/

	m_ready=true; // main headers are saved....before that no activity can take place	
	//if (getIsNoData()) {
	//	m_start=true;
	//}
}

bool CDataCollection::saveDump(char * memory)
{
	bool track;
	char *buffer;
	baci::ThreadSyncGuard guard(&m_mutex);
	Backends::TDumpHeader *dh=(Backends::TDumpHeader *) memory;
	buffer=memory+sizeof(Backends::TDumpHeader); 
	if (dh->time>=m_telescopeTrackingTime) track=m_telecopeTacking;
	else track=m_prevTelescopeTracking;
	if ((!m_running) && (!m_start) && (!m_stop)) {
		m_start=true;
	}
	return m_dumpCollection.pushDump(dh->time,dh->calOn,memory,buffer,track,dh->dumpSize);
}

ACS::Time CDataCollection::getFirstDumpTime()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	return m_dumpCollection.getFirstTime();
}

long CDataCollection::getInputsNumber()
{
	long sum=0;
	baci::ThreadSyncGuard guard(&m_mutex);
	for (long i=0;i<m_mainH.sections;i++) {
		sum+=m_sectionH[i].inputs;
	}
	return sum;
}

IRA::CString CDataCollection::getSubScanType() const
{
	if (m_scanAxis==Management::MNG_HOR_LON) {
		return "AZ";
	}
	else if (m_scanAxis==Management::MNG_HOR_LAT) {
		return "EL";
	}
	else if (m_scanAxis==Management::MNG_EQ_LON) {
		return "RA";
	}
	else if (m_scanAxis==Management::MNG_EQ_LAT) {
		return "DEC";
	}
	else if (m_scanAxis==Management::MNG_GAL_LON) {
		return "GLON";
	}
	else if (m_scanAxis==Management::MNG_GAL_LAT) {
		return "GLAT";
	}
	else if (m_scanAxis==Management::MNG_BEAMPARK) {
		return "BEAMPARK";
	}
	else if (m_scanAxis==Management::MNG_TRACK) {
		return "TRACKING";
	}
	else if (m_scanAxis==Management::MNG_GCIRCLE) {
		return "GCIRCLE";
	}
	else if (m_scanAxis==Management::MNG_PFP_Y) {
		return "PFP_Y";
	}
	else if (m_scanAxis==Management::MNG_PFP_Z) {
		return "FOCUSING";
	}
	else if (m_scanAxis==Management::MNG_SUBR_Z) {
		return "FOCUSING";
	}
	else if (m_scanAxis==Management::MNG_SUBR_X) {
		return "SUBR_X";
	}
	else if (m_scanAxis==Management::MNG_SUBR_Y) {
		return "SUB_Y";
	}
	else if (m_scanAxis==Management::MNG_NO_AXIS) {
		return "UNKNOWN";
	}
	else {
		return "UNKNOWN";
	}
}

void CDataCollection::getSectionSkyFrequency(ACS::doubleSeq& outFreq,const ACS::doubleSeq& skyFreqs) const
{
	outFreq.length(m_mainH.sections);
	long inputs=0;
	long maxlen=skyFreqs.length();
	for(int i=0;i<m_mainH.sections;i++) {
		if (inputs<maxlen) outFreq[i]=skyFreqs[inputs];
		else outFreq[i]=DOUBLE_DUMMY_VALUE;
		inputs+=m_sectionH[i].inputs;
	}
} 

void CDataCollection::getSectionTypeAndPols(std::list<IRA::CString>& outPols,const ACS::longSeq& pols) const
{
	//outFreq.length(m_mainH.sections);
	long inputs=0;
	long maxlen=pols.length();
	Receivers::TPolarization pol;
	for(int i=0;i<m_mainH.sections;i++) {
		if (m_sectionH[i].polarization==Backends::BKND_FULL_STOKES) {
			outPols.push_back("stokes");
		}
		else if (inputs<maxlen) {
			pol=(Receivers::TPolarization)pols[inputs];
			outPols.push_back(Receivers::Definitions::map(pol));
		}
		else outPols.push_back(STRING_DUMMY_VALUE);
		inputs+=m_sectionH[i].inputs;
	}
}
void CDataCollection::getInputsConfiguration(ACS::longSeq& sectionID,ACS::longSeq& feeds,ACS::longSeq& ifs,ACS::doubleSeq& freqs,ACS::doubleSeq& bws,ACS::doubleSeq& atts)
{
	long inputs=0;
	baci::ThreadSyncGuard guard(&m_mutex);
	long inputsNumber=getInputsNumber();
	freqs.length(inputsNumber);
	bws.length(inputsNumber);
	feeds.length(inputsNumber);
	ifs.length(inputsNumber);
	atts.length(inputsNumber);
	sectionID.length(inputsNumber);
	for(int i=0;i<m_mainH.sections;i++) {
		if (m_sectionH[i].inputs==1) {
			ifs[inputs]=m_sectionH[i].IF[0];
			atts[inputs]=m_sectionH[i].attenuation[0];
			feeds[inputs]=m_sectionH[i].feed;
			freqs[inputs]=m_sectionH[i].frequency;
			bws[inputs]=m_sectionH[i].bandWidth;
			sectionID[inputs]=m_sectionH[i].id;
			inputs+=1;
		}
		else { //inputs==2
			ifs[inputs]=m_sectionH[i].IF[0];
			atts[inputs]=m_sectionH[i].attenuation[0];
			feeds[inputs]=m_sectionH[i].feed;
			freqs[inputs]=m_sectionH[i].frequency;
			bws[inputs]=m_sectionH[i].bandWidth;
			sectionID[inputs]=m_sectionH[i].id;
			inputs+=1;
			ifs[inputs]=m_sectionH[i].IF[1];
			atts[inputs]=m_sectionH[i].attenuation[1];
			feeds[inputs]=m_sectionH[i].feed;
			freqs[inputs]=m_sectionH[i].frequency;
			bws[inputs]=m_sectionH[i].bandWidth;
			sectionID[inputs]=m_sectionH[i].id;
			inputs+=1;
		}
	}
}

bool CDataCollection::getDump(ACS::Time& time,bool& calOn,char *& memory,char *& buffer,bool& tracking,long& buffSize)
{
	baci::ThreadSyncGuard guard(&m_mutex);
	return m_dumpCollection.popDump(time,calOn,memory,buffer,tracking,buffSize);
}

long CDataCollection::getDumpCollectionSize()
{
	return m_dumpCollection.getCollectionSize();
}

bool CDataCollection::getFakeDump(ACS::Time& time,const long& delay,bool& calOn,char *& memory,char *& buffer,bool& tracking,long& buffSize)
{
	long size=0;
	TIMEVALUE clock;
	ACS::Time now;
	IRA::CIRATools::getTime(clock);
	now=clock.value().value;
	now-=delay*10;
	//IRA::CString outString;
	//IRA::CIRATools::timeToStr(now,outString);
	//cout << "current time " << (const char *) outString << endl;
	//IRA::CIRATools::timeToStr(m_startUTTime,outString);
	//cout << "start time " << (const char *) outString << endl;
	//IRA::CIRATools::timeToStr(m_stopUTTime,outString);
	//cout << "stop time " << (const char *) outString << endl;
	if (m_startUTTime==0) { // if the scan is not started yet...nothing to do...there is no data
		return false;
	}
	else if ((m_stopUTTime!=0) && (m_fakeUTTime>=m_stopUTTime)) { // if a stop has been issued and the fake time is greater than it...
		return false; // now more data available....
	}
	else if (m_startUTTime<now) { // the scan is started....
		if (m_fakeUTTime==0) { // if the fake time is zero, not yet generated at least once......
			m_fakeUTTime=m_startUTTime; //initialize and go ahead....
		}
		else {
			if (m_fakeUTTime+getIntegrationTime()*10000<now) {
				m_fakeUTTime+=getIntegrationTime()*10000; //integration time is in millisec...
				//IRA::CIRATools::timeToStr(m_fakeUTTime,outString);
				//cout << "fake time " << (const char *) outString << endl;
			}
			else {
				return false;
			}
		}
	}
	else {
		return false;
	}
	//compute the size of the fake buffer
	for (int i=0;i<getSectionsNumber();i++) {
		size+=getSectionBins(i)*getSectionStreamsNumber(i); //pol * bins
	}
	//compute tsys size
	buffSize=sizeof(double)*getInputsNumber();
	//finally add the fake raw data size....
	buffSize+=getSampleSize()*size;
	buffer=new char[buffSize];
	memory=buffer;
	bzero(buffer,buffSize);
	time=m_fakeUTTime;
	calOn=false;
	if (time>=m_telescopeTrackingTime) tracking=m_telecopeTacking;
	else tracking=m_prevTelescopeTracking;
	return true;
}

void CDataCollection::startStopStage()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	if (m_running) m_stop=true;
	TIMEVALUE now;
	IRA::CIRATools::getTime(now);
	m_stopUTTime=now.value().value;
}

void CDataCollection::startRunnigStage()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	m_subScanHeader=false;
	m_running=true;
	m_start=false;
}

void CDataCollection::haltStopStage()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	m_running=false;
	m_stop=false;
	m_stopUTTime=m_startUTTime=m_fakeUTTime=0;
}

void CDataCollection::haltResetStage()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	m_reset=false;
}

void CDataCollection::getFileName(IRA::CString& fileName,IRA::CString& fullPath) const
{
	if (getIsNoData()) {
		fullPath=m_fullPath+"tmp/";
		fileName=m_fileName;
	}
	else {
		fileName=m_fileName;
		fullPath=m_fullPath;
	}
}

IRA::CString CDataCollection::getFileName() const 
{
	if (getIsNoData()) {
		return m_fullPath+"tmp/"+m_fileName;
	}
	else {
		return m_fullPath+m_fileName;
	}
}

IRA::CString CDataCollection::getSummaryFileName() const
{
	return m_fullPath+m_summaryFileName;
}

bool CDataCollection::setScanSetup(const Management::TScanSetup& setup,bool& recording,bool& inconsistent)
{
	baci::ThreadSyncGuard guard(&m_mutex);
	if (m_start && m_running) {
		recording=true;
		inconsistent =false;
		return false;
	}
	else {
		if (!m_scanHeader) {
			IRA::CString basePath;
			IRA::CString extraPath;
			IRA::CString baseName;
			m_project=setup.projectName;
			m_scanTag=setup.scanTag;
			m_deviceID=setup.device;
			m_scanID=setup.scanId;
			m_observer=setup.observerName;
			m_backendName=setup.backendName;
			m_scheduleName=setup.schedule;
			m_logName=setup.log;
			basePath=setup.path;
			extraPath=setup.extraPath;
			baseName=setup.baseName;
			m_scanHeader=true;
			if (extraPath!="") {
				m_fullPath=basePath+extraPath+baseName+"/";
			}
			else {
				m_fullPath=basePath+baseName+"/";
			}
			//setup.scanLayout is not used
			return true;
		}
		else {
			recording=false;
			inconsistent=true;
			return false;
		}
	}
}

bool CDataCollection::setSubScanSetup(const Management::TSubScanSetup& setup,bool& recording,bool& inconsistent)
{
	baci::ThreadSyncGuard guard(&m_mutex);
	////*******************************************************
	IRA::CString outstr;
	IRA::CIRATools::timeToStr(setup.startUt,outstr);
	ACS_LOG(LM_FULL_INFO,"CDataCollection::setSubScanSetup()",(LM_NOTICE,"SUBSCAN IS %ld",(long)setup.subScanId));
	ACS_LOG(LM_FULL_INFO,"CDataCollection::setSubScanSetup()",(LM_NOTICE,"REQUESTED_START_TIME WAS %s",(const char *)outstr));
	///////////// DEBUGGING ***************************************
	if (m_start && m_running) {
		recording=true;
		inconsistent=false;
		/////*****************************************
		ACS_LOG(LM_FULL_INFO,"CDataCollection::setSubScanSetup()",(LM_WARNING,"RECORD_WHILE_RECORDING_REQUEST %d %d",m_start,m_running));
		///**************************** DEBUGGING, Inserted to track the Problem with roach and nodata
		return false;
	}
	else {
		if (m_scanHeader && !m_subScanHeader) {
			IRA::CString temp;
			IRA::CString baseName;
			m_subScanID=setup.subScanId;
			baseName=setup.baseName;
			temp.Format("_%03d_%03d",m_scanID,m_subScanID);
			m_fileName=baseName+temp+".fits";
			m_summaryFileName="Sum_"+baseName+".fits";
			m_subScanHeader=true;
			m_scanAxis=setup.axis;
			m_startUTTime=setup.startUt;
			//setup.startUt and setup.targetID are not used
			if (getIsNoData()) { // this is a special behaviour with no data
            	m_start=true;
			}
			return true;
		}
		else {
			recording=false;
			inconsistent=true;
			/////*****************************************
			ACS_LOG(LM_FULL_INFO,"CDataCollection::setSubScanSetup()",(LM_WARNING,"INCONSISTENT_REQUEST %d %d %d %d",m_start,m_running,m_scanHeader,m_subScanHeader));
			///**************************** DEBUGGING, Inserted to track the Problem with roach and nodata
			return false;
		}
	}
}

bool CDataCollection::stopScan()
{
	baci::ThreadSyncGuard guard(&m_mutex);
	m_ready=false;
	m_scanHeader=false;
	m_writeSummary=true;
	return true;
}

/*************** Private ***********************************************************/





