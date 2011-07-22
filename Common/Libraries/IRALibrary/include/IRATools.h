#ifndef _IRATOOLS_H
#define _IRATOOLS_H

/* ***************************************************************************************************** */
/* IRA Istituto di Radioastronomia                                                                       */
/*                                                                                                       */
/* This code is under GNU General Public Licence (GPL).                                                  */
/*                                                                                                       */
/*                                                                                                       */
/* Who                                when            What                                               */
/* Andrea Orlati(aorlati@ira.inaf.it)  11/10/2004	   Creation                                          */
/* Andrea Orlati(aorlati@ira.inaf.it)  24/01/2004	   Added functionstimeDifference and timeCopy        */
/* Andrea Orlati(aorlati@ira.inaf.it)  20/06/2005	   Modified methods getDBValue so that they make use */
/*                                                     of class DBConnector no more  					 */
/* Andrea Orlati(aorlati@ira.inaf.it)  21/08/2005	   added function to manage time through the ACS helper classes */
/* Andrea Orlati(aorlati@ira.inaf.it)  24/05/2006	   added function to change values of CDB fields     */
/* Andrea Orlati(aorlati@ira.inaf.it)  31/01/2008	  parameters of timeSubtract rouitne are now passed as const     */
/* Andrea Orlati(aorlati@ira.inaf.it)  04/06/2008	  getDBValue, setDBValue now accept an extra parameter: name     */
/* Andrea Orlati(aorlati@ira.inaf.it)  15/10/2008	  added function getNextToken     */
/* Andrea Orlati(aorlati@ira.inaf.it)  15/10/2008	  added overloaded function Wait, it now accept anly microseconds    */
/* Andrea Orlati(aorlati@ira.inaf.it)  05/05/2009	  added a template construct to choose a type based on the evaluation of a boolean expression    */
/* Andrea Orlati(aorlati@ira.inaf.it)  29/07/2010	  added a serie of function to parse angles and time from user input   */
/* Andrea Orlati(aorlati@ira.inaf.it)  12/09/2010	  The parsing of right ascension is now also added with the support for sexagesimal format */
/* Andrea Orlati(aorlati@ira.inaf.it)  13/09/2010	   added OffsetTorad() */
/* Andrea Orlati(aorlati@ira.inaf.it)  28/02/2011	   methos skyFrequency() */

// $Id: IRATools.h,v 1.23 2011-02-28 17:37:37 a.orlati Exp $

#include <time.h>
#include <sys/time.h>
#include <baciDB.h>
#include <maciContainerServices.h>
#include <acstimeEpochHelper.h>
#include <acstimeDurationHelper.h>
#include <maciSimpleClient.h>

/**
 * @mainpage IRALibrary API Documentation
 * @date 13/09/2010 
 * @version 2.96.0
 * @author <a href=mailto:a.orlati@ira.inaf.it>Andrea Orlati</a>
 * @remarks Last compiled under ACS 7.0.2
 * @remarks gcc version is 4.1.2
 *
 * This library is a collection of functions and classes that were assembled together in order to 
 * provide a basic and general tool for developers. This library includes APIs that can be used to 
 * interface your software to common hardware devices and field-buses (TCP, serial lines ecc..) without bothering 
 * with details. Some utility function and macros are also present in order to provide shortcuts for some
 * of the common ACS procedures.
*/

/**
 * @namespace IRA
 * All functions, classes and types of this library are defined under IRA namespace. In order to take advantage of these tools you
 * have to "use" this namespace.
*/
namespace IRA {

/**
 * This class is the unspecialized part of a template construct. This construct can be used to delcare a type according a condition.
 * Usage example:
 * <pre>
 * condition_type<b==255,char,int>::type p;
 * p=b;
 * </pre>
 * @param cond boolean expression to be evaluated
 * @param ifTrue type if the condition is true
 * @param ifFalse type if the condition is false
 */ 
template<bool cond,typename ifTrue,typename ifFalse> struct conditional_type  {
	typedef ifTrue type; 
};

/**
 * This template is the specialized part for the conidtion_type construct
 */
template<typename ifTrue,typename ifFalse> struct conditional_type<false,ifTrue,ifFalse> {
	typedef ifFalse type; 
};

/**
 * This class contains some functions which could be useful for the developer.
 * getDBValue method can be used to retrive components attributes or property attributes. This can be quickly done like this:
 *  <pre>
 *	   // get an attribute (max_value) from a property (elevation)
 *	   if (!CIRATools::getDBValue(Services,"elevation/max_value",tmpd)) {
 *           // error
 *     }
 *     // get an attribute (ConnectTimeout) from a component
 *	   if (!CIRATools::getDBValue(Services,"ConnectTimeout",tmpd)) {
 *           // error
 *     }
 * </pre>
 * The functions that deal with the time are based on ACS helper classes: DurationHelper and EpochHelper. These two classes are wrappers
 * of the IDL-defined structures Duration and Epoch respectively. Information about these classes can be found whithin the C++ API framework
 * documentation in the ACS web page. 
 * @author <a href=mailto:a.orlati@ira.cnr.it>Andrea Orlati</a>,
 * Istituto di Radioastronomia, Italia<br>
*/
class CIRATools {
public:
	/**
	 * Makes a thread sleep for the specified time.
	 * @param seconds number of seconds to sleep
	 * @param micro number of microseconds to sleep
	*/
	static void Wait(int seconds,long micro);
	
	/**
	 * Makes the thread sleep for the specified time.
	 * @param micro total number of micro seconds to sleep.
	*/
	static void Wait(long long micro);
	/**
     * Converts a duration time to elapsed microseconds.
     * @param time duration time
	 * @return the number of microseconds of the duration
	*/
	static DDWORD timeMicroSeconds(TIMEDIFFERENCE& time);
	/**
	 * Call this function to get the current time.
	 * @param Now the ACS EpochHelper class that contains the current time
	*/
	static void getTime(TIMEVALUE& Now);
	/** 
	 * This function performs the copy of an epoch
	 * @param dst destination epoch
	 * @param src source epoch
	*/
	static void timeCopy(TIMEVALUE& dst,const TIMEVALUE& src);
	/**
	 * Computes the absolute time difference in microseconds between two epoches. 
	 * @param First EpochHelper class storing information about the starting time
	 * @param Last EpochHelper class storing information about the finishing time
	 * @return Number of microseconds elapsed between the first and the second time
	*/
	static DDWORD timeDifference(TIMEVALUE& First,TIMEVALUE& Last);
	
	/**
	 * Computes the time difference between two epoches in microseconds. In this case the difference could be negative.
	 * @param First EpochHelper class storing information about the first time
	 * @param Second EpochHelper class storing information about the second time
	 * @return the difference, first minus second, as number of microseconds
	*/
	static long long timeSubtract(const TIMEVALUE& First,const TIMEVALUE& Second);
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val double value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be found correctly. 
	*/
	static bool getDBValue(maci::ContainerServices *Services,CString fieldName,double &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database when dealing with a client application
	 * @param client pointer to the maci simpleclient, it will be used to obtain reference of the DAL.
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val double value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be found correctly. 
	*/
	static bool getDBValue(maci::SimpleClient *client,CString fieldName,double &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val long value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be found correctly. 
	*/	
	static bool getDBValue(maci::ContainerServices *Services,CString fieldName,long &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database when dealing with a client application
	 * @param client pointer to the maci simpleclient, it will be used to obtain reference of the DAL.
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val long value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be found correctly. 
	*/	
	static bool getDBValue(maci::SimpleClient *client,CString fieldName,long &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val unsigned long value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @return true if the value could be found correctly. 
	*/
	static bool getDBValue(maci::ContainerServices *Services,CString fieldName,DWORD &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database when dealing with a client application.
	 * @param client pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val unsigned long value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @return true if the value could be found correctly. 
	*/
	static bool getDBValue(maci::SimpleClient *client,CString fieldName,DWORD &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val string value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be found correctly. 
	*/
	static bool getDBValue(maci::ContainerServices *Services,CString fieldName,CString &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to retrive a field value from the ACS configuration database when dealing with a client application
	 * @param client pointer to the maci simpleclient, it will be used to obtain reference of the DAL.
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val string value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be found correctly. 
	*/
	static bool getDBValue(maci::SimpleClient *client,CString fieldName,CString &Val,CString Domain="alma/",CString name="");

	/**
	 * Use this function to change the value of a field inside the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val the new double value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be set correctly. 
	*/
	static bool setDBValue(maci::ContainerServices* Services,CString fieldName,const double &Val,CString Domain="alma/",CString name="");

	/**
	 * Use this function to change the value of a field inside the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val the new long value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be set correctly. 
	*/
	static bool setDBValue(maci::ContainerServices* Services,CString fieldName,const long &Val,CString Domain="alma/",CString name="");
	
	/**
	 * Use this function to change the value of a field inside the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val the new double word value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be set correctly. 
	*/
	static bool setDBValue(maci::ContainerServices* Services,CString fieldName,const DWORD &Val,CString Domain="alma/",CString name="");

	/**
	 * Use this function to change the value of a field inside the ACS configuration database.
	 * @param Services pointer to the object that provides all container services to the component
	 * @param fieldName name of the characteristic or attribute. 
	 * @param Val the new string value of the field.
	 * @param Domain domain identifier of the curl that identifies the requested DAO. Default is "alma/".
	 * @param name the name of the "table" inside the CDB, if empty (default) the component name is used.
	 * @return true if the value could be set correctly. 
	*/
	static bool setDBValue(maci::ContainerServices* Services,CString fieldName,const CString &Val,CString Domain="alma/",CString name="");
	
	/** 
	 * This function is used to convert azimuth round angle (0..360) to the angle supported by the ACU (typically -90..450) due to azimuth wrap.
	 * The used strategy is to minimize the distance between current position and destination position. The destination
	 * coordinate is always kept inside the given hardware limits. 
	 * @param current represent the starting azimuth position of the antenna; where the antenna is at the moment (degrees, minAz..maxAz) 
	 * @param dest that's the position we want to reach (degrees, 0..360)
	 * @param minAz hardware lower limit of the azimuth axe (degrees, <=0.0)
	 * @param maxAz hadrware upper limit of the azimuth axe (degrees, >=360.0)
	 * @param section indicates wether there is a preferable section through which reaching the destination. If the destination azimuth is not
	 *                reachable from that section the other is adopted. 0 means NEUTRAL (no preference), >0 Clockwise, <0 CounterClockwise.
	 * @param cwLimit represents the limit beween cw and ccw (in degrees), cw is the sector in which the azimuth is greater than this parameter.
	 * @return the converted coordinate in order to reflect the whole hardware range of the antenna (ex 280.0 could be converted into -80.0). 
	*/
	static double getHWAzimuth(const double& current,const double& dest,const double& minAz,const double& maxAz,const long& section,const double& cwLimit);
	
	/**
	 * This function computes the intersection between two bands. Its use, for example, could be to compute the resulting band from the intersection between
	 * an IF coming from a recevier and a filter applyed before a backend.
	 * @param bf backend start frequency.
	 * @param bbw backend bandwidth
	 * @param rf receiver start frequency
	 * @param rbw receiver band width
	 * @param f start frequency of the resultin band
	 * @param bw width of the resulting band
	 * @return true if the two band produce a band (the intersection is not empty), false otherwise.
	 */  
	static bool skyFrequency(const double& bf,const double& bbw,const double& rf,const double& rbw,double& f,double& bw);
	
	/**
	 * Use this function to devide a string into  separated tokens. The token delimiter can be specified.
	 * @param str string to be devided into tokens
	 * @param start position inside the string from which to start the next token, if a new token has been found
	 *                it points to the character immediately after the localized token
	 * @param delimiter this is the character that separetesd the token
	 * @param it returns the next token in the string
	 * @return true if a token has been found
	 */
	static bool getNextToken(const IRA::CString& str,int &start,char delimiter,IRA::CString &ret);
	
	/**
	 * Use this function to convert a string containing a time interval (days-hh:mm:ss.ss) into an ACS interval notation. If <i>complete</i> argument is not set, not all the expected fields 
	 * have to be  included, the function will try to convert the string anyway. Some examples:
	 * @arg \c "10-12"  is interpreted as 10 days and 12 hours
	 * @arg \c "15.50" is interpreted as 15.50 seconds
	 * @arg \c "12:55.1" is interpreted as 12 minutes and 55.1 seconds
	 * @arg \c "1-3:20" is interpreted as 1 day, 3 hours and 20 minute
	 * @param durationString string that contains the time interval
	 * @param interval result of the conversion, the rapresentation is in 100ns units
	 * @param complete if true the input string must contains exactly the expected fields, otherwise an error is declared. Days field remains optional.
	 * @param dateDelimiter the char that has to be considered as the field separator for the date part of the time epoch 
	 * @param timeDelimiter the char that has to be considered as field separator for the time interval format
	 * @return the result of the conversion, true if the conversion could be done. Hours, minutes and seconds are checked for the standard ranges, days are required to be positive.
	 */
	static bool strToInterval(const IRA::CString& durationString,ACS::TimeInterval& interval,bool complete=false,char dateDelimiter='-',char timeDelimiter=':');

	/**
	 * Use this function to convert an ACS interval notation to a string(days-hh:mm:ss.ss).
	 * @param interval the rapresentation in 100ns units of the time to be converted    
	 * @param outString string that contains the time inteval as it is converted 
	 * @param dateDelimiter the char that has to be considered as the field separator for the date part of the time epoch 
	 * @param timeDelimiter the char that has to be considered as field separator for the time interval format
	 * @return the result of the conversion, true if the conversion could be done
	 */
	static bool intervalToStr(const ACS::TimeInterval& interval,IRA::CString& outString,char dateDelimiter='-',char timeDelimiter=':');

	/**
	 * Use this function to convert a string containing a time epoch (yyyy-ddd-hh:mm:ss.ss) into an ACS time notation. In case <i>complete</i> argument is not set, not all the expected fields 
	 * have to be  included, the function will try to convert the string anyway. The missing fields are replaced with the corresponding fields of the current date and time. For example 
	 * if the date is missing the year indication, the current year is taken; if the time part reports just one numebr it is intended as the hour value, minutes and seconds are considered zero. 
	 * @param timeString string that contains the time epoch to be converted
	 * @param time result of the conversion, the rapresentation is in 100ns units
	 * @param complete if true the input string must contains exactly the expected fields, otherwise an erro is declared. Year field remains optional. 
	 * @param dateDelimiter the char that has to be considered as the field separator for the date part of the time epoch
	 * @param timeDelimiter the char that has to be considered as the field separator for the time part of the time epoch
	 * @return the result of the conversion, true if the conversion could be done. All fields are checked to lie in the standard ranges.
	 */	
	static bool strToTime(const IRA::CString& timeString,ACS::Time& time,bool complete=false,char dateDelimiter='-',char timeDelimiter=':');
	
	/**
	 * Use this function to convert an ACS time epoch notation to a string(yyyy-ddd-hh:mm:ss.ss).
	 * @param time the rapresentation in 100ns units of the time epoch to be converted    
	 * @param outString string that contains the time as it is converted
	 * @param dateDelimiter the char that has to be considered as the field separator for the date part of the time epoch
	 * @param timeDelimiter the char that has to be considered as the field separator for the time part of the time epoch
	 * @return the result of the conversion, true if the conversion could be done
	 */
	static bool timeToStr(const ACS::Time& time,IRA::CString& outString,char dateDelimiter='-',char timeDelimiter=':');
	
	/**
	 * Use this function to convert a string containing an angle in the format (+/-hh:mm:ss.ss) into a radians. If <i>complete</i> argument is not set, the expected fields have not to be all  
	 * included, the function will try to convert the string anyway. The missing fields are replaced with zeros, the first value is always interpreted as the hours value. 
	 * @param angle string that contains the hour angle
	 * @param rad result of the conversion normalized into the range -2PI..2PI
	 * @param complete if true the input string must contains exactly the expected fields, otherwise an error is declared. 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. Standard checks are performed. No normalization is done, 25 hours will result in an error.
	 */	
	static bool hourAngleToRad(const IRA::CString& angle,double& rad,bool complete=false,char delimiter=':');
	
	/**
	 * Use this function to convert a radian into a string according the format (+/-hh:mm:ss.ss).
	 * @param rad angle in radians    
	 * @param outString string that contains the result of the conversion  
	 * @param delimiter the char that has to be considered as the field separator for hour angle
	 * @return the result of the conversion, true if the conversion could be done
	 */
	static bool radToHourAngle(const double& rad,IRA::CString& outString,char delimiter=':');

	/**
	 * Use this function to convert a string containing an angle in the sexasegimal format (+/-dd:mm:ss.ss) into radians. If <i>complete</i> argument is not set, the expected fields have not to be  
	 * all included, the function will try to convert the string anyway. The missing fields are replaced with zeros. If one value is provided it will be considered to
	 * be the degrees value, then the arcminute and so on.   
	 * @param angle string that contains the sexagesimal angle
	 * @param rad result of the conversion, the result is always normalized into -2PI...2PI
	 * @param complete if true the input string must contains exactly the expected fields, otherwise an erro is declared 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. arcminutes and arcseconds are check to lie in 0..60 range. degrees can be in any range but the are normalized.
	 */		
	static bool sexagesimalAngleToRad(const IRA::CString& angle,double& rad,bool complete=false,char delimiter=':');

	/**
	 * Use this function to convert a radian into a string according the sexasegimal format (+/-dd:mm:ss.ss).
	 * @param rad angle in radians, before converting it is normalized into the range -2PI..2PI.
	 * @param outString string that contains the result of the conversion
	 * @param delimiter the char that has to be considered as the field separator for hour angle
	 * @return the result of the conversion, true if the conversion could be done
	 */
	static bool radToSexagesimalAngle(const double& rad,IRA::CString& outString,char delimiter=':');
	
	/**
	 * Use this function to convert a string containing an angle (+/-dd.ddd) into radians.    
	 * @param angle string that contains the angle in degrees
	 * @param rad result of the conversion, the result is always normalized into -2PI...2PI
	 * @return the result of the conversion, true if the conversion could be done. 
	 */		
	static bool angleToRad(const IRA::CString& angle,double& rad);

	/**
	 * Use this function to convert a radian into a string according format (+/-dd.ddd).
	 * @param rad angle in radians, before converting it is normalized into the range -2PI..2PI.
	 * @param outString string that contains the result of the conversion
	 * @return the result of the conversion, true if the conversion could be done
	 */
	static bool radToAngle(const double& rad,IRA::CString& outString);

	/**
	 * Use this function to convert a string containing longitude in the sexasegimal format (+/-dd:mm:ss.ss) or (+/-dd.ddd'd') into radians. The angles must be into the range -180...+180. 
	 * if <i>complete</i> argument is not set, the expected fields have not to be all included, the function will try to convert the string anyway. The missing fields are 
	 * replaced with zeros. If one value is provided it will be considered to be the degrees value then the arcminute and so on.
	 * @param lon string that contains the longitude
	 * @param rad result of the conversion, the result is always normalized into -PI...PI
	 * @param complete if true the input string must contains exactly the expected fields and lie into expected ranges, otherwise an error is declared 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. Arcminutes and arcseconds are check to lie in 0..60 range. The resulting angle cannot exceeds +/- 180°.
	 */		
	static bool longitudeToRad(const IRA::CString& lon,double& rad,bool complete=false,char delimiter=':');
	
	/**
	 * Use this function to convert a string containing a latitude in the sexasegimal format (+/-dd:mm:ss.ss) or (+/-dd.ddd'd') into radians. The angles must be into the range -90...+90. 
	 * if <i>complete</i> argument is not set, the expected fields have not to be all included, the function will try to convert the string anyway. The missing fields are 
	 * replaced with zeros. If one value is provided it will be considered to be the degrees value then the arcminute and so on.
	 * @param lat string that contains the latitude
	 * @param rad result of the conversion, the result is always normalized into -PI/2...PI/2
	 * @param complete if true the input string must contains exactly the expected fields and lie into expected ranges, otherwise an error is declared 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. Arcminutes and arcseconds are check to lie in 0..60 range. The resulting angle cannot exceeds +/- 90°.
	 */			
	static bool latitudeToRad(const IRA::CString& lat,double& rad,bool complete=false,char delimiter=':');
	
	/**
	 * Use this function to convert a string containing a right ascension in the hour angle format (+/-hh:mm:ss.ss'h') or (+/-dd.ddd'd') or (+/-dd::mm:ss.ss) into radians. 
	 * The angles must be into the range 0...24h or 00:00:00...359:59:59.999 or 0..360'd'. if <i>complete</i> argument is not set, the expected fields have not to be all included, the function will try to convert the string anyway. The missing fields are 
	 * replaced with zeros. If one value is provided it will be considered to be the hours value then the arcminute and so on.
	 * @param ra string that contains the right ascension
	 * @param rad result of the conversion, the result is always normalized into 0...2PI
	 * @param complete if true the input string must contains exactly the expected fields and lie into expected ranges, otherwise an error is declared 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. Arcminutes and arcseconds are check to lie in 0..60 range. The resulting angle cannot exceeds +/- 90°.
	 */				
	static bool rightAscensionToRad(const IRA::CString& ra,double& rad,bool complete=false,char delimiter=':');
	
	/**
	 * Use this function in order to convert a declination into radians. The accepted formats are the normal longitude sexagesimal angle "+/-dd:mm:ss.sss" or
	 * as degrees "+/-ddd.ddd". In case of the latter format a character 'd' must be postponed. The resulting value is checked to be in the range -PI/2..PI/2. 
	 * @param dec string that contains the angle right ascension angle
	 * @param rad result of the conversion
	 * @param complete if true the input string must contains exactly the expected fields and lie into expected ranges, otherwise an error is declared 
	 * @param delimiter the char that has to be considered as the field separator for sexagesimal angle fields
	 * @return the result of the conversion, true if the conversion could be done
	 */
	 static bool declinationToRad(const IRA::CString& dec,double& rad,bool complete=false,char delimiter=':');
	
	/**
	 * Use this function to convert a string containing galactic longitude in the sexasegimal format (dd:mm:ss.ss) or (dd.ddd'd') into radians. The angles must be into the range 0...360. 
	 * if <i>complete</i> argument is not set, the expected fields have not to be all included, the function will try to convert the string anyway. The missing fields are 
	 * replaced with zeros. If one value is provided it will be considered to be the degrees value then the arcminute and so on.
	 * @param lon string that contains the longitude
	 * @param rad result of the conversion, the result is always normalized into 0...2PI
	 * @param complete if true the input string must contains exactly the expected fields and lie into expected ranges, otherwise an error is declared 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. Arcminutes and arcseconds are check to lie in 0..60 range.
	 */			 
	 static bool galLongitudeToRad(const IRA::CString& lon,double& rad,bool complete=false,char delimiter=':'); 

	/**
	 * Use this function to convert a string containing galactic latitude in the sexasegimal format (+/-dd:mm:ss.ss) or (+/-dd.ddd'd') into radians. The angles must be into the range -90...+90. 
	 * if <i>complete</i> argument is not set, the expected fields have not to be all included, the function will try to convert the string anyway. The missing fields are 
	 * replaced with zeros. If one value is provided it will be considered to be the degrees value then the arcminute and so on.
	 * @param lon string that contains the longitude
	 * @param rad result of the conversion, the result is always normalized into -PI/2...PI/2
	 * @param complete if true the input string must contains exactly the expected fields and lie into expected ranges, otherwise an error is declared 
	 * @param delimiter the char that has to be considered as the field separator
	 * @return the result of the conversion, true if the conversion could be done. Arcminutes and arcseconds are check to lie in 0..60 range. 
	 */			 
	 static bool galLatitudeToRad(const IRA::CString& lat,double& rad,bool complete=false,char delimiter=':'); 
	 
	/**
	 * Use this function to convert a string containing azimuth (dd.ddd) into radians. The angles must be into the range 0...360. 
	 * @param az string that contains the azimuth
	 * @param rad result of the conversion, the result is always normalized into 0...2PI
	 * @param complete if set the value is checked to be in expected range
	 * @return the result of the conversion, true if the conversion could be done.
	 */			 	 
	 static bool azimuthToRad(const IRA::CString& az,double& rad,bool complete=false);

	/**
	 * Use this function to convert a string containing elevation (dd.ddd) into radians. The angles must be into the range 0...90. 
	 * @param el string that contains the elevation
	 * @param rad result of the conversion, the result is always normalized into 0...PI/2
	 * @param complete if set the value is checked to be in expected range 
	 * @return the result of the conversion, true if the conversion could be done.
	*/			 	 
	 static bool elevationToRad(const IRA::CString& el,double& rad,bool complete=false);
		
	 /**
	  * This function is specific to parse coordinate offsets in any given frame. No range checks are done. The format is supposed to be in degrees in both
	  * sexasegimal (+/-dd:mm:ss.ss) od decimal (+/-dd.dddd'd') form.
	  * @param offset string that contains the offset to be parsed
	  * @param rad result of the conversion in radians
	  * @param delimiter the char that has to be considered as the field separator for sexagesimal format.
	  * @return the result of the conversion, true if the conversion could be done. 
	  */
	 static bool offsetToRad(const IRA::CString& offset,double& rad,char delimiter=':');
};
	
}

#endif
