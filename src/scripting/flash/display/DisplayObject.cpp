/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "scripting/abc.h"
#include "compat.h"
#include "swf.h"
#include "scripting/flash/display/DisplayObject.h"
#include "backends/rendering.h"
#include "backends/input.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/geom/flashgeom.h"

using namespace lightspark;
using namespace std;

ATOMIC_INT32(DisplayObject::instanceCount);

Vector2f DisplayObject::getXY()
{
	Vector2f ret;
	ret.x = getMatrix().getTranslateX();
	ret.y = getMatrix().getTranslateY();
	return ret;
}

bool DisplayObject::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, const MATRIX& m) const
{
	if(!isConstructed())
		return false;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	if(ret)
	{
		number_t tmpX[4];
		number_t tmpY[4];
		m.multiply2D(xmin,ymin,tmpX[0],tmpY[0]);
		m.multiply2D(xmax,ymin,tmpX[1],tmpY[1]);
		m.multiply2D(xmax,ymax,tmpX[2],tmpY[2]);
		m.multiply2D(xmin,ymax,tmpX[3],tmpY[3]);
		auto retX=minmax_element(tmpX,tmpX+4);
		auto retY=minmax_element(tmpY,tmpY+4);
		xmin=*retX.first;
		xmax=*retX.second;
		ymin=*retY.first;
		ymax=*retY.second;
	}
	return ret;
}

number_t DisplayObject::getNominalWidth()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	return ret?(xmax-xmin):0;
}

number_t DisplayObject::getNominalHeight()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	return ret?(ymax-ymin):0;
}

void DisplayObject::Render(RenderContext& ctxt)
{
	if(!isConstructed() || skipRender())
		return;
	renderImpl(ctxt);
}

DisplayObject::DisplayObject(Class_base* c):EventDispatcher(c),matrix(Class<Matrix>::getInstanceS(c->getSystemState())),tx(0),ty(0),rotation(0),
	sx(1),sy(1),alpha(1.0),blendMode(BLENDMODE_NORMAL),isLoadedRoot(false),ClipDepth(0),maskOf(),parent(nullptr),eventparent(nullptr),constructed(false),useLegacyMatrix(true),onStage(false),
	visible(true),mask(),invalidateQueueNext(),loaderInfo(),filters(Class<Array>::getInstanceSNoArgs(c->getSystemState())),hasChanged(true),legacy(false),cacheAsBitmap(false),
	name(BUILTIN_STRINGS::EMPTY)
{
	subtype=SUBTYPE_DISPLAYOBJECT;
//	name = tiny_string("instance") + Integer::toString(ATOMIC_INCREMENT(instanceCount));
}

DisplayObject::~DisplayObject() {}

void DisplayObject::finalize()
{
	EventDispatcher::finalize();
	maskOf.reset();
	parent=nullptr;
	eventparent =nullptr;
	mask.reset();
	matrix.reset();
	loaderInfo.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
	hasChanged = true;
}

bool DisplayObject::destruct()
{
	// TODO make all DisplayObject derived classes reusable
	maskOf.reset();
	parent=nullptr;
	eventparent =nullptr;
	mask.reset();
	matrix.reset();
	loaderInfo.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
	hasChanged = true;
	return EventDispatcher::destruct();
}

void DisplayObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleZ","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleZ),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleZ","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleZ),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(c->getSystemState(),_getZ),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(c->getSystemState(),_setZ),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(c->getSystemState(),_getVisible),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(c->getSystemState(),_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(c->getSystemState(),_getRotation),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(c->getSystemState(),_setRotation),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,name);
	c->setDeclaredMethodByQName("parent","",Class<IFunction>::getFunction(c->getSystemState(),_getParent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("root","",Class<IFunction>::getFunction(c->getSystemState(),_getRoot),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(c->getSystemState(),_getBlendMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(c->getSystemState(),_setBlendMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(c->getSystemState(),_getScale9Grid),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(c->getSystemState(),_setScale9Grid),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stage","",Class<IFunction>::getFunction(c->getSystemState(),_getStage),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(c->getSystemState(),_getMask),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(c->getSystemState(),_setMask),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(c->getSystemState(),_getAlpha),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(c->getSystemState(),_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",Class<IFunction>::getFunction(c->getSystemState(),_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getRect","",Class<IFunction>::getFunction(c->getSystemState(),_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseX","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseY","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",Class<IFunction>::getFunction(c->getSystemState(),localToGlobal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",Class<IFunction>::getFunction(c->getSystemState(),globalToLocal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTestObject","",Class<IFunction>::getFunction(c->getSystemState(),hitTestObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hitTestPoint","",Class<IFunction>::getFunction(c->getSystemState(),hitTestPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transform","",Class<IFunction>::getFunction(c->getSystemState(),_getTransform),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transform","",Class<IFunction>::getFunction(c->getSystemState(),_setTransform),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,accessibilityProperties);
	REGISTER_GETTER_SETTER(c,cacheAsBitmap);
	REGISTER_GETTER_SETTER(c,filters);
	REGISTER_GETTER_SETTER(c,scrollRect);
	REGISTER_GETTER_SETTER(c, rotationX);
	REGISTER_GETTER_SETTER(c, rotationY);
	REGISTER_GETTER_SETTER(c, opaqueBackground);

	c->addImplementedInterface(InterfaceClass<IBitmapDrawable>::getClass(c->getSystemState()));
	IBitmapDrawable::linkTraits(c);
}

ASFUNCTIONBODY_GETTER_SETTER_STRINGID(DisplayObject,name);
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,accessibilityProperties);
//TODO: Use a callback for the cacheAsBitmap getter, since it should use computeCacheAsBitmap
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,cacheAsBitmap);
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,filters);
ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,scrollRect);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, rotationX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, rotationY);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplayObject, opaqueBackground);

bool DisplayObject::computeCacheAsBitmap() const
{
	return cacheAsBitmap || (!filters.isNull() && filters->size()!=0);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getTransform)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	
	th->incRef();
	ret = asAtomHandler::fromObject(Class<Transform>::getInstanceS(sys,_MR(th)));
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setTransform)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<Transform> trans;
	ARG_UNPACK_ATOM(trans);
	if (!trans.isNull())
	{
		th->setMatrix(trans->owner->matrix);
		th->colorTransform = trans->owner->colorTransform;
	}
}

void DisplayObject::buildTraits(ASObject* o)
{
}

void DisplayObject::setMatrix(_NR<Matrix> m)
{
	bool mustInvalidate=false;
	if (m.isNull())
	{
		if (!matrix.isNull())
			mustInvalidate=true;
		matrix= NullRef;
	}
	else
	{
		SpinlockLocker locker(spinlock);
		if (matrix.isNull())
			matrix= _MR(Class<Matrix>::getInstanceS(this->getSystemState()));
		if(matrix->matrix!=m->matrix)
		{
			matrix->matrix=m->matrix;
			extractValuesFromMatrix();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate && onStage)
	{
		hasChanged=true;
		requestInvalidation(getSystemState());
	}
}

void DisplayObject::setLegacyMatrix(const lightspark::MATRIX& m)
{
	if(!useLegacyMatrix)
		return;
	bool mustInvalidate=false;
	{
		SpinlockLocker locker(spinlock);
		if (matrix.isNull())
			matrix= _MR(Class<Matrix>::getInstanceS(this->getSystemState()));
		if(matrix->matrix!=m)
		{
			matrix->matrix=m;
			extractValuesFromMatrix();
			afterSetLegacyMatrix();
			mustInvalidate=true;
		}
	}
	if(mustInvalidate && onStage)
	{
		hasChanged=true;
		requestInvalidation(getSystemState());
	}
}

void DisplayObject::becomeMaskOf(_NR<DisplayObject> m)
{
	maskOf=m;
}

void DisplayObject::setMask(_NR<DisplayObject> m)
{
	bool mustInvalidate=(mask!=m || (m && m->hasChanged)) && onStage;

	if(!mask.isNull())
	{
		//Remove previous mask
		mask->becomeMaskOf(NullRef);
	}

	mask=m;
	if(!mask.isNull())
	{
		//Use new mask
		this->incRef();
		mask->becomeMaskOf(_MR(this));
	}

	if(mustInvalidate && onStage)
	{
		hasChanged=true;
		requestInvalidation(getSystemState());
	}
}
void DisplayObject::setBlendMode(UI8 blendmode)
{
	if (blendmode <= 1 || blendmode > 14)
		this->blendMode = BLENDMODE_NORMAL;
	else
	{
		this->blendMode = (AS_BLENDMODE)(uint8_t)blendmode;
	}
}
MATRIX DisplayObject::getConcatenatedMatrix() const
{
	if(!parent)
		return getMatrix();
	else
		return parent->getConcatenatedMatrix().multiplyMatrix(getMatrix());
}

/* Return alpha value between 0 and 1. (The stored alpha value is not
 * necessary bounded.) */
float DisplayObject::clippedAlpha() const
{
	float a = alpha;
	if (!this->colorTransform.isNull())
	{
		if (alpha != 1.0 && this->colorTransform->alphaMultiplier != 1.0)
			a = alpha * this->colorTransform->alphaMultiplier /256.;
		if (this->colorTransform->alphaOffset != 0)
			a = alpha + this->colorTransform->alphaOffset /256.;
	}
	return dmin(dmax(a, 0.), 1.);
}

float DisplayObject::getConcatenatedAlpha() const
{
	if(!parent)
		return clippedAlpha();
	else
		return parent->getConcatenatedAlpha()*clippedAlpha();
}

void DisplayObject::onNewEvent()
{
	eventparent = parent;
	if (eventparent)
	{
		eventparent->incRef();
	}
}

void DisplayObject::afterHandleEvent()
{
	if (eventparent)
	{
		eventparent->decRef();
	}
	eventparent =nullptr;
}

tiny_string DisplayObject::AVM1GetPath()
{
	tiny_string res;
	if (getParent())
		res = getParent()->AVM1GetPath();
	if (this->name != BUILTIN_STRINGS::EMPTY)
	{
		if (!res.empty())
			res += ".";
		res += getSystemState()->getStringFromUniqueId(this->name);
	}
	return res;
}

MATRIX DisplayObject::getMatrix() const
{
	SpinlockLocker locker(spinlock);
	//Start from the residual matrix and construct the whole one
	MATRIX ret;
	if (!matrix.isNull())
		ret=matrix->matrix;
	ret.scale(sx,sy);
	ret.rotate(rotation*M_PI/180.0);
	ret.translate(tx,ty);
	return ret;
}

void DisplayObject::extractValuesFromMatrix()
{
	//Extract the base components from the matrix and leave in
	//it only the residual components
	assert(!matrix.isNull());
	tx=matrix->matrix.getTranslateX();
	ty=matrix->matrix.getTranslateY();
	sx=matrix->matrix.getScaleX();
	sy=matrix->matrix.getScaleY();
	rotation=matrix->matrix.getRotation();
	//Deapply translation
	matrix->matrix.translate(-tx,-ty);
	//Deapply rotation
	matrix->matrix.rotate(-rotation*M_PI/180);
	//Deapply scaling
	matrix->matrix.scale(1.0/sx,1.0/sy);
}

bool DisplayObject::skipRender() const
{
	return visible==false || clippedAlpha()==0.0 || ClipDepth;
}

void DisplayObject::defaultRender(RenderContext& ctxt) const
{
	// TODO: use scrollRect

	const CachedSurface& surface=ctxt.getCachedSurface(this);
	/* surface is only modified from within the render thread
	 * so we need no locking here */
	if(!surface.tex.isValid())
		return;

	AS_BLENDMODE bl = this->blendMode;
	if (bl == BLENDMODE_NORMAL)
	{
		DisplayObject* obj = this->getParent();
		while (obj && bl == BLENDMODE_NORMAL)
		{
			bl = obj->getBlendMode();
			obj = obj->getParent();
		}
	}
	ctxt.setProperties(bl);

	ctxt.lsglLoadIdentity();
	ctxt.renderTextured(surface.tex, surface.xOffset, surface.yOffset,
			surface.tex.width, surface.tex.height,
			surface.alpha, RenderContext::RGB_MODE);
}

void DisplayObject::computeBoundsForTransformedRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
		int32_t& outXMin, int32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight,
		const MATRIX& m) const
{
	//As the transformation is arbitrary we have to check all the four vertices
	number_t coords[8];
	m.multiply2D(xmin,ymin,coords[0],coords[1]);
	m.multiply2D(xmin,ymax,coords[2],coords[3]);
	m.multiply2D(xmax,ymax,coords[4],coords[5]);
	m.multiply2D(xmax,ymin,coords[6],coords[7]);
	//Now find out the minimum and maximum that represent the complete bounding rect
	number_t minx=coords[6];
	number_t maxx=coords[6];
	number_t miny=coords[7];
	number_t maxy=coords[7];
	for(int i=0;i<6;i+=2)
	{
		if(coords[i]<minx)
			minx=coords[i];
		else if(coords[i]>maxx)
			maxx=coords[i];
		if(coords[i+1]<miny)
			miny=coords[i+1];
		else if(coords[i+1]>maxy)
			maxy=coords[i+1];
	}
	outXMin=minx;
	outYMin=miny;
	outWidth=ceil(maxx-minx);
	outHeight=ceil(maxy-miny);
}

IDrawable* DisplayObject::invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing)
{
	//Not supposed to be called
	throw RunTimeException("DisplayObject::invalidate");
}

void DisplayObject::requestInvalidation(InvalidateQueue* q)
{
	//Let's invalidate also the mask
	if(!mask.isNull())
		mask->requestInvalidation(q);
}
//TODO: Fix precision issues, Adobe seems to do the matrix mult with twips and rounds the results, 
//this way they have less pb with precision.
void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getMatrix().multiply2D(xin, yin, xout, yout);
	if(parent)
		parent->localToGlobal(xout, yout, xout, yout);
}
//TODO: Fix precision issues
void DisplayObject::globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getConcatenatedMatrix().getInverted().multiply2D(xin, yin, xout, yout);
}

void DisplayObject::setOnStage(bool staged, bool force)
{
	//TODO: When removing from stage released the cachedTex
	if(staged!=onStage)
	{
		//Our stage condition changed, send event
		onStage=staged;
		if(staged==true)
		{
			hasChanged=true;
			requestInvalidation(getSystemState());
		}
		if(getVm(getSystemState())==NULL)
			return;
		force = true;
	}
	if (force)
	{
		/*NOTE: By tests we can assert that added/addedToStage is dispatched
		  immediately when addChild is called. On the other hand setOnStage may
		  be also called outside of the VM thread (for example by Loader::execute)
		  so we have to check isVmThread and act accordingly. If in the future
		  asynchronous uses of setOnStage are removed the code can be simplified
		  by removing the !isVmThread case.
		*/
		if(onStage==true)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"addedToStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(this,e);
			else
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),e);
			}
		}
		else if(onStage==false)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"removedFromStage"));
			if(isVmThread())
				ABCVm::publicHandleEvent(this,e);
			else
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),e);
			}
		}
	}
}

bool DisplayObject::isVisible() const
{
	if (visible)
		return parent ? parent->isVisible() : true;
	return visible;
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t val;
	ARG_UNPACK_ATOM (val);
	
	/* The stored value is not clipped, _getAlpha will return the
	 * stored value even if it is outside the [0, 1] range. */
	if(th->alpha != val)
	{
		th->alpha=val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(sys);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->alpha);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMask)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if(th->mask.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->mask->incRef();
	ret = asAtomHandler::fromObject(th->mask.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setMask)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::is<DisplayObject>(args[0]))
	{
		//We received a valid mask object
		DisplayObject* newMask=asAtomHandler::as<DisplayObject>(args[0]);
		newMask->incRef();
		th->setMask(_MR(newMask));
	}
	else
		th->setMask(NullRef);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->sx);
}

void DisplayObject::setScaleX(number_t val)
{
	//Apply the difference
	if(sx!=val)
	{
		sx=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleX(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->sy);
}

void DisplayObject::setScaleY(number_t val)
{
	//Apply the difference
	if(sy!=val)
	{
		sy=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleY(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScaleZ)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->sz);
}

void DisplayObject::setScaleZ(number_t val)
{
	//Apply the difference
	if(sz!=val)
	{
		sz=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setScaleZ)
{
	LOG(LOG_NOT_IMPLEMENTED,"DisplayObject.scaleZ is set, but not used anywhere");
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleZ(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->tx);
}

void DisplayObject::setX(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	//Apply translation, it's trivial
	if(tx!=val)
	{
		tx=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

void DisplayObject::setY(number_t val)
{
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	//Apply translation, it's trivial
	if(ty!=val)
	{
		ty=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

void DisplayObject::setZ(number_t val)
{
	LOG(LOG_NOT_IMPLEMENTED,"setting DisplayObject.z has no effect");
	
	//Stop using the legacy matrix
	if(useLegacyMatrix)
		useLegacyMatrix=false;
	//Apply translation, it's trivial
	if(tz!=val)
	{
		tz=val;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	th->setX(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->ty);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	th->setY(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getZ)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->tz);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setZ)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	th->setZ(val);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getBounds)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);

	if(asAtomHandler::is<Undefined>(args[0]) || asAtomHandler::is<Null>(args[0]))
	{
		ret =  asAtomHandler::fromObject(Class<Rectangle>::getInstanceS(sys));
		return;
	}
	if (!asAtomHandler::is<DisplayObject>(args[0]))
		LOG(LOG_ERROR,"DisplayObject.getBounds invalid type:"<<asAtomHandler::toDebugString(args[0]));
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	DisplayObject* target=asAtomHandler::as<DisplayObject>(args[0]);
	//Compute the transformation matrix
	MATRIX m;
	DisplayObject* cur=th;
	while(cur!=NULL && cur!=target)
	{
		m = cur->getMatrix().multiplyMatrix(m);
		cur=cur->parent;
	}
	if(cur==NULL)
	{
		//We crawled all the parent chain without finding the target
		//The target is unrelated, compute it's transformation matrix
		const MATRIX& targetMatrix=target->getConcatenatedMatrix();
		//If it's not invertible just use the previous computed one
		if(targetMatrix.isInvertible())
			m = targetMatrix.getInverted().multiplyMatrix(m);
	}

	Rectangle* res=Class<Rectangle>::getInstanceS(sys);
	number_t x1,x2,y1,y2;
	bool r=th->getBounds(x1,x2,y1,y2, m);
	if(r)
	{
		//Bounds are in the form [XY]{min,max}
		//convert it to rect (x,y,width,height) representation
		res->x=x1;
		res->width=x2-x1;
		res->y=y1;
		res->height=y2-y1;
	}
	else
	{
		res->x=0;
		res->width=0;
		res->y=0;
		res->height=0;
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getLoaderInfo)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);

	/* According to tests returning root.loaderInfo is the correct
	 * behaviour, even though the documentation states that only
	 * the main class should have non-null loaderInfo. */
	_NR<RootMovieClip> r=th->getRoot();
	if (r.isNull())
	{
		// if this DisplayObject is not yet added to the stage we just use the mainclip
		r = _MR(sys->mainClip);
		r->incRef();
	}
	if(r.isNull() || r->loaderInfo.isNull())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	
	r->loaderInfo->incRef();
	ret = asAtomHandler::fromObject(r->loaderInfo.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getStage)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<Stage> res=th->getStage();
	if(res.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}

	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getScale9Grid)
{
	//DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"DispalyObject.Scale9Grid");
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setScale9Grid)
{
	//DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"DispalyObject.Scale9Grid");
	asAtomHandler::setUndefined(ret);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getBlendMode)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	tiny_string res;
	switch (th->blendMode)
	{
		case BLENDMODE_LAYER: res = "layer"; break;
		case BLENDMODE_MULTIPLY: res = "multiply"; break;
		case BLENDMODE_SCREEN: res = "screen"; break;
		case BLENDMODE_LIGHTEN: res = "lighten"; break;
		case BLENDMODE_DARKEN: res = "darken"; break;
		case BLENDMODE_DIFFERENCE: res = "difference"; break;
		case BLENDMODE_ADD: res = "add"; break;
		case BLENDMODE_SUBTRACT: res = "subtract"; break;
		case BLENDMODE_INVERT: res = "invert"; break;
		case BLENDMODE_ALPHA: res = "alpha"; break;
		case BLENDMODE_ERASE: res = "erase"; break;
		case BLENDMODE_OVERLAY: res = "overlay"; break;
		case BLENDMODE_HARDLIGHT: res = "hardlight"; break;
		default: res = "normal"; break;
	}

	ret = asAtomHandler::fromString(sys,res);
}
ASFUNCTIONBODY_ATOM(DisplayObject,_setBlendMode)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	tiny_string val;
	ARG_UNPACK_ATOM(val);

	th->blendMode = BLENDMODE_NORMAL;
	if (val == "add") th->blendMode = BLENDMODE_ADD;
	else if (val == "alpha") th->blendMode = BLENDMODE_ALPHA;
	else if (val == "darken") th->blendMode = BLENDMODE_DARKEN;
	else if (val == "difference") th->blendMode = BLENDMODE_DIFFERENCE;
	else if (val == "erase") th->blendMode = BLENDMODE_ERASE;
	else if (val == "hardlight") th->blendMode = BLENDMODE_HARDLIGHT;
	else if (val == "invert") th->blendMode = BLENDMODE_INVERT;
	else if (val == "layer") th->blendMode = BLENDMODE_LAYER;
	else if (val == "lighten") th->blendMode = BLENDMODE_LIGHTEN;
	else if (val == "multiply") th->blendMode = BLENDMODE_MULTIPLY;
	else if (val == "overlay") th->blendMode = BLENDMODE_OVERLAY;
	else if (val == "screen") th->blendMode = BLENDMODE_SCREEN;
	else if (val == "subtract") th->blendMode = BLENDMODE_SUBTRACT;
}

ASFUNCTIONBODY_ATOM(DisplayObject,localToGlobal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t tempx, tempy;

	th->localToGlobal(pt->getX(), pt->getY(), tempx, tempy);

	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(sys,tempx, tempy));
}

ASFUNCTIONBODY_ATOM(DisplayObject,globalToLocal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t tempx, tempy;

	th->globalToLocal(pt->getX(), pt->getY(), tempx, tempy);

	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(sys,tempx, tempy));
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setRotation)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	//Apply the difference
	if(th->rotation!=val)
	{
		th->rotation=val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(sys);
	}
}

void DisplayObject::setParent(DisplayObjectContainer *p)
{
	if(parent!=p)
	{
		parent=p;
		hasChanged=true;
		if(onStage)
			requestInvalidation(getSystemState());
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getParent)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	if(!th->parent)
	{
		asAtomHandler::setUndefined(ret);
		return;
	}

	th->parent->incRef();
	ret = asAtomHandler::fromObject(th->parent);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getRoot)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<DisplayObject> res;
	
	if (th->isLoadedRootObject())
		res = _NR<DisplayObject>(th);
	else if (th->is<Stage>())
		// according to spec, the root of the stage is the stage itself
		res = _NR<DisplayObject>(th);
	else
		res =th->getRoot();
	if(res.isNull())
	{
		asAtomHandler::setUndefined(ret);	
		return;
	}

	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getRotation)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->rotation);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setVisible)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	th->visible=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getVisible)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setBool(ret,th->visible);
}

number_t DisplayObject::computeHeight()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix());

	return (ret)?(y2-y1):0;
}

number_t DisplayObject::computeWidth()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2,getMatrix());

	return (ret)?(x2-x1):0;
}

_NR<RootMovieClip> DisplayObject::getRoot()
{
	if(!parent)
		return NullRef;

	return parent->getRoot();
}

_NR<Stage> DisplayObject::getStage()
{
	if(!parent)
		return NullRef;

	return parent->getStage();
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getWidth)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->computeWidth());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setWidth)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t newwidth=asAtomHandler::toNumber(args[0]);

	number_t xmin,xmax,y1,y2;
	if(!th->boundsRect(xmin,xmax,y1,y2))
		return;

	number_t width=xmax-xmin;
	if(width==0) //Cannot scale, nothing to do (See Reference)
		return;
	
	if(width*th->sx!=newwidth) //If the width is changing, calculate new scale
	{
		if(th->useLegacyMatrix)
			th->useLegacyMatrix=false;
		th->setScaleX(newwidth/width);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getHeight)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->computeHeight());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_setHeight)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t newheight=asAtomHandler::toNumber(args[0]);

	number_t x1,x2,ymin,ymax;
	if(!th->boundsRect(x1,x2,ymin,ymax))
		return;

	number_t height=ymax-ymin;
	if(height==0) //Cannot scale, nothing to do (See Reference)
		return;

	if(height*th->sy!=newheight) //If the height is changing, calculate new scale
	{
		if(th->useLegacyMatrix)
			th->useLegacyMatrix=false;
		th->setScaleY(newheight/height);
	}
}

Vector2f DisplayObject::getLocalMousePos()
{
	return getConcatenatedMatrix().getInverted().multiply2D(getSystemState()->getInputThread()->getMousePos());
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMouseX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->getLocalMousePos().x);
}

ASFUNCTIONBODY_ATOM(DisplayObject,_getMouseY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->getLocalMousePos().y);
}

_NR<DisplayObject> DisplayObject::hitTest(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type,bool interactiveObjectsOnly)
{
	if(!(visible || type == GENERIC_HIT_INVISIBLE) || !isConstructed())
		return NullRef;

	//First check if there is any mask on this object, if so the point must be inside the mask to go on
	if(!mask.isNull())
	{
		//First compute the global coordinates from the local ones
		//TODO: we may also pass the global coordinates to all the calls
		const MATRIX& thisMatrix = this->getConcatenatedMatrix();
		number_t globalX, globalY;
		thisMatrix.multiply2D(x,y,globalX,globalY);
		//Now compute the coordinates local to the mask
		const MATRIX& maskMatrix = mask->getConcatenatedMatrix();
		if(!maskMatrix.isInvertible())
		{
			//If the matrix is not invertible the mask as collapsed to zero size
			//If the mask is zero sized then the object is not visible
			return NullRef;
		}
		number_t maskX, maskY;
		maskMatrix.getInverted().multiply2D(globalX,globalY,maskX,maskY);
		if(mask->hitTest(last, maskX, maskY, type,false)==false)
			return NullRef;
	}

	return hitTestImpl(last, x,y, type,interactiveObjectsOnly);
}

/* Display objects have no children in general,
 * so we skip to calling the constructor, if necessary.
 * This is called in vm's thread context */
void DisplayObject::initFrame()
{
	if(!isConstructed() && getClass())
	{
		asAtom o = asAtomHandler::fromObject(this);
		getClass()->handleConstruction(o,NULL,0,true);

		/*
		 * Legacy objects have their display list properties set on creation, but
		 * the related events must only be sent after the constructor is sent.
		 * This is from "Order of Operations".
		 */
		if(parent)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"added"));
			ABCVm::publicHandleEvent(this,e);
		}
		if(onStage)
		{
			_R<Event> e=_MR(Class<Event>::getInstanceS(getSystemState(),"addedToStage"));
			ABCVm::publicHandleEvent(this,e);
		}
	}
}

void DisplayObject::executeFrameScript()
{
	// send the init/complete events after the first frame is executed
	if (!this->loaderInfo.isNull())
		this->loaderInfo->setComplete();
}

void DisplayObject::constructionComplete()
{
	RELEASE_WRITE(constructed,true);
	if(!loaderInfo.isNull())
	{
		this->incRef();
		loaderInfo->objectHasLoaded(_MR(this));
	}
	hasChanged=true;
	if(onStage)
		requestInvalidation(getSystemState());
}

void DisplayObject::gatherMaskIDrawables(std::vector<IDrawable::MaskData>& masks) const
{
	if(mask.isNull())
		return;

	//If the mask is hard we need the drawable for each child
	//If the mask is soft we need the rendered final result
	IDrawable::MASK_MODE maskMode = IDrawable::HARD_MASK;
	//For soft masking to work, both the object and the mask must be
	//cacheAsBitmap and the mask must be on the stage
	if(this->computeCacheAsBitmap() && mask->computeCacheAsBitmap() && mask->isOnStage())
		maskMode = IDrawable::SOFT_MASK;

	if(maskMode == IDrawable::HARD_MASK)
	{
		SoftwareInvalidateQueue queue;
		mask->requestInvalidation(&queue);
		for(auto it=queue.queue.begin();it!=queue.queue.end();it++)
		{
			DisplayObject* target=(*it).getPtr();
			//Get the drawable from each of the added objects
			IDrawable* drawable=target->invalidate(NULL, MATRIX(),false);
			if(drawable==NULL)
				continue;
			masks.emplace_back(drawable, maskMode);
		}
	}
	else
	{
		IDrawable* drawable=NULL;
		if(mask->is<DisplayObjectContainer>())
		{
			//HACK: use bitmap temporarily
			number_t xmin,xmax,ymin,ymax;
			bool ret=mask->getBounds(xmin,xmax,ymin,ymax,mask->getConcatenatedMatrix());
			if(ret==false)
				return;
			_R<BitmapData> data(Class<BitmapData>::getInstanceS(getSystemState(),xmax-xmin,ymax-ymin));
			//Forge a matrix. It must contain the right rotation and scaling while translation
			//only compensate for the xmin/ymin offset
			MATRIX m=mask->getConcatenatedMatrix();
			m.x0 -= xmin;
			m.y0 -= ymin;
			data->drawDisplayObject(mask.getPtr(), m,false);
			_R<Bitmap> bmp(Class<Bitmap>::getInstanceS(getSystemState(),data));

			//The created bitmap is already correctly scaled and rotated
			//Just apply the needed offset
			MATRIX m2(1,1,0,0,xmin,ymin);
			drawable=bmp->invalidate(NULL, m2,false);
		}
		else
			drawable=mask->invalidate(NULL, MATRIX(),false);

		if(drawable==NULL)
			return;
		masks.emplace_back(drawable, maskMode);
	}
}

void DisplayObject::computeMasksAndMatrix(DisplayObject* target, std::vector<IDrawable::MaskData>& masks, MATRIX& totalMatrix) const
{
	const DisplayObject* cur=this;
	bool gatherMasks = true;
	while(cur && cur!=target)
	{
		totalMatrix=cur->getMatrix().multiplyMatrix(totalMatrix);
		//Get an IDrawable for all the hierarchy of each mask.
		if(gatherMasks)
		{
			if(cur->maskOf.isNull())
				cur->gatherMaskIDrawables(masks);
			else
			{
				//Stop gathering masks if any level of the hierarchy it's a mask
				masks.clear();
				masks.shrink_to_fit();
				gatherMasks=false;
			}
		}
		cur=cur->getParent();
	}
}

// Compute the minimal, axis aligned bounding box in global
// coordinates
bool DisplayObject::boundsRectGlobal(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	number_t x1, x2, y1, y2;
	if (!boundsRect(x1, x2, y1, y2))
		return abstract_b(getSystemState(),false);

	localToGlobal(x1, y1, x1, y1);
	localToGlobal(x2, y2, x2, y2);

	// Mapping to global may swap min and max values (for example,
	// rotation by 180 degrees)
	xmin = dmin(x1, x2);
	xmax = dmax(x1, x2);
	ymin = dmin(y1, y2);
	ymax = dmax(y1, y2);

	return true;
}

ASFUNCTIONBODY_ATOM(DisplayObject,hitTestObject)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	_NR<DisplayObject> another;
	ARG_UNPACK_ATOM(another);

	number_t xmin, xmax, ymin, ymax;
	if (!th->boundsRectGlobal(xmin, xmax, ymin, ymax))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	number_t xmin2, xmax2, ymin2, ymax2;
	if (!another->boundsRectGlobal(xmin2, xmax2, ymin2, ymax2))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	number_t intersect_xmax = dmin(xmax, xmax2);
	number_t intersect_xmin = dmax(xmin, xmin2);
	number_t intersect_ymax = dmin(ymax, ymax2);
	number_t intersect_ymin = dmax(ymin, ymin2);

	asAtomHandler::setBool(ret,(intersect_xmax > intersect_xmin) && 
			  (intersect_ymax > intersect_ymin));
}

ASFUNCTIONBODY_ATOM(DisplayObject,hitTestPoint)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t x;
	number_t y;
	bool shapeFlag;
	ARG_UNPACK_ATOM (x) (y) (shapeFlag, false);

	number_t xmin, xmax, ymin, ymax;
	if (!th->boundsRectGlobal(xmin, xmax, ymin, ymax))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	bool insideBoundingBox = (xmin <= x) && (x < xmax) && (ymin <= y) && (y < ymax);

	if (!shapeFlag)
	{
		asAtomHandler::setBool(ret,insideBoundingBox);
		return;
	}
	else
	{
		if (!insideBoundingBox)
		{
			asAtomHandler::setBool(ret,false);
			return;
		}

		number_t localX;
		number_t localY;
		th->globalToLocal(x, y, localX, localY);

		// Hmm, hitTest will also check the mask, is this the
		// right thing to do?
		th->incRef();
		_NR<DisplayObject> hit = th->hitTest(_MR(th), localX, localY,
						     HIT_TYPE::GENERIC_HIT_INVISIBLE,false);

		asAtomHandler::setBool(ret,!hit.isNull());
	}
}

multiname* DisplayObject::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	multiname* res = EventDispatcher::setVariableByMultiname(name,o,allowConst);
	if (getSystemState()->getSwfVersion() < 9)
	{
		if (name.name_s_id == BUILTIN_STRINGS::STRING_ONENTERFRAME ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
		{
			this->incRef();
			getSystemState()->registerFrameListener(_MR(this));
			getSystemState()->stage->AVM1AddEventListener(this);
			setIsEnumerable(name, false);
		}
		if (name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEMOVE ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEDOWN ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONMOUSEUP ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONPRESS ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONRELEASE)
		{
			getSystemState()->stage->AVM1AddMouseListener(this);
			setIsEnumerable(name, false);
		}
	}
	return res;
}
bool DisplayObject::deleteVariableByMultiname(const multiname& name)
{
	bool res = EventDispatcher::deleteVariableByMultiname(name);
	if (getSystemState()->getSwfVersion() < 9)
	{
		if (name.name_s_id == BUILTIN_STRINGS::STRING_ONENTERFRAME ||
				name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
		{
			this->incRef();
			getSystemState()->unregisterFrameListener(_MR(this));
		}
	}
	return res;
}
bool DisplayObject::AVM1HandleMouseEventStandard(DisplayObject *dispobj,MouseEvent *e)
{
	bool result = false;
	asAtom func=asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.isAttribute = false;
	asAtom ret=asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::fromObject(this);
	if (e->type == "mouseMove")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEMOVE;
		getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "click")
	{
		if (dispobj == this)
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASE;
			getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				result=true;
			}
		}
	}
	else if (e->type == "mouseDown")
	{
		if (dispobj == this)
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONPRESS;
			getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				result=true;
			}
		}
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEDOWN;
		getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "mouseUp")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEUP;
		getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "releaseOutside")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASEOUTSIDE;
		getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	return result;
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->sx*100.0);
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setScaleX)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleX(val/100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->sy*100.0);
}

ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setScaleY)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);
	number_t val=asAtomHandler::toNumber(args[0]);
	//Stop using the legacy matrix
	if(th->useLegacyMatrix)
		th->useLegacyMatrix=false;
	th->setScaleY(val/100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getParent)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	DisplayObject* p = th->parent;
	if(!p)
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	if (p->is<Stage>())
		p= sys->mainClip;
	p->incRef();
	ret = asAtomHandler::fromObject(p);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getRoot)
{
	sys->mainClip->incRef();
	ret = asAtomHandler::fromObject(sys->mainClip);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_hitTest)
{
	asAtomHandler::setBool(ret,false);
	if (argslen==1)
	{
		if (!asAtomHandler::is<Undefined>(args[0]))
		{
			if (asAtomHandler::is<MovieClip>(obj))
			{
				tiny_string s = asAtomHandler::toString(args[0],sys);
				MovieClip* path = asAtomHandler::as<MovieClip>(obj)->AVM1GetClipFromPath(s);
				if (path)
				{
					asAtom pathobj = asAtomHandler::fromObject(path);
					hitTestObject(ret,sys,obj,&pathobj,1);
				}
				else
					LOG(LOG_ERROR,"AVM1_hitTest:clip not found:"<<asAtomHandler::toDebugString(args[0]));
			}
			else
				LOG(LOG_NOT_IMPLEMENTED,"AVM1_hitTest:object is no MovieClip:"<<asAtomHandler::toDebugString(obj));
		}
	}
	else
		hitTestPoint(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_localToGlobal)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen == 1);

	
	ASObject* pt=asAtomHandler::toObject(args[0],sys);
	
	asAtom x = asAtomHandler::fromInt(0);
	asAtom y = asAtomHandler::fromInt(0);
	multiname mx(nullptr);
	mx.name_type=multiname::NAME_STRING;
	mx.name_s_id=sys->getUniqueStringId("x");
	mx.isAttribute = false;
	pt->getVariableByMultiname(x,mx);
	multiname my(nullptr);
	my.name_type=multiname::NAME_STRING;
	my.name_s_id=sys->getUniqueStringId("y");
	my.isAttribute = false;
	pt->getVariableByMultiname(y,my);

	number_t tempx, tempy;

	th->localToGlobal(asAtomHandler::toNumber(x), asAtomHandler::toNumber(y), tempx, tempy);
	asAtomHandler::setNumber(x,sys,tempx);
	asAtomHandler::setNumber(y,sys,tempy);
	pt->setVariableByMultiname(mx,x,CONST_ALLOWED);
	pt->setVariableByMultiname(my,y,CONST_ALLOWED);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBytesLoaded)
{
	if (sys->mainClip->loaderInfo)
	{
		asAtomHandler::setUInt(ret,sys,sys->mainClip->loaderInfo->getBytesLoaded());
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBytesTotal)
{
	if (sys->mainClip->loaderInfo)
	{
		asAtomHandler::setUInt(ret,sys,sys->mainClip->loaderInfo->getBytesTotal());
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getQuality)
{
	ret = asAtomHandler::fromString(sys,sys->stage->quality.uppercase());
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setQuality)
{
	if (argslen > 0)
		sys->stage->quality = asAtomHandler::toString(args[0],sys).uppercase();
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	asAtomHandler::setNumber(ret,sys,th->alpha*100.0);
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_setAlpha)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	number_t val;
	ARG_UNPACK_ATOM (val);
	val /= 100.0;
	if(th->alpha != val)
	{
		th->alpha=val;
		th->hasChanged=true;
		if(th->onStage)
			th->requestInvalidation(sys);
	}
}
ASFUNCTIONBODY_ATOM(DisplayObject,AVM1_getBounds)
{
	DisplayObject* th=asAtomHandler::as<DisplayObject>(obj);
	assert_and_throw(argslen==1);

	ASObject* o =  Class<ASObject>::getInstanceS(sys);
	ret = asAtomHandler::fromObject(o);
	if(asAtomHandler::is<Undefined>(args[0]) || asAtomHandler::is<Null>(args[0]))
		return;
	if (!asAtomHandler::is<DisplayObject>(args[0]))
		LOG(LOG_ERROR,"DisplayObject.getBounds invalid type:"<<asAtomHandler::toDebugString(args[0]));
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	DisplayObject* target=asAtomHandler::as<DisplayObject>(args[0]);
	//Compute the transformation matrix
	MATRIX m;
	DisplayObject* cur=th;
	while(cur!=NULL && cur!=target)
	{
		m = cur->getMatrix().multiplyMatrix(m);
		cur=cur->parent;
	}
	if(cur==NULL)
	{
		//We crawled all the parent chain without finding the target
		//The target is unrelated, compute it's transformation matrix
		const MATRIX& targetMatrix=target->getConcatenatedMatrix();
		//If it's not invertible just use the previous computed one
		if(targetMatrix.isInvertible())
			m = targetMatrix.getInverted().multiplyMatrix(m);
	}

	number_t x1=0,x2=0,y1=0,y2=0;
	th->getBounds(x1,x2,y1,y2, m);

	asAtom v=asAtomHandler::invalidAtom;
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=sys->getUniqueStringId("xMin");
	v = asAtomHandler::fromNumber(sys,x1,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
	name.name_s_id=sys->getUniqueStringId("xMax");
	v = asAtomHandler::fromNumber(sys,x2,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
	name.name_s_id=sys->getUniqueStringId("yMin");
	v = asAtomHandler::fromNumber(sys,y1,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
	name.name_s_id=sys->getUniqueStringId("yMax");
	v = asAtomHandler::fromNumber(sys,y2,false);
	o->setVariableByMultiname(name,v,ASObject::CONST_ALLOWED);
}
void DisplayObject::AVM1SetupMethods(Class_base* c)
{
	// setup all methods and properties available for MovieClips in AVM1
	
	c->destroyContents();
	c->setDeclaredMethodByQName("_x","",Class<IFunction>::getFunction(c->getSystemState(),_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_x","",Class<IFunction>::getFunction(c->getSystemState(),_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_y","",Class<IFunction>::getFunction(c->getSystemState(),_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_y","",Class<IFunction>::getFunction(c->getSystemState(),_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_visible","",Class<IFunction>::getFunction(c->getSystemState(),_getVisible),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_visible","",Class<IFunction>::getFunction(c->getSystemState(),_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_xscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getScaleX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_xscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_yscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getScaleY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_yscale","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",Class<IFunction>::getFunction(c->getSystemState(),_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",Class<IFunction>::getFunction(c->getSystemState(),_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_parent","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getParent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_root","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getRoot),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTest","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_hitTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_localToGlobal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getBytesLoaded),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getBytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getBytesTotal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("_xmouse","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_ymouse","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_quality","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getQuality),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_quality","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setQuality),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_alpha","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getAlpha),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_alpha","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",Class<IFunction>::getFunction(c->getSystemState(),AVM1_getBounds),NORMAL_METHOD,true);
}

