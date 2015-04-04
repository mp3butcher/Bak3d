/*-----------------------------------------------------------------------
    Copyright (c) 2013, Tristan Lorach. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to lorachnroll@gmail.com (Tristan Lorach)
*/ //--------------------------------------------------------------------
#pragma warning (disable : 4244)//conversion from 'double' to 'float', possible loss of data
#define _CRT_SECURE_NO_DEPRECATE // we have still fopen() etc functions, here...

#include "curvereader.h"
#include <algorithm>
#include "nv_math.h"

#define Deg2Rad 0.0174532925199432958
#define FAILURE(msg) \
{\
	printf(msg);\
	goto failure;\
}

#ifndef LOGMSG
#	define LOG_MSG stdout
#	define LOG_WARN stdout
#	define LOG_ERR stderr
#define LOGMSG fprintf
#endif
/*----------------------------------------------------------------------------------*/ /**

Construct the 'vector' : 
- allocate for the name
- allocate room for the curves
.

**/ //----------------------------------------------------------------------------------
CurveVector::CurveVector(const char * name, int dim)
{
    m_ON = true;
	m_time = 0;
	m_bindPtr[0] = m_bindPtr[1] = m_bindPtr[2] = m_bindPtr[3] = NULL;
	m_cvvals[0] = m_cvvals[1] = m_cvvals[2] = m_cvvals[3] = 0.0f;
	m_cvvalsBase[0] = m_cvvalsBase[1] = m_cvvalsBase[2] = m_cvvalsBase[3] = 0.0f;
	if(dim < 1) dim = 1;
	if(dim > 4) dim = 4;
	//
	// setup the name
	//
	if(name)
	{
		m_name = new char[strlen(name)+1];
		strcpy_s(m_name, strlen(name)+1, name);
	}
	//
	// setup the curves
	//
	m_dim = dim;
	for(int i=0; i<4; i++)
	{
		if(i < dim)	m_cvs[i] = new CurveReader;
		else m_cvs[i] = NULL;
	}
    unbindPtrs();
}
CurveReader *CurveVector::getCurve(int n) 
{ 
	if(n < m_dim) 
		return m_cvs[n]; 
	return NULL;
}

CurveVector::~CurveVector()
{
	if(m_name)
		delete [] m_name;
	for(int i=0; i<m_dim; i++)
	{
		if(m_cvs[i])
			delete m_cvs[i];
		m_cvs[i] = NULL;
	}

}

bool CurveVector::find(float time, int *indexx, int *indexy, int *indexz, int *indexw)
{
	bool bres = true;
	int idx[4];
	for(int i=0; i<m_dim; i++)
	{
		bool b;
		b = m_cvs[i]->find(time, &(idx[i]));
		if(!b)
			bres = false;
	}
	if(indexx) *indexx = idx[0];
	if(indexy) *indexy = idx[1];
	if(indexz) *indexz = idx[2];
	if(indexw) *indexw = idx[3];
	return bres;
}
float CurveVector::evaluate(float time, float *v1, float *v2, float *v3, float *v4)
{
    bool bOnlyforBinding;
    if(m_ON == false)
        return 0.0f;
    if(v1||v2||v3||v4)
            bOnlyforBinding = false;
    else    bOnlyforBinding = true;
	for(int i=0; i<m_dim; i++)
	{
        if(bOnlyforBinding && (m_bindPtr[i]==NULL))
            continue;
		m_cvvals[i] = m_cvs[i]->evaluate(time);
        if(m_bindPtr[i]) 
        {
            *m_bindPtr[i]= m_cvvals[i] + m_cvvalsBase[i];
            if(m_pDirty) *m_pDirty = true;
        }
	}
	if(v1) *v1= m_cvvals[0] + m_cvvalsBase[0];
	if(v2) *v2= m_cvvals[1] + m_cvvalsBase[1];
	if(v3) *v3= m_cvvals[2] + m_cvvalsBase[2];
	if(v4) *v4= m_cvvals[3] + m_cvvalsBase[3];
	
	return m_cvvals[0];
}
float CurveVector::getClosestKeyTime(float time, bool backward)
{
    float tclosest = time;
	for(int i=0; i<m_dim; i++)
	{
		float t = m_cvs[i]->getClosestKeyTime(time, backward);
        if((fabs(t-time) < fabs(tclosest-time))||( tclosest == time))
            tclosest = t;
	}
    return tclosest;
}
float CurveVector::evaluateInfinities(float time, bool evalPre, float *v1, float *v2, float *v3, float *v4)
{
    bool bOnlyforBinding;
    if(m_ON == false)
        return 0.0f;
    if(v1||v2||v3||v4)
            bOnlyforBinding = false;
    else    bOnlyforBinding = true;
	for(int i=0; i<m_dim; i++)
	{
        if(bOnlyforBinding && (m_bindPtr[i]==NULL))
            continue;
		m_cvvals[i] = m_cvs[i]->evaluateInfinities(time, evalPre);
        if(m_bindPtr[i]) {
            *m_bindPtr[i]= m_cvvals[i] + m_cvvalsBase[i];
            if(m_pDirty) *m_pDirty = true;
        }
	}
	if(v1) *v1= m_cvvals[0] + m_cvvalsBase[0];
	if(v2) *v2= m_cvvals[1] + m_cvvalsBase[1];
	if(v3) *v3= m_cvvals[2] + m_cvvalsBase[2];
	if(v4) *v4= m_cvvals[3] + m_cvvalsBase[3];
	return m_cvvals[0];
}
void CurveVector::startKeySetup(bool inputIsTime, bool outputIsAngular, bool isWeighted,
								EtInfinityType preinftype, EtInfinityType postinftype)
{
	if(m_cvx) m_cvx->startKeySetup(inputIsTime, outputIsAngular, isWeighted, preinftype, postinftype);
	if(m_cvy) m_cvy->startKeySetup(inputIsTime, outputIsAngular, isWeighted, preinftype, postinftype);
	if(m_cvz) m_cvz->startKeySetup(inputIsTime, outputIsAngular, isWeighted, preinftype, postinftype);
	if(m_cvw) m_cvw->startKeySetup(inputIsTime, outputIsAngular, isWeighted, preinftype, postinftype);
}
/**

This method is an easy way to create a new key: sames parameters for all components of the vector...

Furthermore curves are compliled again in this method

 **/
void CurveVector::addKey(float frame, float x, float y, float z, float w, 
			EtTangentType inTangentType, EtTangentType outTangentType, 
			float inAngle, float inWeight, float outAngle, float outWeight)
{
	if(m_cvx) m_cvx->addKey(frame, x, inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvy) m_cvy->addKey(frame, y, inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvz) m_cvz->addKey(frame, z, inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvw) m_cvw->addKey(frame, w, inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvx) m_cvx->endKeySetup();
	if(m_cvy) m_cvy->endKeySetup();
	if(m_cvz) m_cvz->endKeySetup();
	if(m_cvw) m_cvw->endKeySetup();
}
/**

This method is an easy way to create a new key: sames parameters for all components of the vector...
And we take the current time and values in order to create the key.
Furthermore curves are compliled again in this method

 **/
void CurveVector::addKeyHere(EtTangentType inTangentType, EtTangentType outTangentType, 
			float inAngle, float inWeight, float outAngle, float outWeight)
{
	float frame = m_time * m_cvx->m_frameRate;
	m_cvx->addKey(frame, m_cvvals[0], inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvy) m_cvy->addKey(frame, m_cvvals[1], inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvz) m_cvz->addKey(frame, m_cvvals[2], inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvw) m_cvw->addKey(frame, m_cvvals[3], inTangentType, outTangentType, inAngle, inWeight, outAngle, outWeight);
	if(m_cvx) m_cvx->endKeySetup();
	if(m_cvy) m_cvy->endKeySetup();
	if(m_cvz) m_cvz->endKeySetup();
	if(m_cvw) m_cvw->endKeySetup();
}

/*----------------------------------------------------------------------------------*/ /**

Clear the curve

**/ //----------------------------------------------------------------------------------
void CurveVector::clear(int n)
{
	if(n < 0)
		for(int i=0; i<m_dim; i++)
		{
			assert(m_cvs[i]);
			m_cvs[i]->clear();
		}
	else if(n < m_dim)
	{
		m_cvs[n]->clear();
	}
}

#pragma message(__FILE__"("S__LINE__"):>>>>>>>TODO - CurveVector::bindPtrs")
void CurveVector::bindPtrs(float *v1, float *v2, float *v3, float *v4, unsigned char *pDirty)
{
    int i=0;
    if(pDirty) m_pDirty = pDirty;
    if(m_cvs[i] && v1)
    {
        if(m_cvs[i]->m_keys.size() > 0)
            m_bindPtr[i] = v1;
    }
    i++;
    if(m_cvs[i] && v2)
    {
        if(m_cvs[i]->m_keys.size() > 0)
            m_bindPtr[i] = v2;
    }
    i++;
    if(m_cvs[i] && v3)
    {
        if(m_cvs[i]->m_keys.size() > 0)
            m_bindPtr[i] = v3;
    }
    i++;
    if(m_cvs[i] && v4)
    {
        if(m_cvs[i]->m_keys.size() > 0)
            m_bindPtr[i] = v4;
    }
}
void CurveVector::unbindPtrs()
{
    m_pDirty = NULL;
    memset(m_bindPtr, 0, sizeof(float)*4);
}

/*----------------------------------------------------------------------------------*/ /**



**/ //----------------------------------------------------------------------------------
CurveVector *CurvePool::getCV(const char *  name) 
{
	CurveMapType::const_iterator icv;
	icv = m_curves.find(name);
	if(icv == m_curves.end())
		return NULL;
	return icv->second; 
};

/*----------------------------------------------------------------------------------*/ /**

Create a new Curve-vector

**/ //----------------------------------------------------------------------------------
CurveVector *CurvePool::newCV(const char *  name, int dim)
{
	CurveVector *newcv;
	newcv = new CurveVector(name, dim);
	m_curves[newcv->m_name] = newcv;
    m_curvesVec.push_back(newcv);
	return newcv;
}
/*----------------------------------------------------------------------------------*/ /**

Create a new Curve-Quat

**/ //----------------------------------------------------------------------------------
CurveQuat *CurvePool::newQuatCV(const char *  name)
{
	CurveQuat *newcv;
	newcv = new CurveQuat(name);
	m_mapquatcurves[newcv->m_name] = newcv;
    m_quatcurves.push_back(newcv);
	return newcv;
}
//
/// converting string into enum
//
bool checkInfinity(const char * str, EtInfinityType &it)
{
	if(!str)
	{
		it = kInfinityConstant;
		return false;
	}
	if(!strcmp(str , "constant"))
		it = kInfinityConstant;
	else if(!strcmp(str , "cycle"))
		it = kInfinityCycle;
	else if(!strcmp(str , "cyclerelative"))
		it = kInfinityCycleRelative;
	else if(!strcmp(str , "linear"))
		it = kInfinityLinear;
	else if(!strcmp(str , "oscillate"))
		it = kInfinityOscillate;
	else 
	{
		it = kInfinityConstant;
		return false;
	}
	return true;
}
//
/// converting string into enum
//
bool AsTangentType(const char * str, EtTangentType &tt)
{
	if(!str)
	{
		tt = (kTangentSmooth);
		return false;
	}
	if(!strcmp(str , "fixed"))
		tt = (kTangentFixed);
	else if(!strcmp(str , "linear"))
		tt = (kTangentLinear);
	else if(!strcmp(str , "flat"))
		tt = (kTangentFlat);
	else if(!strcmp(str , "smooth"))
		tt = (kTangentSmooth);
	else if(!strcmp(str , "step"))
		tt = (kTangentStep);
	else if(!strcmp(str , "slow"))
		tt = (kTangentSlow);
	else if(!strcmp(str , "fast"))
		tt = (kTangentFast);
	else if(!strcmp(str , "clamped"))
		tt = (kTangentClamped);
	else
	{
		tt = (kTangentSmooth);
		return false;
	}
	return true;
}
/*----------------------------------------------------------------------------------*/ /**

Create a new Curve-vector from a file

**/ //----------------------------------------------------------------------------------
CurveVector *CurvePool::newCVFromFile(const char *  fname, char *  overloadname)
{
	CurveVector *newcv = NULL;
	CurveVector *firstcv = NULL;
	int num,dim,a,b,c, res;
	int comp, i,k;
	float frm,val,f1,f2,f3,f4;
	char *  finalname;
	char name2[60];
	char s1[60];
	char s2[60];
	FILE *fd;
	fd = fopen(fname, "r");
	if(!fd) 
		return NULL;
	for(k=0;;k++)
	{
		res = fscanf(fd, "name:%s keys:%d dim:%d\n", name2, &num, &dim);
		if(res != 3)
			break;
		if(overloadname && (k==0))
			finalname = overloadname;
		else
			finalname = name2;
		newcv = new CurveVector(finalname, dim);
		if(k==0) firstcv = newcv;
        std::vector<CurveVector*>::iterator iCVVec = m_curvesVec.begin();
	    CurveMapType::iterator iCv;
	    if((iCv = m_curves.find(newcv->m_name)) != m_curves.end())
	    {
		    CurveVector* pcv = iCv->second;
		    m_curves.erase(iCv);
            while(iCVVec != m_curvesVec.end()) // need to also remove in the vector
            {
                if(*iCVVec == pcv)
                {
                    m_curvesVec.erase(iCVVec);
                    break;
                }
                ++iCVVec;
            }
		    delete pcv;
	    }
		m_curves[newcv->m_name] = newcv;
        m_curvesVec.push_back(newcv);
		res = fscanf(fd, "inputistime:%d outputisangular:%d isweighted:%d preinftype:%s postinftype:%s\n"	, &a,&b,&c, s1, s2);
		if(res != 5)
		{
			LOGMSG(LOG_ERR, "error while parsing curve %d at 'inputistime: outputisangular: isweighted: preinftype: postinftype:'", k);
			return NULL;
		}
		EtInfinityType it1;
		EtInfinityType it2;
		checkInfinity(s1, it1);
		checkInfinity(s2, it2);
		for(i=0; i<dim; i++)
		{
			CurveReader *cr = newcv->getCurve(i);
			cr->startKeySetup(a?true:false, b?true:false, c?true:false,	it1, it2);
		}
		for(i=0; i<num; i++)
		{
			res = fscanf(fd, "comp:%d frm:%f val:%f %s %s %d %d %d %f %f %f %f\n",
				&comp, &frm, &val, s1, s2, &a, &b, &c, &f1, &f2, &f3, &f4);
			if(res != 12)
			{
				for(i=0; i<dim; i++)
				{
					CurveReader *cr = newcv->getCurve(i);
					cr->endKeySetup();
				}
				fclose(fd);
				LOGMSG(LOG_ERR, "error while parsing curve %d at key %d", k,i);
				return newcv;
			}
			CurveReader *cr = newcv->getCurve(comp);
			if(cr==NULL)
				continue;
			EtTangentType tt1, tt2;
			AsTangentType(s1, tt1);
			AsTangentType(s2, tt2);
			cr->addKey(frm, val, tt1, tt2, f1, f2, f3, f4);
            if(frm > endTime)
                endTime = frm;
		}
		for(i=0; i<dim; i++)
		{
			CurveReader *cr = newcv->getCurve(i);
			cr->endKeySetup();
		}
	}
	fclose(fd);
	if(!firstcv)
	{
		LOGMSG(LOG_ERR, "no curves could be parsed");
	}
	return firstcv;
}
/*----------------------------------------------------------------------------------*/ /**

Destructor : releasing the curves

**/ //----------------------------------------------------------------------------------
CurvePool::~CurvePool()
{
	clear();
}
/*----------------------------------------------------------------------------------*/ /**

Clear the container

**/ //----------------------------------------------------------------------------------
void CurvePool::clear()
{
	CurveMapType::iterator icv;
	icv = m_curves.begin();
	while(icv != m_curves.end())
	{
		CurveVector *cv = icv->second;
		cv->clear();
		delete icv->second;
		m_curves.erase(icv);
		icv = m_curves.begin();
	}
    m_curvesVec.clear();
}
/*----------------------------------------------------------------------------------*/ /**

Evaluate all curves.
So we assume some float ptrs got bound to them...

**/ //----------------------------------------------------------------------------------
void CurvePool::updateAll(float time)
{
    curTime = time;
	CurveMapType::iterator icv;
	icv = m_curves.begin();
	while(icv != m_curves.end())
	{
		CurveVector *cv = icv->second;
        if(cv->m_ON)
		    cv->evaluate(time);
        ++icv;
	}
	CurveQuatMapType::iterator iqcv;
	iqcv = m_mapquatcurves.begin();
	while(iqcv != m_mapquatcurves.end())
	{
		CurveQuat *cv = iqcv->second;
        if(cv->m_ON)
    		cv->evaluate(time);
        ++iqcv;
	}
}
void CurvePool::updateAllInfinities(float time, bool evalPre)
{
    curTime = time;
	CurveMapType::iterator icv;
	icv = m_curves.begin();
	while(icv != m_curves.end())
	{
		CurveVector *cv = icv->second;
        if(cv->m_ON)
		    cv->evaluateInfinities(time, evalPre);
	}
}
void CurvePool::updateToClosestKey(bool backward)
{
    bool bFirstTime = true;
    float newTime = 0.0;
	CurveMapType::iterator icv;
	icv = m_curves.begin();
    curTime+= backward ? -1.0f : 1.0f;
	while(icv != m_curves.end())
	{
		CurveVector *cv = icv->second;
		float t = cv->getClosestKeyTime(curTime, backward);
        if(bFirstTime 
          || (backward   && (newTime > t))
          ||((!backward) && (newTime < t)) )
        {
            newTime = t;
            bFirstTime = false;
        }
        ++icv;
	}
	CurveQuatMapType::iterator iqcv;
	iqcv = m_mapquatcurves.begin();
	while(iqcv != m_mapquatcurves.end())
	{
		CurveQuat *cv = iqcv->second;
		float t = cv->getClosestKeyTime(curTime, backward);
        if(bFirstTime 
          || (backward   && (newTime > t))
          ||((!backward) && (newTime < t)) )
        {
            newTime = t;
            bFirstTime = false;
        }
        ++iqcv;
	}
    updateAll(newTime);
}
/*----------------------------------------------------------------------------------*/ /**

Misc initialization.

**/ //----------------------------------------------------------------------------------
CurveReader::CurveReader()
{
	m_unitConversion = 1.0;
	m_frameRate = 1.0;//24.0;
}
void CurveReader::clear()
{
	m_keys.clear();
	m_keyList.clear();
	m_numKeys = 0;
}
/*----------------------------------------------------------------------------------*/ /**
		A static helper function to assemble an EtCurve animation curve
	from a linked list of heavy-weights keys

  Input Arguments:

		- keys					the vector of keys
		- EtBoolean isWeighted		Whether or not the curve has weighted tangents
		- EtBoolean useOldSmooth		Whether or not to use pre-Maya2.0 smooth
									tangent computation
		.

	Note:

		This function will also free the memory used by the heavy-weight keys
**/ //----------------------------------------------------------------------------------/*
bool CurveReader::assembleAnimCurve(vector<ReadKey> &keys, bool isWeighted, bool useOldSmooth)
{
	unsigned int	index;
	Key			thisKey;
	ReadKey *prevKey = NULL;
	ReadKey *key = NULL;
	ReadKey *nextKey = NULL;
	float		py, ny, dx;
	bool		hasSmooth;
	float		length;
	float		inLength, outLength, inSlope, outSlope;
	float		inTanX, inTanY, outTanX, outTanY;
	float		inTanXs, inTanYs, outTanXs, outTanYs;

	/* make sure we have useful information */
	if (keys.empty())
		return false;

    inTanX = inTanY = outTanX = outTanY = 0.0f;
	m_keyList.clear();
	m_numKeys = (int)keys.size();
	/* initialise the cache */
	m_lastKey = NULL;
	m_lastIndex = -1;
	m_lastInterval = -1;
	m_isStep = false;

	/* compute tangents */
	index = 0;
	nextKey = &(keys[0]);
	//while (index < keys.size()) 
	while (nextKey != NULL)
	{
		prevKey = key;
		key = nextKey;
		nextKey = (index+1) < keys.size() ? &(keys[index+1]) : NULL;
		/*prevKey = &(keys[index]);
		key = NULL;
		nextKey = NULL;

		if(index+1 < keys.size())
			key = &(keys[index+1]);
		if(index+2 < keys.size())
			nextKey = &(keys[index+2]);*/

		/* construct the final EtKey (light-weight key) */
		thisKey.time = key->time;
		thisKey.value = key->value;

		/* compute the in-tangent values */
		/* kTangentClamped */
		if ((key->inTangentType == kTangentClamped) && (prevKey != NULL)) {
			py = prevKey->value - key->value;
			if (py < 0.0) py = -py;
			ny = (nextKey == NULL ? py : nextKey->value - key->value);
			if (ny < 0.0) ny = -ny;
			if ((ny <= 0.05) || (py <= 0.05)) {
				key->inTangentType = kTangentFlat;
			}
		}
		hasSmooth = false;
		switch (key->inTangentType) {
		case kTangentFixed:
			inTanX = key->inWeight * (float)cosf (key->inAngle) * 3.0f;
			inTanY = key->inWeight * (float)sinf (key->inAngle) * 3.0f;
			break;
		case kTangentLinear:
			if (prevKey == NULL) {
				inTanX = 1.0;
				inTanY = 0.0;
			}
			else {
				inTanX = key->time - prevKey->time;
				inTanY = key->value - prevKey->value;
			}
			break;
		case kTangentFlat:
			if (prevKey == NULL) {
				inTanX = (nextKey == NULL ? 0.0f : nextKey->time - key->time);
				inTanY = 0.0;
			}
			else {
				inTanX = key->time - prevKey->time;
				inTanY = 0.0;
			}
			break;
		case kTangentStep:
			inTanX = 0.0;
			inTanY = 0.0;
			break;
		case kTangentSlow:
		case kTangentFast:
			key->inTangentType = kTangentSmooth;
			if (prevKey == NULL) {
				inTanX = 1.0;
				inTanY = 0.0;
			}
			else {
				inTanX = key->time - prevKey->time;
				inTanY = key->value - prevKey->value;
			}
			break;
		case kTangentSmooth:
		case kTangentClamped:
			key->inTangentType = kTangentSmooth;
			hasSmooth = true;
			break;
		}

		/* compute the out-tangent values */
		/* kTangentClamped */
		if ((key->outTangentType == kTangentClamped) && (nextKey != NULL)) {
			ny = nextKey->value - key->value;
			if (ny < 0.0) ny = -ny;
			py = (prevKey == NULL ? ny : prevKey->value - key->value);
			if (py < 0.0) py = -py;
			if ((ny <= 0.05) || (py <= 0.05)) {
				key->outTangentType = kTangentFlat;
			}
		}
		switch (key->outTangentType) {
		case kTangentFixed:
			outTanX = key->outWeight * (float)cosf (key->outAngle) * 3.0f;
			outTanY = key->outWeight * (float)sinf (key->outAngle) * 3.0f;
			break;
		case kTangentLinear:
			if (nextKey == NULL) {
				outTanX = 1.0;
				outTanY = 0.0;
			}
			else {
				outTanX = nextKey->time - key->time;
				outTanY = nextKey->value - key->value;
			}
			break;
		case kTangentFlat:
			if (nextKey == NULL) {
				outTanX = (prevKey == NULL ? 0.0f : key->time - prevKey->time);
				outTanY = 0.0;
			}
			else {
				outTanX = nextKey->time - key->time;
				outTanY = 0.0;
			}
			break;
		case kTangentStep:
			outTanX = 0.0;
			outTanY = 0.0;
			break;
		case kTangentSlow:
		case kTangentFast:
			key->outTangentType = kTangentSmooth;
			if (nextKey == NULL) {
				outTanX = 1.0;
				outTanY = 0.0;
			}
			else {
				outTanX = nextKey->time - key->time;
				outTanY = nextKey->value - key->value;
			}
			break;
		case kTangentSmooth:
		case kTangentClamped:
			key->outTangentType = kTangentSmooth;
			hasSmooth = true;
			break;
		}

		/* compute smooth tangents (if necessary) */
		if (hasSmooth) {
			if (useOldSmooth && m_isWeighted) {
				/* pre-Maya 2.0 smooth tangents */
				if ((prevKey == NULL) && (nextKey != NULL)) {
					outTanXs = nextKey->time - key->time;
					outTanYs = nextKey->value - key->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != NULL) && (nextKey == NULL)) {
					outTanXs = key->time - prevKey->time;
					outTanYs = key->value - prevKey->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != NULL) && (nextKey != NULL)) {
					/* There is a CV before and after this one */
					/* Find average of the adjacent in and out tangents */
					inTanXs = key->time - prevKey->time;
					inTanYs = key->value - prevKey->value;
					outTanXs = nextKey->time - key->time;
					outTanYs = nextKey->value - key->value;

					if (inTanXs > 0.01) {
						inSlope = inTanYs / inTanXs;
					}
					else {
						inSlope = 0.0;
					}
					inLength = (inTanXs * inTanXs) + (inTanYs * inTanYs);
					if (outTanXs > 0.01) {
						outSlope = outTanYs / outTanXs;
					}
					else {
						outSlope = 0.0;
					}
					outLength = (outTanXs * outTanXs) + (outTanYs * outTanYs);

					if ((inLength != 0.0) || (outLength != 0.0)) {
						inLength = sqrt (inLength);
						outLength = sqrt (outLength);
						outTanYs = ((inSlope * outLength) + (outSlope * inLength)) / (inLength + outLength);
						inTanYs = outTanYs * inTanXs;
						outTanYs *= outTanXs;
						/*
						// Set the In and Out tangents, at that keyframe, to be the
						// smaller (in length) off the two.
						*/
						inLength = (inTanXs * inTanXs) + (inTanYs * inTanYs);
						outLength = (outTanXs * outTanXs) + (outTanYs * outTanYs);
						if (inLength < outLength) {
							outTanXs = inTanXs;
							outTanYs = inTanYs;
						}
						else {
							inTanXs = outTanXs;
							inTanYs = outTanYs;
						}
					}
				}
				else {
					inTanXs = 1.0;
					inTanYs = 0.0;
					outTanXs = 1.0;
					outTanYs = 0.0;
				}
			}
			else {
				/* Maya 2.0 smooth tangents */
				if ((prevKey == NULL) && (nextKey != NULL)) {
					outTanXs = nextKey->time - key->time;
					outTanYs = nextKey->value - key->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != NULL) && (nextKey == NULL)) {
					outTanXs = key->time - prevKey->time;
					outTanYs = key->value - prevKey->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != NULL) && (nextKey != NULL)) {
					/* There is a CV before and after this one*/
					/* Find average of the adjacent in and out tangents. */

					dx = nextKey->time - prevKey->time;
					if (dx < 0.0001) {
						outTanYs = (float)kMaxTan;
					}
					else {
						outTanYs = (nextKey->value - prevKey->value) / dx;
					}

					outTanXs = nextKey->time - key->time;
					inTanXs = key->time - prevKey->time;
					inTanYs = outTanYs * inTanXs;
					outTanYs *= outTanXs;
				}
				else {
					inTanXs = 1.0;
					inTanYs = 0.0;
					outTanXs = 1.0;
					outTanYs = 0.0;
				}
			}
			if (key->inTangentType == kTangentSmooth) {
				inTanX = inTanXs;
				inTanY = inTanYs;
			}
			if (key->outTangentType == kTangentSmooth) {
				outTanX = outTanXs;
				outTanY = outTanYs;
			}
		}

		/* make sure the computed tangents are valid */
		if (m_isWeighted) {
			if (inTanX < 0.0) inTanX = 0.0;
			if (outTanX < 0.0) outTanX = 0.0;
		}
		else {
			if (inTanX < 0.0) {
				inTanX = 0.0;
			}
			length = sqrt ((inTanX * inTanX) + (inTanY * inTanY));
			if (length != 0.0) {	/* zero lengths can come from step tangents */
				inTanX /= length;
				inTanY /= length;
			}
			if ((inTanX == 0.0) && (inTanY != 0.0)) {
				inTanX = 0.0001f;
				inTanY = (inTanY < 0.0f ? -1.0f : 1.0f) * (inTanX * kMaxTan);
			}
			if (outTanX < 0.0) {
				outTanX = 0.0;
			}
			length = sqrt ((outTanX * outTanX) + (outTanY * outTanY));
			if (length != 0.0) {	/* zero lengths can come from step tangents */
				outTanX /= length;
				outTanY /= length;
			}
			if ((outTanX == 0.0) && (outTanY != 0.0)) {
				outTanX = 0.0001f;
				outTanY = (outTanY < 0.0f ? -1.0f : 1.0f) * (outTanX * kMaxTan);
			}
		}

		thisKey.inTanX = inTanX;
		thisKey.inTanY = inTanY;
		thisKey.outTanX = outTanX;
		thisKey.outTanY = outTanY;

		/*
		// check whether or not this animation curve is static (i.e. all the
		// key values are the same)
		*/
		if (m_isStatic) {
			if ((prevKey != NULL) && (prevKey->value != key->value)) {
				m_isStatic = false;
			}
			else if ((inTanY != 0.0) || (outTanY != 0.0)) {
				m_isStatic = false;
			}
		}
		index++;
		m_keyList.push_back(thisKey);
	}
	if (m_isStatic) {
		if ((prevKey != NULL) && (key != NULL) && (prevKey->value != key->value)) {
			m_isStatic = false;
		}
	}
	return true;
}

/**********************************************************************/ /**

 **/
/*EtTangentType CurveReader::AsTangentType (const char * str)
{
	if(str == "fixed")
		return (kTangentFixed);
	if(str == "linear")
		return (kTangentLinear);
	if(str == "flat")
		return (kTangentFlat);
	if(str == "smooth")
		return (kTangentSmooth);
	if(str == "step")
		return (kTangentStep);
	if(str == "slow")
		return (kTangentSlow);
	if(str == "fast")
		return (kTangentFast);
	if(str == "clamped")
		return (kTangentClamped);
	return (kTangentSmooth);
}
bool CurveReader::InitPlugXML(PlugValue *lpPlug)
{
	bool			isWeighted = false;
	float			unitConversion = 1.0;
	float			frameRate = 24.0;
	int i;
	Plug *p;
	String str;
	vector<ReadKey> rkeys;

	//cleanup();
	// initialise the animation curve parameters
	m_isWeighted = isWeighted;
	m_isStatic = true;
	m_preInfinity = kInfinityConstant;
	m_postInfinity = kInfinityConstant;
	//
	//====> Get the name
	//
	if(p = lpPlug->findChild("name"))
	{
		p->getValue(str);
		setName(str.c_str());
	}
	//
	//====> Get the input type
	//
	if(p = lpPlug->findChild("input"))
	{
		p->getValue(str);
		if(str == "time")
			m_inputistime = true;
	}
	//
	//====> Get the output type
	//
	if(p = lpPlug->findChild("output"))
	{
		p->getValue(str);
		if(str == "angular")
			unitConversion = Deg2Rad;
	}
	//
	//====> Get the weight type
	//
	if(p = lpPlug->findChild("weighted"))
		p->getValue(isWeighted);
	//
	//====> Get the preInfinity type
	//
	if(p = lpPlug->findChild("PreInfinity"))
	{
		p->getValue(str);
		if(str == "constant")
			m_preInfinity = kInfinityConstant;
		else if(str == "cycle")
			m_preInfinity = kInfinityCycle;
		else if(str == "cyclerelative")
			m_preInfinity = kInfinityCycleRelative;
		else if(str == "linear")
			m_preInfinity = kInfinityLinear;
		else if(str == "oscillate")
			m_preInfinity = kInfinityOscillate;
	}
	//
	//====> Get the postInfinity type
	//
	if(p = lpPlug->findChild("PostInfinity"))
	{
		p->getValue(str);
		if(str == "constant")
			m_postInfinity = kInfinityConstant;
		else if(str == "cycle")
			m_postInfinity = kInfinityCycle;
		else if(str == "cyclerelative")
			m_postInfinity = kInfinityCycleRelative;
		else if(str == "linear")
			m_postInfinity = kInfinityLinear;
		else if(str == "oscillate")
			m_postInfinity = kInfinityOscillate;
	}
	//
	//====> Find the Vertex list
	//
	if(!(p = lpPlug->findChild("Keys")))
		FAILURE("No Key table");
		char tmpstr[20];
	for(i=0;;i++)
	{
		Plug *p2,*p3;
		ReadKey k;
		//
		// find a Key
		//
		sprintf(tmpstr, "Key%d",i);
		if(!(p2 = p->findChild(tmpstr)))
			break;
		if(!(p3 = p2->findChild("frame")))
			FAILURE("no frame number");
		p3->getValue(k.time);
		k.time /= frameRate;
		if(!(p3 = p2->findChild("value")))
			FAILURE("no key value");
		p3->getValue(k.value);
		k.value *=  unitConversion;
		if(!(p3 = p2->findChild("intgtype")))
			FAILURE("no in tg type");
		p3->getValue(str);
		k.inTangentType = AsTangentType(str);
		if(!(p3 = p2->findChild("outtgtype")))
			FAILURE("no out tg type");
		p3->getValue(str);
		k.outTangentType = AsTangentType(str);
		if(k.inTangentType == kTangentFixed)
		{
			if(!(p3 = p2->findChild("intgtypeangle")))
				FAILURE("no in tangent angle");
			p3->getValue(k.inAngle);
			k.inAngle *= unitConversion;
			if(!(p3 = p2->findChild("intgtypeweight")))
				FAILURE("no in tangent weight");
			p3->getValue(k.inWeight);
		}
		if(k.outTangentType == kTangentFixed)
		{
			if(!(p3 = p2->findChild("outtgtypeangle")))
				FAILURE("no out tangent angle");
			p3->getValue(k.outAngle);
			k.outAngle *= unitConversion;
			if(!(p3 = p2->findChild("outtgtypeweight")))
				FAILURE("no out tangent weight");
			p3->getValue(k.outWeight);
		}
		rkeys.push_back(k);
	}
	//
	// Assemble keys
	//
	if (!rkeys.empty()) {
		// assemble the animation curve and add it to our channel list
		assembleAnimCurve(rkeys, isWeighted, false);
	}
	return true;
failure:
	//cleanup();
	return false;
}
*/
void CurveReader::startKeySetup(bool inputIsTime, bool outputIsAngular, bool isWeighted,
								EtInfinityType preinftype, EtInfinityType postinftype)
{
	// initialise the animation curve parameters
	m_unitConversion = 1.0;
	m_frameRate = 1.0;//24.0;
	m_isStatic = true;
	m_preInfinity = kInfinityConstant;
	m_postInfinity = kInfinityConstant;

	m_inputistime = inputIsTime;
//	if(outputIsAngular)
//			m_unitConversion = Deg2Rad;
	m_isWeighted = isWeighted;

	m_preInfinity = preinftype;
	m_postInfinity = postinftype;

//	m_keys.clear();
}
void CurveReader::getKeySetup(bool &inputIsTime, bool &outputIsAngular, bool &isWeighted,
					EtInfinityType &preinftype, EtInfinityType &postinftype)
{
	inputIsTime = m_inputistime;
	if(m_unitConversion == Deg2Rad)
		outputIsAngular = true;
	else
		outputIsAngular = false;
	isWeighted = m_isWeighted;

	preinftype = m_preInfinity;
	postinftype = m_postInfinity;
}
bool keycompare(ReadKey &k1, ReadKey &k2)
{
	if(k1.time < k2.time)
		return true;
	return false;
}

/*----------------------------------------------------------------------------------*/ /**

add some new keys:
- \c frame : the time where the key is located
- \c val : the value of the key
- \c 

**/ //----------------------------------------------------------------------------------
void CurveReader::addKey(float frame, float val, 
						EtTangentType inTangentType, EtTangentType outTangentType, 
						float inAngle, float inWeight, float outAngle, float outWeight)
{
	ReadKey k;
	k.time = frame;
	k.time /= m_frameRate; // TODO: we may want to do this LATER !!
	k.value = val;
	k.value *=  m_unitConversion; // TODO: we may want to do this LATER !!
	k.inTangentType = inTangentType;
	k.outTangentType = outTangentType;
	k.inAngle = 0;
	k.inWeight = 0;
	k.outAngle = 0;
	k.outWeight = 0;
	//if(k.inTangentType == kTangentFixed)
	{
		k.inAngle = inAngle;
		k.inAngle *= (float)Deg2Rad;//m_unitConversion; // TODO: we may want to do this LATER !!
		k.inWeight = inWeight;
	}
	//if(k.outTangentType == kTangentFixed)
	{
		k.outAngle = outAngle;
		k.outAngle *= (float)Deg2Rad;//m_unitConversion; // TODO: we may want to do this LATER !!
		k.outWeight = outWeight;
	}
	bool found = false;
	for(unsigned int i=0; i<m_keys.size(); i++)
	{
		if(m_keys[i].time == k.time)
		{
			m_keys[i] = k;
			found = true;
		}
	}
	if(!found) 
	{
		m_keys.push_back(k);
		//
		// Sort the keys
		//
		std::sort(m_keys.begin(), m_keys.end(), keycompare);
	}
}

/*----------------------------------------------------------------------------------*/ /**

finalize the curve by 'compiling it depending on the table of keys we created

**/ //----------------------------------------------------------------------------------
void CurveReader::endKeySetup()
{
	//
	// Assemble keys
	//
	if (!m_keys.empty()) 
	{
		// assemble the animation curve and add it to our channel list
		assembleAnimCurve(m_keys, m_isWeighted, false);
	}
}
/*----------------------------------------------------------------------------------*/ /**

returns a key

**/ //----------------------------------------------------------------------------------
bool CurveReader::delkey(int nkey)
{
	vector<ReadKey>::iterator iK = m_keys.begin();
	if(nkey >= (int)m_keys.size())
		return false;
	nkey--;
	for(int i=0 ; i<nkey; i++) // because & is failing !
		iK++;
	m_keys.erase(iK);
	endKeySetup();
	return true;
}
/*----------------------------------------------------------------------------------*/ /**

returns a key

**/ //----------------------------------------------------------------------------------
bool CurveReader::getKey(int n, ReadKey &k)
{
	if(n >= (int)m_keys.size()) 
		return false;
	k = m_keys[n];
	k.time *= m_frameRate;
	k.value /= m_unitConversion;
	k.inAngle /= (float)Deg2Rad;//m_unitConversion;
	k.outAngle /= (float)Deg2Rad;//m_unitConversion;
	return true;
}

/*----------------------------------------------------------------------------------*/ /*
  --------------------------------------------------------------------------------------
    class CurveQuat
  --------------------------------------------------------------------------------------
 */ //----------------------------------------------------------------------------------

CurveQuat::CurveQuat(const char * name)
{
    m_ON = true;
	m_name = NULL;
	m_time = 0.0f;
    m_cvvals[0] = m_cvvals[1] = m_cvvals[2] = m_cvvals[3] = 0.0f;
    m_bindPtr = NULL;
    m_pDirty = NULL;
	m_numKeys = 0;
	m_lastKey = NULL;
	m_lastIndex = -1;
	m_lastInterval = -1;
	m_isStep = false;
	if(name)
	{
		m_name = new char[strlen(name)+1];
		strcpy_s(m_name, strlen(name)+1, name);
	}
}
CurveQuat::~CurveQuat()
{
}
void    CurveQuat::bindPtr(float *v, unsigned char *pDirty)
{
    if(pDirty) m_pDirty = pDirty;
    m_bindPtr = v;
}
void    CurveQuat::unbindPtrs()
{
    m_pDirty = NULL;
    memset(m_bindPtr, 0, sizeof(float)*4);
}

bool CurveQuat::find (float time, int *index)
{
	int len, mid, low, high;

	/* make sure we have something to search */
	if ((index == NULL)) {
		return (false);
	}

	/* use a binary search to find the key */
	*index = 0;
	len = m_numKeys;
	if (len > 0) {
		low = 0;
		high = len - 1;
		do {
			mid = (low + high) >> 1;
			if (time < m_keys[mid].time) {
				high = mid - 1;			/* Search lower half */
			} else if (time > m_keys[mid].time) {
				low  = mid + 1;			/* Search upper half */
			}
			else {
				*index = mid;	/* Found item! */
				return (true);
			}
		} while (low <= high);
		*index = low;
	}
	return (false);
}

void    CurveQuat::evaluate(float time, float *v)
{
	bool withinInterval = false;
	Key *nextKey = NULL;
	int index = 0;
	float value = 0.0;

    if (m_lastKey != NULL) {
		if ((m_lastIndex < (m_numKeys - 1))
		&&	(time > m_lastKey->time)) {
			nextKey = &(m_keys[m_lastIndex + 1]);
			if (time == nextKey->time) {
				m_lastKey = nextKey;
				m_lastIndex++;
                memcpy(m_cvvals, m_lastKey->value, sizeof(float)*4);
                if(v) memcpy(v, m_cvvals, sizeof(float)*4);
                if(m_bindPtr) memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
                if(m_pDirty) *m_pDirty = true;
				return;
			}
			if (time < nextKey->time ) {
				index = m_lastIndex + 1;
				withinInterval = true;
			}
		}
		else if ((m_lastIndex > 0)
			&&	(time < m_lastKey->time)) {
			nextKey = &(m_keys[m_lastIndex - 1]);
			if (time > nextKey->time) {
				index = m_lastIndex;
				withinInterval = true;
			}
			if (time == nextKey->time) {
				m_lastKey = nextKey;
				m_lastIndex--;
                memcpy(m_cvvals, m_lastKey->value, sizeof(float)*4);
                if(v) memcpy(v, m_cvvals, sizeof(float)*4);
                if(m_bindPtr) memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
                if(m_pDirty) *m_pDirty = true;
				return;
			}
		}
	}

	/* it does not, so find the new segment */
	if (!withinInterval) {
		if (find (time, &index) || (index == 0)) {
			/*
				Exact match or before range of this action,
				return exact keyframe value.
			*/
			m_lastKey = &(m_keys[index]);
			m_lastIndex = index;
            memcpy(m_cvvals, m_lastKey->value, sizeof(float)*4);
            if(v) memcpy(v, m_cvvals, sizeof(float)*4);
            if(m_bindPtr) memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
            if(m_pDirty) *m_pDirty = true;
			return;
		}
		else if (index == m_numKeys) {
			/* Beyond range of this action return end keyframe value */
			m_lastKey = &(m_keys[0]);
			m_lastIndex = 0;
            memcpy(m_cvvals, m_keys[m_numKeys - 1].value, sizeof(float)*4);
            if(v) memcpy(v, m_cvvals, sizeof(float)*4);
            if(m_bindPtr) memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
            if(m_pDirty) *m_pDirty = true;
            return;
		}
	}

	/* if we are in a new segment, pre-compute and cache the bezier parameters */
	if (m_lastInterval != (index - 1)) {
		m_lastInterval = index - 1;
		m_lastIndex = m_lastInterval;
		m_lastKey = &(m_keys[m_lastInterval]);
		m_isStep = false;
		nextKey = &(m_keys[index]);
            // TODO: setup some params for smooth quat transitions
	}

	/* finally we can evaluate the segment */
	if (m_isStep) {
        memcpy(m_cvvals, m_lastKey->value, sizeof(float)*4);
        if(v) memcpy(v, m_cvvals, sizeof(float)*4);
        if(m_bindPtr) memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
        if(m_pDirty) *m_pDirty = true;
        return;
	}
	else {
        float range = nextKey->time - m_lastKey->time;
        float s = time-m_lastKey->time;
        s /= range;
		//value = lerp... TODO
        nv_math::quatf* pQ = (nv_math::quatf*)m_cvvals;
        nv_math::slerp_quats(*pQ, s
            , *(nv_math::quatf*)m_lastKey->value
            , *(nv_math::quatf*)nextKey->value );
        if(v)
            memcpy(v, m_cvvals, sizeof(float)*4);
        if(m_bindPtr)
            memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
        if(m_pDirty)
            *m_pDirty = true;
        return;
	}

    if(v) memcpy(v, m_cvvals, sizeof(float)*4);
    if(m_bindPtr) memcpy(m_bindPtr, m_cvvals, sizeof(float)*4);
    if(m_pDirty) *m_pDirty = true;
}

bool keycompare2(CurveQuat::Key &k1, CurveQuat::Key &k2)
{
	if(k1.time < k2.time)
		return true;
	return false;
}

float   CurveQuat::getClosestKeyTime(float time, bool backward)
{
	Key *nextKey;
	int index = 0;

    if (m_lastKey != NULL) {
		if ((m_lastIndex < (m_numKeys - 1))
		&&	(time >= m_lastKey->time) && (!backward)) {
			nextKey = &(m_keys[m_lastIndex + 1]);
            return nextKey->time;
		}
		else if ((m_lastIndex > 0)
			&&	(time <= m_lastKey->time)) {
			nextKey = &(m_keys[m_lastIndex - 1]);
			return nextKey->time;
		}
	}

	find (time, &index);
	if (index == m_numKeys)
		return m_keys[m_numKeys - 1].time;
	return m_keys[index].time;
}
void    CurveQuat::startKeySetup()
{
}
void    CurveQuat::endKeySetup()
{
}
void    CurveQuat::addKey(float frame, float *q)
{
	Key k;
    k.time = frame;
    memcpy(k.value, q, sizeof(float)*4);
    //k.time /= m_frameRate; // TODO: we may want to do this LATER !!
	bool found = false;
	for(unsigned int i=0; i<m_keys.size(); i++)
	{
		if(m_keys[i].time == k.time)
		{
			m_keys[i] = k;
			found = true;
		}
	}
	if(!found) 
	{
		m_keys.push_back(k);
		//
		// Sort the keys
		//
		std::sort(m_keys.begin(), m_keys.end(), keycompare2);
        m_numKeys = (int)m_keys.size();
	}
}
void    CurveQuat::addKeyHere(float *q)
{
    addKey(m_time, q);
}
void CurveQuat::clear(int n)
{
    m_keys.clear();
}
