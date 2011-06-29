#ifndef _BOSS_CORE_PREPARESCAN_I_
#define _BOSS_CORE_PREPARESCAN_I_

// $Id: BossCore_prepareScan.i,v 1.11 2011-05-27 13:55:30 a.orlati Exp $


/*void CBossCore::prepareScan(Antenna::EphemGenerator_ptr generator,ACS::Time& startUT,const Antenna::TTrackingParameters& prim,const Antenna::TTrackingParameters& sec,
		const TOffset& userOffset,Antenna::TTrackingParameters& lastPar,Antenna::TSections& section,double& ra,double& dec,double& lon,double& lat,double& vlsr,IRA::CString& sourceName,TOffset& scanOffset) const
		throw (ComponentErrors::CouldntCallOperationExImpl,ComponentErrors::UnexpectedExImpl,ComponentErrors::CORBAProblemExImpl,AntennaErrors::ScanErrorExImpl,AntennaErrors::SecondaryScanErrorExImpl,
				AntennaErrors::MissingTargetExImpl)
{
	double latOffTmp,lonOffTmp;
	TOffset scanOffTmp;
	Antenna::TCoordinateFrame offFrameTmp;
	Antenna::TTrackingParameters primary,secondary;
	double secRa,secDec,secLon,secLat,secVlsr;
	IRA::CString secSourceName;
	bool secondaryActive=false;
	if (prim.applyOffsets) { //offsets are newer overloaded by the secondary track offsets...because the secondary offsets are always ignored!
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"APPLYING_OFFSETS: %lf %lf",prim.longitudeOffset,prim.latitudeOffset));
		scanOffTmp=TOffset(prim.longitudeOffset,prim.latitudeOffset,prim.offsetFrame);
		addOffsets(lonOffTmp,latOffTmp,offFrameTmp,userOffset,scanOffTmp);
	}
	else { //keep the orginals
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"KEEP_ORIGINAL_OFFSETS"));
		latOffTmp=userOffset.lat;
		lonOffTmp=userOffset.lon;
		offFrameTmp=userOffset.frame;
		// the addOffsets is the frame of user and scan offset are different uses the scan offset, in that case the scan offset are null so I want to make sure that
		// the composition of user and scan offset is equal to user offsets in that case.
		scanOffTmp.frame=userOffset.frame; 
	}
	try {
		generator->setOffsets(lonOffTmp,latOffTmp,offFrameTmp); //could throw an AntennaErrorsEx exception
	}
	catch (CORBA::SystemException& ex) {
		_EXCPT(ComponentErrors::CORBAProblemExImpl,impl,"CBossCore::prepareScan()");
		impl.setName(ex._name());
		impl.setMinor(ex.minor());
		throw impl;
	}
	catch(AntennaErrors::AntennaErrorsEx& ex) {
		_ADD_BACKTRACE(ComponentErrors::CouldntCallOperationExImpl,impl,ex,"CBossCore::prepareScan()");
		impl.setOperationName("setOffsets()");
		throw impl;
	}
	// let's save the primary and secondary tracks information.
	copyTrack(primary,prim);
	copyTrack(secondary,sec); 
	secondary.applyOffsets=false; //the offsets of the secondary track are always ignored 
	secondary.secondary=false;
	// support for secondary scan.......
	if (primary.secondary) { // if the the secondary bit is set the secondary scan must be taken into consideration
		if ((secondary.type!=Antenna::ANT_NONE)||(lastPar.type!=Antenna::ANT_NONE)) {  //check if the secondary scan is given or the lastScan has been commanded
			if (primary.type==Antenna::ANT_OTF) {  // if it is OTF some special check are due
				if (primary.otf.description!=Antenna::SUBSCAN_CENTER) {
					_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
					impl.setTargetName((const char *)primary.targetName);
					impl.setGeneratorCode("OTF");
					throw impl;
				}
				if (primary.otf.geometry==Antenna::SUBSCAN_GREATCIRCLE) {
					_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
					impl.setTargetName((const char *)primary.targetName);
					impl.setGeneratorCode("OTF");
					throw impl;				
				}
				if ((primary.otf.coordFrame!=Antenna::ANT_EQUATORIAL)&&(primary.otf.coordFrame==Antenna::ANT_GALACTIC)) {
					_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
					impl.setTargetName((const char *)primary.targetName);
					impl.setGeneratorCode("OTF");
					throw impl;									
				}
			}
		}
		else {
			_EXCPT(AntennaErrors::MissingTargetExImpl,impl,"CBossCore::prepareScan()");
			throw impl;
		}
		if (secondary.type!=Antenna::ANT_NONE) { // the secondary track passed takes the precendence
			if (primary.type==Antenna::ANT_OTF) {  // if it is OTF...we need to take the ra/dec of lon/lat as scan center....
				if (!prepareOTFSecondary(secondary,secSourceName,secRa,secDec,secLon,secLat,secVlsr)) {
					_EXCPT(AntennaErrors::SecondaryScanErrorExImpl,ex,"CBossCore::prepareScan()");
					throw ex;
				}
				secondaryActive=true;
				//if it is correct the secondary is stored as the current last track
				copyTrack(lastPar,secondary);
			}
			else { // this is the case primary.secondary=true and secondary track given, in practice a non sense... we consider the primary as it is 
				secondaryActive=false;
			}
		}
		else if (lastPar.type!=Antenna::ANT_NONE) { // otherwise the scan commanded the previuos time is used as secondary
			if (primary.type==Antenna::ANT_OTF) {  // if it is OTF...we need to take the ra/dec of lon/lat as scan center....
				if (!prepareOTFSecondary(lastPar,secSourceName,secRa,secDec,secLon,secLat,secVlsr)) {
					_EXCPT(AntennaErrors::SecondaryScanErrorExImpl,ex,"CBossCore::prepareScan()");
					throw ex;
				}
				secondaryActive=true;
				if (primary.otf.coordFrame==Antenna::ANT_EQUATORIAL) {
					primary.otf.lon1=secRa;
					primary.otf.lat1=secDec;
				}
				else if (primary.otf.coordFrame==Antenna::ANT_GALACTIC) {
					primary.otf.lon1=secLon;
					primary.otf.lat1=secLat;
				}
			}
			else { // this is the case the primary.secondary=true and the last commanded scan has to be used......
				//my current primary scan is going to be the last commanded, offset are neglected because they are already been applyed
				copyTrack(primary,lastPar);
				secondaryActive=true;
			}
			// in that case the lastScan continue to be the same
		}
	}
	// by default we choose the section reported in tracking point structure
	section=primary.section;
	if (primary.type==Antenna::ANT_SIDEREAL) {
		Antenna::SkySource_var tracker;
		tracker=Antenna::SkySource::_narrow(generator);
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SIDEREAL_TRACKING"));
		try {
			if (primary.paramNumber==0) {
				tracker->loadSourceFromCatalog(primary.targetName);
			}
			else if (primary.frame==Antenna::ANT_EQUATORIAL) {
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SOURCE_RA_DEC: %lf %lf",primary.parameters[0],primary.parameters[1]));
				if (primary.paramNumber==2) {
					tracker->setSourceFromEquatorial(primary.targetName,primary.parameters[0],primary.parameters[1],primary.equinox,0.0,0.0,0.0,0.0);
				}
				else {
					tracker->setSourceFromEquatorial(primary.targetName,primary.parameters[0],primary.parameters[1],primary.equinox,primary.parameters[2],primary.parameters[3],primary.parameters[4],
							primary.parameters[5]);					
				}
			}
			else if (primary.frame==Antenna::ANT_GALACTIC) {
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SOURCE_LON_LAT: %lf %lf",primary.parameters[0],primary.parameters[1]));
				tracker->setSourceFromGalactic(primary.targetName,primary.parameters[0],primary.parameters[1]);
			}
			else if (primary.frame==Antenna::ANT_HORIZONTAL) {
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SOURCE_AZ_EL: %lf %lf",primary.parameters[0],primary.parameters[1]));
				tracker->setFixedPoint(primary.parameters[0],primary.parameters[1]);
			}
			//copy the current track and store it
			copyTrack(lastPar,primary);
			lastPar.applyOffsets=false;
			lastPar.secondary=false;
		}
		catch(AntennaErrors::AntennaErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setTargetName((const char *)primary.targetName);
			impl.setGeneratorCode("SIDEREAL");
			throw impl;
		}
		catch (ComponentErrors::ComponentErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setTargetName((const char *)primary.targetName);
			impl.setGeneratorCode("SIDEREAL");
			throw impl;
		}
		catch (...) {
			_THROW_EXCPT(ComponentErrors::UnexpectedExImpl,"CBossCore::prepareScan()");
		}
		try {
			Antenna::SkySourceAttributes_var att;
			tracker->getAttributes(att);
			ra=att->J2000RightAscension;
			dec=att->J2000Declination;
			lon=att->gLongitude;
			lat=att->gLatitude;
			vlsr=att->inputRadialVelocity;
			sourceName=IRA::CString(att->sourceID);
		}
		catch (CORBA::SystemException& ex) {
			sourceName=IRA::CString("????");
			ra=dec=0.0; // in that case I do not want to rise an error
		}		
	}
	else if (primary.type==Antenna::ANT_MOON) {
		// moon has nothing to do...no configuration
		Antenna::Moon_var tracker;
		tracker=Antenna::Moon::_narrow(generator);
		//copy the current track and store it
		copyTrack(lastPar,primary);
		lastPar.applyOffsets=false;
		lastPar.secondary=false;
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"MOON_TRACKING"));
		try {
			Antenna::MoonAttributes_var att;
			tracker->getAttributes(att);
			ra=att->J2000RightAscension;
			dec=att->J2000Declination;
			lon=att->gLongitude;
			lat=att->gLatitude;
			vlsr=0.0; 
			sourceName=IRA::CString(att->sourceID);
		}
		catch (CORBA::SystemException& ex) {
			sourceName=IRA::CString("????");
			ra=dec=0.0; // in that case I do not want to rise an error
			vlsr=0.0;
		}		
	}
	else if (primary.type==Antenna::ANT_OTF) {
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_SCANNING"));
		Antenna::OTF_var tracker;
		tracker=Antenna::OTF::_narrow(generator);
		try {
			//for otf the section reported in par structure is ovecome by the one computed by the OTF generator....
			// if startUT==0 (start as soon as possible) the component will set back the estimated startUT.
			ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_LON_LAT: %lf %lf %lf %lf",primary.otf.lon1,primary.otf.lat1,primary.otf.lon2,primary.otf.lat2));
			double roundAz=slaDranrm(m_lastEncoderAzimuth);
			section=tracker->setSubScan(roundAz,m_lastAzimuthSection,m_lastEncoderElevation,m_lastEncoderRead,primary.otf.lon1,primary.otf.lat1,primary.otf.lon2,primary.otf.lat2,primary.otf.coordFrame,
					primary.otf.geometry,primary.otf.subScanFrame,primary.otf.description,primary.otf.direction,startUT,primary.otf.subScanDuration);
			ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SECTION_IS: %ld",section));
		}
		catch(AntennaErrors::AntennaErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setTargetName((const char *)primary.targetName);
			impl.setGeneratorCode("OTF");
			throw impl;
		}
		catch (ComponentErrors::ComponentErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setTargetName((const char *)primary.targetName);
			impl.setGeneratorCode("OTF");
			throw impl;
		}
		catch (...) {
			_THROW_EXCPT(ComponentErrors::UnexpectedExImpl,"CBossCore::prepareScan()");
		}
		try {
			Antenna::OTFAttributes_var att;
			tracker->getAttributes(att);
			ra=att->centerRA;
			dec=att->centerDec;
			lon=att->centerGLon;
			lat=att->centerGLat;
			if (secondaryActive) { // in case of a secondary track....it is possible that vlsr and flux are computable
				vlsr=secVlsr;
				sourceName=secSourceName;
			}
			else { //normal OTF...impossible to know target name or vlsr
				vlsr=0.0;
				sourceName=IRA::CString(att->sourceID);
			}
			startUT=att->startUT; //very important....we want to save the start time computed by OTF
		}
		catch (CORBA::SystemException& ex) {
			sourceName=IRA::CString("????");
			ra=dec=0.0; // in that case I do not want to rise an error
			vlsr=0.0;
		}
	}
	else if (primary.type==Antenna::ANT_SATELLITE) {
	}
	else if (primary.type==Antenna::ANT_SOLARSYTEMBODY) {
	}
	else if (primary.type==Antenna::ANT_SUN) { 
	}
	// if everything looks ok....return back the offsets.....
	scanOffset=scanOffTmp;
}*/

void CBossCore::copyTrack(Antenna::TTrackingParameters& dest,const Antenna::TTrackingParameters& source,bool copyOffs) const
{
	dest.targetName=source.targetName;
	dest.type=source.type;
	dest.paramNumber=source.paramNumber;
	for (long k=0;k<Antenna::ANTENNA_TRACKING_PARAMETER_NUMBER;k++) dest.parameters[k]=source.parameters[k];
	dest.frame=source.frame;
	dest.equinox=source.equinox;
	if (copyOffs) {
		dest.longitudeOffset=source.longitudeOffset;
		dest.latitudeOffset=source.latitudeOffset;
		dest.offsetFrame=source.offsetFrame;
		dest.applyOffsets=source.applyOffsets;
	}
	dest.section=source.section;
	dest.secondary=source.secondary;
	dest.otf.lon1=source.otf.lon1;
	dest.otf.lat1=source.otf.lat1;
	dest.otf.lon2=source.otf.lon2;
	dest.otf.lat2=source.otf.lat2;
	dest.otf.coordFrame=source.otf.coordFrame;
	dest.otf.geometry=source.otf.geometry;
	dest.otf.subScanFrame=source.otf.subScanFrame;
	dest.otf.description=source.otf.description;
	dest.otf.direction=source.otf.direction;
	dest.otf.subScanDuration=source.otf.subScanDuration;
}

Antenna::EphemGenerator_ptr CBossCore::prepareScan(
		bool useInternals,
		ACS::Time& startUT,
		const Antenna::TTrackingParameters& prim,
		const Antenna::TTrackingParameters& sec,
		const TOffset& userOffset,
		Antenna::TGeneratorType& generatorType,
		Antenna::TTrackingParameters& lastPar,
		Antenna::TSections& section,
		double& ra,
		double& dec,
		double& lon,
		double& lat,
		double& vlsr,
		IRA::CString& sourceName,
		TOffset& scanOffset) 
		throw (ComponentErrors::CouldntCallOperationExImpl,ComponentErrors::UnexpectedExImpl,ComponentErrors::CORBAProblemExImpl,
			   AntennaErrors::ScanErrorExImpl,AntennaErrors::SecondaryScanErrorExImpl,AntennaErrors::MissingTargetExImpl,AntennaErrors::LoadGeneratorErrorExImpl)
{
	double latOffTmp,lonOffTmp;
	TOffset scanOffTmp(0.0,0.0,Antenna::ANT_HORIZONTAL);
	Antenna::TCoordinateFrame offFrameTmp;
	Antenna::TTrackingParameters primary,secondary;
	double secRa,secDec,secLon,secLat,secVlsr;
	IRA::CString secSourceName;
	bool secondaryActive=false;
	bool lastActive=false;
	
	Antenna::EphemGenerator_var currentGenerator;

	// let's save the primary and secondary tracks information.
	copyTrack(primary,prim);
	copyTrack(secondary,sec); 
	secondary.applyOffsets=false; //the offsets of the secondary track are always ignored as well as the secondary bit
	secondary.secondary=false;		
	
	// some priliminary checks and initializations
	if (primary.type==Antenna::ANT_NONE) {
		if (!primary.secondary) { // if primary.type is NONE than the secondary flag has to be given......
			_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
			impl.setReason("The scan configuration is not supported");
			throw impl;	
		}
		if (secondary.type==Antenna::ANT_NONE) {
			if (lastPar.type==Antenna::ANT_NONE) {
				_EXCPT(AntennaErrors::MissingTargetExImpl,impl,"CBossCore::prepareScan()");
				throw impl;
			}
			else {
				copyTrack(primary,lastPar,false);
			}
		}
		else {
			copyTrack(primary,secondary,false);
		}	
	}
	else if (primary.secondary) { // // support for secondary scan.......primary.type mut be different from NONE
		if (primary.type==Antenna::ANT_OTF) {  // if it is OTF some special check are due
			if ((secondary.type!=Antenna::ANT_NONE)||(lastPar.type!=Antenna::ANT_NONE)) {  //check if the secondary scan is given or the lastScan has been commanded
				if (primary.otf.description!=Antenna::SUBSCAN_CENTER) {
					_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
					impl.setReason("The description of the OTF can be only CENTER in this configuration");
					throw impl;
				}
				if (primary.otf.geometry==Antenna::SUBSCAN_GREATCIRCLE) {
					_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
					impl.setReason("The geometry of the OTF cannot be GREATCIRCLE in this configuration");
					throw impl;				
				}
				if ((primary.otf.coordFrame!=Antenna::ANT_EQUATORIAL)&&(primary.otf.coordFrame==Antenna::ANT_GALACTIC)) {
					_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
					impl.setReason("This OTF configuration support only galactic or equatorial frame");
					throw impl;									
				}
			}
			else { // if primary.secondary==true and neither secondary.type!=NONE lastPar.type!=NONE
				_EXCPT(AntennaErrors::MissingTargetExImpl,impl,"CBossCore::prepareScan()");
				throw impl;
			}			
			if (secondary.type!=Antenna::ANT_NONE) { // the secondary track takes the precendence
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_ON_SECONDARY_TRACK"));
				if (!prepareOTFSecondary(secondary,secSourceName,secRa,secDec,secLon,secLat,secVlsr)) {
					_EXCPT(AntennaErrors::SecondaryScanErrorExImpl,ex,"CBossCore::prepareScan()");
					throw ex;
				}
				if (primary.otf.coordFrame==Antenna::ANT_EQUATORIAL) {
					primary.otf.lon1=secRa;
					primary.otf.lat1=secDec;
				}
				else if (primary.otf.coordFrame==Antenna::ANT_GALACTIC) {
					primary.otf.lon1=secLon;
					primary.otf.lat1=secLat;
				}
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_SECONDARY_PREPARED_ON_LONLAT %lf %lf",primary.otf.lon1,primary.otf.lat1));
				secondaryActive=true;
			}
			else if (lastPar.type!=Antenna::ANT_NONE) { // otherwise the scan commanded the previuos time is used as secondary
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_ON_LAST_COMMANDED_TRACK"));
				if (!prepareOTFSecondary(lastPar,secSourceName,secRa,secDec,secLon,secLat,secVlsr)) {
					_EXCPT(AntennaErrors::SecondaryScanErrorExImpl,ex,"CBossCore::prepareScan()");
					throw ex;
				}
				if (primary.otf.coordFrame==Antenna::ANT_EQUATORIAL) {
					primary.otf.lon1=secRa;
					primary.otf.lat1=secDec;
				}
				else if (primary.otf.coordFrame==Antenna::ANT_GALACTIC) {
					primary.otf.lon1=secLon;
					primary.otf.lat1=secLat;
				}
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_LAST_TRACK_PREPARED_ON_LONLAT %lf %lf",primary.otf.lon1,primary.otf.lat1));
				lastActive=true;
			}
		}
		else { // if it is not OTF
			_EXCPT(AntennaErrors::ScanErrorExImpl,impl,"CBossCore::prepareScan()");
			impl.setReason("The scan configuration is not supported");
			throw impl;	
		}
	}
	generatorType=primary.type;
	try {
		if (!useInternals) {
			ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"LOADING_REQUIRED_PRIMARY_GENERATOR"));
			currentGenerator=loadPrimaryGenerator(generatorType);
		}
		else {
			ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"LOADING_REQUIRED_INTERNAL_GENERATOR"));
			currentGenerator=loadInternalGenerator(generatorType);
		}
	}
	catch (ACSErr::ACSbaseExImpl& ex) {
		_ADD_BACKTRACE(AntennaErrors::LoadGeneratorErrorExImpl,impl,ex,"CBossCore::prepareScan()");
		throw impl;
	}
	if (primary.applyOffsets) { //offsets are newer overloaded by the secondary track offsets...because the secondary offsets are always ignored!
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"APPLYING_OFFSETS: %lf %lf",primary.longitudeOffset,primary.latitudeOffset));
		scanOffTmp=TOffset(primary.longitudeOffset,primary.latitudeOffset,primary.offsetFrame);
		addOffsets(lonOffTmp,latOffTmp,offFrameTmp,userOffset,scanOffTmp);
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"TOTAL_OFFSETS: %lf %lf",lonOffTmp,latOffTmp));
	}
	else { //keep the orginals
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"KEEP_ORIGINAL_OFFSETS"));
		latOffTmp=userOffset.lat;
		lonOffTmp=userOffset.lon;
		offFrameTmp=userOffset.frame;
		// the addOffsets is the frame of user and scan offset are different uses the scan offset, in that case the scan offset are null so I want to make sure that
		// the composition of user and scan offset is equal to user offsets in that case.
		scanOffTmp.frame=userOffset.frame; 
	}
	try {
		currentGenerator->setOffsets(lonOffTmp,latOffTmp,offFrameTmp); //could throw an AntennaErrorsEx exception
	}
	catch (CORBA::SystemException& ex) {
		_EXCPT(ComponentErrors::CORBAProblemExImpl,impl,"CBossCore::prepareScan()");
		impl.setName(ex._name());
		impl.setMinor(ex.minor());
		throw impl;
	}
	catch(AntennaErrors::AntennaErrorsEx& ex) {
		_ADD_BACKTRACE(ComponentErrors::CouldntCallOperationExImpl,impl,ex,"CBossCore::prepareScan()");
		impl.setOperationName("setOffsets()");
		throw impl;
	}
	// by default we choose the section reported in tracking point structure
	section=primary.section;
	if (primary.type==Antenna::ANT_SIDEREAL) {
		Antenna::SkySource_var tracker;
		tracker=Antenna::SkySource::_narrow(currentGenerator);
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SIDEREAL_TRACKING"));
		try {
			if (primary.paramNumber==0) {
				tracker->loadSourceFromCatalog(primary.targetName);
			}
			else if (primary.frame==Antenna::ANT_EQUATORIAL) {
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SOURCE_RA_DEC: %lf %lf",primary.parameters[0],primary.parameters[1]));
				if (primary.paramNumber==2) {
					tracker->setSourceFromEquatorial(primary.targetName,primary.parameters[0],primary.parameters[1],primary.equinox,0.0,0.0,0.0,0.0);
				}
				else {
					tracker->setSourceFromEquatorial(primary.targetName,primary.parameters[0],primary.parameters[1],primary.equinox,primary.parameters[2],primary.parameters[3],primary.parameters[4],
							primary.parameters[5]);					
				}
			}
			else if (primary.frame==Antenna::ANT_GALACTIC) {
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SOURCE_LON_LAT: %lf %lf",primary.parameters[0],primary.parameters[1]));
				tracker->setSourceFromGalactic(primary.targetName,primary.parameters[0],primary.parameters[1]);
			}
			else if (primary.frame==Antenna::ANT_HORIZONTAL) {
				ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SOURCE_AZ_EL: %lf %lf",primary.parameters[0],primary.parameters[1]));
				tracker->setFixedPoint(primary.parameters[0],primary.parameters[1]);
			}
			//copy the current track and store it
			copyTrack(lastPar,primary);
			lastPar.applyOffsets=false;
			lastPar.secondary=false;
		}
		catch(AntennaErrors::AntennaErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setReason("Unable to load the scan configuration into the generator");
			throw impl;
		}
		catch (ComponentErrors::ComponentErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setReason("Unable to load the scan configuration into the generator");
			throw impl;
		}
		catch (...) {
			_THROW_EXCPT(ComponentErrors::UnexpectedExImpl,"CBossCore::prepareScan()");
		}
		try {
			Antenna::SkySourceAttributes_var att;
			tracker->getAttributes(att);
			ra=att->J2000RightAscension;
			dec=att->J2000Declination;
			lon=att->gLongitude;
			lat=att->gLatitude;
			vlsr=att->inputRadialVelocity;
			sourceName=IRA::CString(att->sourceID);
		}
		catch (CORBA::SystemException& ex) {
			sourceName=IRA::CString("????");
			ra=dec=0.0; // in that case I do not want to rise an error
		}		
	}
	else if (primary.type==Antenna::ANT_MOON) {
		// moon has nothing to do...no configuration
		Antenna::Moon_var tracker;
		tracker=Antenna::Moon::_narrow(currentGenerator);
		//copy the current track and store it
		copyTrack(lastPar,primary);
		lastPar.applyOffsets=false;
		lastPar.secondary=false;
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"MOON_TRACKING"));
		try {
			Antenna::MoonAttributes_var att;
			tracker->getAttributes(att);
			ra=att->J2000RightAscension;
			dec=att->J2000Declination;
			lon=att->gLongitude;
			lat=att->gLatitude;
			vlsr=0.0; 
			sourceName=IRA::CString(att->sourceID);
		}
		catch (CORBA::SystemException& ex) {
			sourceName=IRA::CString("????");
			ra=dec=0.0; // in that case I do not want to rise an error
			vlsr=0.0;
		}		
	}
	else if (primary.type==Antenna::ANT_OTF) {
		ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_SCANNING"));
		Antenna::OTF_var tracker;
		tracker=Antenna::OTF::_narrow(currentGenerator);
		try {
			//for otf the section reported in par structure is ovecome by the one computed by the OTF generator....
			// if startUT==0 (start as soon as possible) the component will set back the estimated startUT.
			ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"OTF_LON_LAT: %lf %lf %lf %lf",primary.otf.lon1,primary.otf.lat1,primary.otf.lon2,primary.otf.lat2));
			double roundAz=slaDranrm(m_lastEncoderAzimuth);
			section=tracker->setSubScan(roundAz,m_lastAzimuthSection,m_lastEncoderElevation,m_lastEncoderRead,primary.otf.lon1,primary.otf.lat1,primary.otf.lon2,primary.otf.lat2,primary.otf.coordFrame,
					primary.otf.geometry,primary.otf.subScanFrame,primary.otf.description,primary.otf.direction,startUT,primary.otf.subScanDuration);
			ACS_LOG(LM_FULL_INFO,"CBossCore::prepareScan()",(LM_DEBUG,"SECTION_IS: %ld",section));
			if (secondaryActive) {
				copyTrack(lastPar,secondary);
			}
			else if (lastActive) {
				// if lastActive no need to do anything because the last scan paramters remains the same
			}
			else {
				copyTrack(lastPar,primary);
				lastPar.applyOffsets=false;
				lastPar.secondary=false;
			}
		}
		catch(AntennaErrors::AntennaErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setReason("Unable to load the scan configuration into the generator");
			throw impl;
		}
		catch (ComponentErrors::ComponentErrorsEx& ex) {
			_ADD_BACKTRACE(AntennaErrors::ScanErrorExImpl,impl,ex,"CBossCore::prepareScan()");
			impl.setReason("Unable to load the scan configuration into the generator");
			throw impl;
		}
		catch (...) {
			_THROW_EXCPT(ComponentErrors::UnexpectedExImpl,"CBossCore::prepareScan()");
		}
		try {
			Antenna::OTFAttributes_var att;
			tracker->getAttributes(att);
			ra=att->centerRA;
			dec=att->centerDec;
			lon=att->centerGLon;
			lat=att->centerGLat;
			if (secondaryActive||lastActive) { // in case of a secondary track....it is possible that vlsr and flux are computable
				vlsr=secVlsr;
				sourceName=secSourceName;
			}
			else { //normal OTF...impossible to know target name or vlsr
				vlsr=0.0;
				sourceName=IRA::CString(att->sourceID);
			}
			startUT=att->startUT; //very important....we want to save the start time computed by OTF
		}
		catch (CORBA::SystemException& ex) {
			sourceName=IRA::CString("????");
			ra=dec=0.0; // in that case I do not want to rise an error
			vlsr=0.0;
		}
	}
	else if (primary.type==Antenna::ANT_SATELLITE) {
	}
	else if (primary.type==Antenna::ANT_SOLARSYTEMBODY) {
	}
	else if (primary.type==Antenna::ANT_SUN) { 
	}
	// if everything looks ok....return back the offsets.....
	/*
	lonOff=lonOffTmp;
	offFrame=offFrameTmp;*/
	scanOffset=scanOffTmp;
	return currentGenerator._retn();
}

bool CBossCore::prepareOTFSecondary(const Antenna::TTrackingParameters& sec,IRA::CString& sourceName,double& ra,double& dec,double& lon,double& lat,double& vlsr) 
{
	ACS::Time inputTime;
	TIMEVALUE now;
	TOffset scanOff;
	IRA::CString name;
	Antenna::TSections section;
	Antenna::TTrackingParameters nullScan,lastScan;
	
	Antenna::EphemGenerator_var tmp;
	Antenna::TGeneratorType genType;
	
	nullScan.type=Antenna::ANT_NONE;
	nullScan.secondary=false;
	nullScan.applyOffsets=false;
	
	lastScan.type=Antenna::ANT_NONE;
	lastScan.secondary=false;
	lastScan.applyOffsets=false;
	
	// get current Time
	IRA::CIRATools::getTime(now);
	inputTime=now.value().value;

	/***************************************************************************/
	// in order to fine tune this should be a two steps alghoritm: in the first step compute the position...then compute the estimated arrival time (of the telescope)
	// then recompute the position using that time. This is to optimize in case of object with fast proper motion, like satellites of planets
	/******************************************************************************/
	try {
		tmp=prepareScan(true,inputTime,sec,nullScan,m_userOffset,genType,lastScan,section,ra,dec,lon,lat,vlsr,sourceName,scanOff); 
	}
	catch (ACSErr::ACSbaseExImpl& ex) {
		return false;
	}
	return true;
}

#endif

