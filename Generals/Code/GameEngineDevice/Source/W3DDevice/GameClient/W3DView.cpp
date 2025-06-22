/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: W3DView.cpp //////////////////////////////////////////////////////////////////////////////
//
// W3D implementation of the game view class.  This view allows us to have
// a "window" into the game world that can change its width, height as 
// well as camera positioning controls
//
// Author: Colin Day, April 2001
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES ////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <windows.h>

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "Common/BuildAssistant.h"
#include "Common/GlobalData.h"
#include "Common/Module.h"
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingSort.h"
#include "Common/PerfTimer.h"
#include "Common/PlayerList.h"
#include "Common/Player.h"

#include "GameClient/Color.h"
#include "GameClient/CommandXlat.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/Image.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Line2D.h"
#include "GameClient/SelectionInfo.h"
#include "GameClient/Shell.h"
#include "GameClient/TerrainVisual.h"
#include "GameClient/Water.h"

#include "GameLogic/AI.h"			///< For AI debug (yes, I'm cheating for now)
#include "GameLogic/AIPathfind.h"			///< For AI debug (yes, I'm cheating for now)
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/OpenContain.h"
#include "GameLogic/Object.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/TerrainLogic.h"									///< @todo This should be TerrainVisual (client side)
#include "Common/AudioEventInfo.h"

#include "W3DDevice/Common/W3DConvert.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "W3DDevice/GameClient/W3DDisplay.h"
#include "W3DDevice/GameClient/W3DScene.h"
#include "W3DDevice/GameClient/W3DView.h"
#include "d3dx8math.h"
#include "W3DDevice/GameClient/W3DShaderManager.h"
#include "W3DDevice/GameClient/Module/W3DModelDraw.h"
#include "W3DDevice/GameClient/W3DCustomScene.h"

#include "WW3D2/dx8renderer.h"
#include "WW3D2/light.h"
#include "WW3D2/camera.h"
#include "WW3D2/coltype.h"
#include "WW3D2/predlod.h"
#include "WW3D2/ww3d.h"

#include "WinMain.h"  /** @todo Remove this, it's only here because we
													are using timeGetTime, but we can remove that
													when we have our own timer */
#ifdef RTS_INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif



// 30 fps
Int TheW3DFrameLengthInMsec = 1000/LOGICFRAMES_PER_SECOND; // default is 33msec/frame == 30fps. but we may change it depending on sys config.
static const Int MAX_REQUEST_CACHE_SIZE = 40;	// Any size larger than 10, or examine code below for changes. jkmcd.
static const Real DRAWABLE_OVERSCAN = 75.0f;  ///< 3D world coords of how much to overscan in the 3D screen region






//=================================================================================================
inline Real minf(Real a, Real b) { if (a < b) return a; else return b; }
inline Real maxf(Real a, Real b) { if (a > b) return a; else return b; }

//-------------------------------------------------------------------------------------------------
// Normalizes angle to +- PI.
//-------------------------------------------------------------------------------------------------
static void normAngle(Real &angle)
{
	if (angle < -10*PI) {
		angle = 0;
	}
	if (angle > 10*PI) {
		angle = 0;
	}
	while (angle < -PI) {
		angle += 2*PI;
	}
	while (angle > PI) {
		angle -= 2*PI;
	}
}

#define TERRAIN_SAMPLE_SIZE 40.0f
static Real getHeightAroundPos(Real x, Real y)
{
	// terrain height + desired height offset == cameraOffset * actual zoom
	Real terrainHeight = TheTerrainLogic->getGroundHeight(x, y);

	// find best approximation of max terrain height we can see
	Real terrainHeightMax = terrainHeight;
	terrainHeightMax = max(terrainHeightMax, TheTerrainLogic->getGroundHeight(x+TERRAIN_SAMPLE_SIZE, y-TERRAIN_SAMPLE_SIZE));
	terrainHeightMax = max(terrainHeightMax, TheTerrainLogic->getGroundHeight(x-TERRAIN_SAMPLE_SIZE, y-TERRAIN_SAMPLE_SIZE));
	terrainHeightMax = max(terrainHeightMax, TheTerrainLogic->getGroundHeight(x+TERRAIN_SAMPLE_SIZE, y+TERRAIN_SAMPLE_SIZE));
	terrainHeightMax = max(terrainHeightMax, TheTerrainLogic->getGroundHeight(x-TERRAIN_SAMPLE_SIZE, y+TERRAIN_SAMPLE_SIZE));

	return terrainHeightMax;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DView::W3DView()
{
	
	m_3DCamera = NULL;
	m_2DCamera = NULL;
	m_groundLevel = 10.0;
	m_cameraOffset.z = TheGlobalData->m_cameraHeight;
	m_cameraOffset.y = -(m_cameraOffset.z / tan(TheGlobalData->m_cameraPitch * (PI / 180.0)));
	m_cameraOffset.x = -(m_cameraOffset.y * tan(TheGlobalData->m_cameraYaw * (PI / 180.0)));

	m_viewFilterMode = FM_NULL_MODE;
	m_viewFilter = FT_NULL_FILTER;
	m_isWireFrameEnabled = m_nextWireFrameEnabled = FALSE;
	m_shakeOffset.x = 0.0f;
	m_shakeOffset.y = 0.0f;
	m_shakeIntensity = 0.0f;
	m_FXPitch = 1.0f;
	m_freezeTimeForCameraMovement = false;
	m_cameraHasMovedSinceRequest = true;
	m_locationRequests.clear();
	m_locationRequests.reserve(MAX_REQUEST_CACHE_SIZE + 10);	// This prevents the vector from ever re-allocing

}  // end W3DView

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DView::~W3DView()
{

	REF_PTR_RELEASE( m_2DCamera );
	REF_PTR_RELEASE( m_3DCamera );

}  // end ~W3DView

//-------------------------------------------------------------------------------------------------
/** Sets the height of the viewport, while maintaining original camera perspective. */
//-------------------------------------------------------------------------------------------------
void W3DView::setHeight(Int height)
{
	// extend View functionality
	View::setHeight(height);

	Vector2 vMin,vMax;
	m_3DCamera->Set_Aspect_Ratio((Real)getWidth()/(Real)height);
 	m_3DCamera->Get_Viewport(vMin,vMax);
 	vMax.Y=(Real)(m_originY+height)/(Real)TheDisplay->getHeight();
 	m_3DCamera->Set_Viewport(vMin,vMax);
}

//-------------------------------------------------------------------------------------------------
/** Sets the width of the viewport, while maintaining original camera perspective. */
//-------------------------------------------------------------------------------------------------
void W3DView::setWidth(Int width)
{
	// extend View functionality
	View::setWidth(width);

	Vector2 vMin,vMax;
	m_3DCamera->Set_Aspect_Ratio((Real)width/(Real)getHeight());
 	m_3DCamera->Get_Viewport(vMin,vMax);
 	vMax.X=(Real)(m_originX+width)/(Real)TheDisplay->getWidth();
 	m_3DCamera->Set_Viewport(vMin,vMax);

	//we want to maintain the same scale, so we'll need to adjust the fov.
	//default W3D fov for full-screen is 50 degrees.
	m_3DCamera->Set_View_Plane((Real)width/(Real)TheDisplay->getWidth()*DEG_TO_RADF(50.0f),-1);
}

//-------------------------------------------------------------------------------------------------
/** Sets location of top-left view corner on display */
//-------------------------------------------------------------------------------------------------
void W3DView::setOrigin( Int x, Int y)
{
	// extend View functionality
	View::setOrigin(x,y);

	Vector2 vMin,vMax;

 	m_3DCamera->Get_Viewport(vMin,vMax);
 	vMin.X=(Real)x/(Real)TheDisplay->getWidth();
	vMin.Y=(Real)y/(Real)TheDisplay->getHeight();
 	m_3DCamera->Set_Viewport(vMin,vMax);

	// bottom-right border was also moved my this call, so force an update of extents.
	setWidth(m_width);
	setHeight(m_height);
}

//-------------------------------------------------------------------------------------------------
/** @todo This is inefficient. We should construct the matrix directly using vectors. */
//-------------------------------------------------------------------------------------------------
void W3DView::buildCameraTransform( Matrix3D *transform )
{
	Vector3 sourcePos, targetPos;

	Real groundLevel = m_groundLevel; // 93.0f; 

	Real zoom = getZoom();
	Real angle = getAngle();
	Real pitch = getPitch();
	Coord3D pos = *getPosition();

	// add in the camera shake, if any
	pos.x += m_shakeOffset.x;
	pos.y += m_shakeOffset.y;

	if (m_cameraConstraintValid)
	{
		pos.x = maxf(m_cameraConstraint.lo.x, pos.x);
		pos.x = minf(m_cameraConstraint.hi.x, pos.x);
		pos.y = maxf(m_cameraConstraint.lo.y, pos.y);
		pos.y = minf(m_cameraConstraint.hi.y, pos.y);
	}

	// set position of camera itself
	sourcePos.X = m_cameraOffset.x*zoom;
	sourcePos.Y = m_cameraOffset.y*zoom;
	sourcePos.Z = m_cameraOffset.z*zoom;
	

#ifdef NOT_IN_USE
	if (TheGlobalData->m_isOffsetCameraZ && TheTerrainLogic)
	{
		sourcePos.Z += TheTerrainLogic->getGroundHeight(pos.x, pos.y);
		if (m_prevSourcePosZ != SOURCEPOS_INVALID)
		{
			const Real MAX_SPZ_VARIATION = 0.05f;
			Real spzMin = m_prevSourcePosZ*(1.0-MAX_SPZ_VARIATION);
			Real spzMax			Coord3D center;
 = m_prevSourcePosZ*(1.0+MAX_SPZ_VARIATION);
			if (sourcePos.Z < spzMin) sourcePos.Z = spzMin;
			if (sourcePos.Z > spzMax) sourcePos.Z = spzMax;
		}
		m_prevSourcePosZ = sourcePos.Z;
	}
#endif

	// camera looking at origin
	targetPos.X = 0;
	targetPos.Y = 0;
	targetPos.Z = 0;


	Real factor = 1.0 - (groundLevel/sourcePos.Z );

	// construct a matrix to rotate around the up vector by the given angle
	Matrix3D angleTransform( Vector3( 0.0f, 0.0f, 1.0f ), angle );

	// construct a matrix to rotate around the horizontal vector by the given angle
	Matrix3D pitchTransform( Vector3( 1.0f, 0.0f, 0.0f ), pitch );

	// rotate camera position (pitch, then angle)
#ifdef ALLOW_TEMPORARIES
	sourcePos = pitchTransform * sourcePos;
	sourcePos = angleTransform * sourcePos;
#else
	pitchTransform.mulVector3(sourcePos);
	angleTransform.mulVector3(sourcePos);
#endif
	sourcePos *= factor;

	// translate to current XY position
	sourcePos.X += pos.x;
	sourcePos.Y += pos.y;
	sourcePos.Z += groundLevel;
	
	targetPos.X += pos.x;
	targetPos.Y += pos.y;
	targetPos.Z += groundLevel;

	// do m_FXPitch adjustment.
	Real height = sourcePos.Z - targetPos.Z;
	height *= m_FXPitch;
	targetPos.Z = sourcePos.Z - height;

	// build new camera transform
	transform->Make_Identity();
	transform->Look_At( sourcePos, targetPos, 0 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::calcCameraConstraints()
{
//	const Matrix3D& cameraTransform = m_3DCamera->Get_Transform();

//	DEBUG_LOG(("*** rebuilding cam constraints\n"));

	// ok, now check to ensure that we can't see outside the map region,
	// and twiddle the camera if needed
	if (TheTerrainLogic)
	{
		Region3D mapRegion;
		TheTerrainLogic->getExtent( &mapRegion );
		
	/*
		Note the following restrictions on camera constraints!

		-- they assume that all maps are height 'm_groundLevel' at the edges.
				(since you need to add some "buffer" around the edges of your map
				anyway, this shouldn't be an issue.)

		-- for angles/pitches other than zero, it may show boundaries.
				since we currently plan the game to be restricted to this,
				it shouldn't be an issue.

	*/
		Real maxEdgeZ = m_groundLevel;
//		const Real BORDER_FUDGE = MAP_XY_FACTOR * 1.414f;
		Coord3D center, bottom;
		ICoord2D screen;

		//Pick at the center
		screen.x=0.5f*getWidth()+m_originX;
		screen.y=0.5f*getHeight()+m_originY;

		Vector3 rayStart,rayEnd;

		getPickRay(&screen,&rayStart,&rayEnd);

		center.x = Vector3::Find_X_At_Z(maxEdgeZ, rayStart, rayEnd);
		center.y = Vector3::Find_Y_At_Z(maxEdgeZ, rayStart, rayEnd);
		center.z = maxEdgeZ;

		screen.y = m_originY+ 0.95f*getHeight();
 		getPickRay(&screen,&rayStart,&rayEnd);
 		bottom.x = Vector3::Find_X_At_Z(maxEdgeZ, rayStart, rayEnd);
		bottom.y = Vector3::Find_Y_At_Z(maxEdgeZ, rayStart, rayEnd);
		bottom.z = maxEdgeZ;
		center.x -= bottom.x;
		center.y -= bottom.y;

		Real offset = center.length();

		if (TheGlobalData->m_debugAI) {
			offset = -1000; // push out the constraints so we can look at staging areas.
		}

		m_cameraConstraint.lo.x = mapRegion.lo.x + offset;
		m_cameraConstraint.hi.x = mapRegion.hi.x - offset;
		// this looks inverted, but is correct
		m_cameraConstraint.lo.y = mapRegion.lo.y + offset;
		m_cameraConstraint.hi.y = mapRegion.hi.y - offset;
		m_cameraConstraintValid = true;
	}
}

//-------------------------------------------------------------------------------------------------
/** Returns a world-space ray originating at a given screen pixel position
	and ending at the far clip plane for current camera.  Screen coordinates
	assumed in absolute values relative to full display resolution.*/
//-------------------------------------------------------------------------------------------------
void W3DView::getPickRay(const ICoord2D *screen, Vector3 *rayStart, Vector3 *rayEnd)
{
	Real logX,logY;

	//W3D Screen coordinates are -1 to 1, so we need to do some conversion:
	PixelScreenToW3DLogicalScreen(screen->x - m_originX,screen->y - m_originY, &logX, &logY,getWidth(),getHeight());

	*rayStart = m_3DCamera->Get_Position();	//get camera location
	m_3DCamera->Un_Project(*rayEnd,Vector2(logX,logY));	//get world space point
	*rayEnd -= *rayStart;	//vector camera to world space point
	rayEnd->Normalize();	//make unit vector
	*rayEnd *= m_3DCamera->Get_Depth();	//adjust length to reach far clip plane
	*rayEnd += *rayStart;	//get point on far clip plane along ray from camera.
}

//-------------------------------------------------------------------------------------------------
/** set the transform matrix of m_3DCamera, based on m_pos & m_angle */
//-------------------------------------------------------------------------------------------------
void W3DView::setCameraTransform( void )
{
	if (TheGlobalData->m_headless)
		return;
	m_cameraHasMovedSinceRequest = true;
	Matrix3D cameraTransform( 1 );
	
	Real nearZ, farZ;
	m_3DCamera->Get_Clip_Planes(nearZ, farZ);
	// Set the near to MAP_XY_FACTOR.  Improves zbuffer resolution.
	nearZ = MAP_XY_FACTOR; 
	farZ = 1200.0f;

	if ((TheGlobalData && TheGlobalData->m_drawEntireTerrain) || (m_FXPitch<0.95f || m_zoom>1.05))
	{	//need to extend far clip plane so entire terrain can be visible
		farZ *= MAP_XY_FACTOR;
	}

	m_3DCamera->Set_Clip_Planes(nearZ, farZ);
#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
	if (TheGlobalData->m_useCameraConstraints)
#endif
	{
		if (!m_cameraConstraintValid)
		{
			buildCameraTransform(&cameraTransform);
			m_3DCamera->Set_Transform( cameraTransform );
			calcCameraConstraints();
		}
		DEBUG_ASSERTLOG(m_cameraConstraintValid,("*** cam constraints are not valid!!!\n"));

		if (m_cameraConstraintValid)
		{
			Coord3D pos = *getPosition();
			pos.x = maxf(m_cameraConstraint.lo.x, pos.x);
			pos.x = minf(m_cameraConstraint.hi.x, pos.x);
			pos.y = maxf(m_cameraConstraint.lo.y, pos.y);
			pos.y = minf(m_cameraConstraint.hi.y, pos.y);
			setPosition(&pos);
		}
	}

#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
	m_3DCamera->Set_View_Plane( m_FOV, -1 );
#endif

	// rebuild it (even if we just did it due to camera constraints)
	buildCameraTransform( &cameraTransform );
	m_3DCamera->Set_Transform( cameraTransform );

	if (TheTerrainRenderObject)
	{
		RefRenderObjListIterator *it = W3DDisplay::m_3DScene->createLightsIterator();
		TheTerrainRenderObject->updateCenter(m_3DCamera, it);
		if (it) 
		{
		 W3DDisplay::m_3DScene->destroyLightsIterator(it);
		 it = NULL;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::init( void )
{
	// extend View functionality
	View::init();
	setName("W3DView");
	// set default camera "lookat" point
	Coord3D pos;
	pos.x = 87.0f;
	pos.y = 77.0f;
	pos.z = 0;

	pos.x *= MAP_XY_FACTOR;
	pos.y *= MAP_XY_FACTOR;

	setPosition(&pos);

	// create our 3D camera
	m_3DCamera = NEW_REF( CameraClass, () );


	setCameraTransform();

	// create our 2D camera for the GUI overlay
	m_2DCamera = NEW_REF( CameraClass, () );
	m_2DCamera->Set_Position( Vector3( 0, 0, 1 ) );
	Vector2 min = Vector2( -1, -0.75f );
	Vector2 max = Vector2( +1, +0.75f );
	m_2DCamera->Set_View_Plane( min, max );		
	m_2DCamera->Set_Clip_Planes( 0.995f, 2.0f );

	m_cameraConstraintValid = false;

	m_scrollAmountCutoff = TheGlobalData->m_scrollAmountCutoff;

}  // end init

//-------------------------------------------------------------------------------------------------
const Coord3D& W3DView::get3DCameraPosition() const
{
	Vector3 camera = m_3DCamera->Get_Position();
	static Coord3D pos;
	pos.set( camera.X, camera.Y, camera.Z );
	return pos;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::reset( void )
{
	View::reset();

	// Just in case...
	setTimeMultiplier(1); // Set time rate back to 1.

	Coord3D arbitraryPos = { 0, 0, 0 };
	// Just move the camera to 0, 0, 0. It'll get repositioned at the beginning of the next game
	// anyways.
	resetCamera(&arbitraryPos, 1);

	setViewFilter((FilterTypes)0);

	Coord2D gb = { 0,0 };
	setGuardBandBias( &gb );
}

//-------------------------------------------------------------------------------------------------
/** draw worker for drawables in the view region */
//-------------------------------------------------------------------------------------------------
static void drawDrawable( Drawable *draw, void *userData )
{

	draw->draw( (View *)userData );

}  // end drawDrawable

// ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void drawTerrainNormal( Drawable *draw, void *userData )
{
	UnsignedInt color = GameMakeColor( 255, 255, 0, 255 );
  if (TheTerrainLogic)
  {
    Coord3D pos = *draw->getPosition();
    Coord3D normal;
    pos.z = TheTerrainLogic->getGroundHeight(pos.x, pos.y, &normal);
    const Real NORMLEN = 20;
    normal.x = pos.x + normal.x * NORMLEN;
    normal.y = pos.y + normal.y * NORMLEN;
    normal.z = pos.z + normal.z * NORMLEN;
    ICoord2D start, end;
		TheTacticalView->worldToScreen(&pos, &start);
		TheTacticalView->worldToScreen(&normal, &end);
		TheDisplay->drawLine(start.x, start.y, end.x, end.y, 1.0f, color);
  }
}

#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
// ------------------------------------------------------------------------------------------------
// Draw a crude circle. Appears on top of any world geometry
// ------------------------------------------------------------------------------------------------
void drawDebugCircle( const Coord3D & center, Real radius, Real width, Color color )
{
  const Real inc = PI/4.0f;
  Real angle = 0.0f;
  Coord3D pnt, lastPnt;
  ICoord2D start, end;
  Bool endValid, startValid;

  lastPnt.x = center.x + radius * (Real)cos(angle);
  lastPnt.y = center.y + radius * (Real)sin(angle);
  lastPnt.z = center.z;
  endValid = ( TheTacticalView->worldToScreenTriReturn( &lastPnt, &end ) != View::WTS_INVALID );
  
  for( angle = inc; angle <= 2.0f * PI; angle += inc )
  {
    pnt.x = center.x + radius * (Real)cos(angle);
    pnt.y = center.y + radius * (Real)sin(angle);
    pnt.z = center.z;
    startValid = ( TheTacticalView->worldToScreenTriReturn( &pnt, &start ) != View::WTS_INVALID );
    
    if ( startValid && endValid ) 
      TheDisplay->drawLine( start.x, start.y, end.x, end.y, width, color );
    
    lastPnt = pnt;
    end = start;
    endValid = startValid;
  }
}

void drawDrawableExtents( Drawable *draw, void *userData );  // FORWARD DECLARATION
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static void drawContainedDrawable( Object *obj, void *userData )
{
	Drawable *draw = obj->getDrawable();

	if( draw )
		drawDrawableExtents( draw, userData );

}  // end drawContainedDrawable

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void drawDrawableExtents( Drawable *draw, void *userData )
{
	UnsignedInt color = GameMakeColor( 0, 255, 0, 255 );

	switch( draw->getDrawableGeometryInfo().getGeomType() )
	{

		//---------------------------------------------------------------------------------------------
		case GEOMETRY_BOX:
		{
			Real angle = draw->getOrientation();
			Real c = (Real)cos(angle);
			Real s = (Real)sin(angle);
			Real exc = draw->getDrawableGeometryInfo().getMajorRadius()*c;
			Real eyc = draw->getDrawableGeometryInfo().getMinorRadius()*c;
			Real exs = draw->getDrawableGeometryInfo().getMajorRadius()*s;
			Real eys = draw->getDrawableGeometryInfo().getMinorRadius()*s;
			Coord3D pts[4];
			pts[0].x = draw->getPosition()->x - exc - eys;
			pts[0].y = draw->getPosition()->y + eyc - exs;
			pts[0].z = 0;
			pts[1].x = draw->getPosition()->x + exc - eys;
			pts[1].y = draw->getPosition()->y + eyc + exs;
			pts[1].z = 0;
			pts[2].x = draw->getPosition()->x + exc + eys;
			pts[2].y = draw->getPosition()->y - eyc + exs;
			pts[2].z = 0;
			pts[3].x = draw->getPosition()->x - exc + eys;
			pts[3].y = draw->getPosition()->y - eyc - exs;
			pts[3].z = 0;
			Real z = draw->getPosition()->z;
			for( int i = 0; i < 2; i++ )
			{

				for (int corner = 0; corner < 4; corner++)
				{
					ICoord2D start, end;
					pts[corner].z = z;
					pts[(corner+1)&3].z = z;
					TheTacticalView->worldToScreen(&pts[corner], &start);
					TheTacticalView->worldToScreen(&pts[(corner+1)&3], &end);
					TheDisplay->drawLine(start.x, start.y, end.x, end.y, 1.0f, color);
				}

				z += draw->getDrawableGeometryInfo().getMaxHeightAbovePosition();

			}  // end for i

			break;

		}  // end case box

		//---------------------------------------------------------------------------------------------
		case GEOMETRY_SPHERE:	// not quite right, but close enough
		case GEOMETRY_CYLINDER:
		{ 
      Coord3D center = *draw->getPosition();
      const Real radius = draw->getDrawableGeometryInfo().getMajorRadius();

			// draw cylinder
			for( int i=0; i<2; i++ )
			{
        drawDebugCircle( center, radius, 1.0f, color );

        // next time 'round, draw the top of the cylinder
        center.z += draw->getDrawableGeometryInfo().getMaxHeightAbovePosition();
			}	// end for i

			// draw centerline
      ICoord2D start, end;
      center = *draw->getPosition();
      TheTacticalView->worldToScreen( &center, &start );
      center.z += draw->getDrawableGeometryInfo().getMaxHeightAbovePosition();
      TheTacticalView->worldToScreen( &center, &end );
			TheDisplay->drawLine( start.x, start.y, end.x, end.y, 1.0f, color );

			break;

		}	// case CYLINDER

	} // end switch

	// draw any extents for things that are contained by this
	Object *obj = draw->getObject();
	if( obj )
	{
		ContainModuleInterface *contain = obj->getContain();

		if( contain )
			contain->iterateContained( drawContainedDrawable, userData, FALSE );

	}  // end if

}  // end drawDrawableExtents


void drawAudioLocations( Drawable *draw, void *userData );
// ------------------------------------------------------------------------------------------------
// Helper for drawAudioLocations
// ------------------------------------------------------------------------------------------------
static void drawContainedAudioLocations( Object *obj, void *userData )
{
  Drawable *draw = obj->getDrawable();
  
  if( draw )
    drawAudioLocations( draw, userData );
  
}  // end drawContainedAudio


//-------------------------------------------------------------------------------------------------
// Draw the location of audio objects in the world
//-------------------------------------------------------------------------------------------------
static void drawAudioLocations( Drawable *draw, void *userData )
{
  // draw audio for things that are contained by this
  Object *obj = draw->getObject();
  if( obj )
  {
    ContainModuleInterface *contain = obj->getContain();
    
    if( contain )
      contain->iterateContained( drawContainedAudioLocations, userData, FALSE );
    
  }  // end if

  const ThingTemplate * thingTemplate = draw->getTemplate();

  if ( thingTemplate == NULL || thingTemplate->getEditorSorting() != ES_AUDIO )
  {
    return; // All done
  }

  // Copied in hideously inappropriate code copying ways from DrawObject.cpp
  // Should definately be a global, probably read in from an INI file <gasp>
  static const Int poleHeight = 20;
  static const Int flagHeight = 10;
  static const Int flagWidth = 10;
  const Color color = GameMakeColor(0x25, 0x25, 0xEF, 0xFF);

  // Draw flag for audio-only objects:
  //  *
  //  * *
  //  *   *
  //  *     *
  //  *   *
  //  * *
  //  *
  //  *
  //  *
  //  *
  //  *

  Coord3D worldPoint;
  ICoord2D start, end;

  worldPoint = *draw->getPosition();
  TheTacticalView->worldToScreen( &worldPoint, &start );
  worldPoint.z += poleHeight;
  TheTacticalView->worldToScreen( &worldPoint, &end );
  TheDisplay->drawLine( start.x, start.y, end.x, end.y, 1.0f, color );
  
  worldPoint.z -= flagHeight / 2;
  worldPoint.x += flagWidth;
  TheTacticalView->worldToScreen( &worldPoint, &start );
  TheDisplay->drawLine( start.x, start.y, end.x, end.y, 1.0f, color );

  worldPoint.z -= flagHeight / 2;
  worldPoint.x -= flagWidth;
  TheTacticalView->worldToScreen( &worldPoint, &end );
  TheDisplay->drawLine( start.x, start.y, end.x, end.y, 1.0f, color );
}

//-------------------------------------------------------------------------------------------------
// Draw the radii of sounds attached to any type of object. 
//-------------------------------------------------------------------------------------------------
static void drawAudioRadii( const Drawable * drawable )
{
  
  // Draw radii, if sound is playing
  const AudioEventRTS * ambientSound = drawable->getAmbientSound();
  
  if ( ambientSound && ambientSound->isCurrentlyPlaying() )
  {
    const AudioEventInfo * ambientInfo = ambientSound->getAudioEventInfo();
    
    if ( ambientInfo == NULL )
    {
      // I don't think that's right...
      OutputDebugString( ("Playing sound has NULL AudioEventInfo?\n" ) );
      
      if ( TheAudio != NULL )
      {
        ambientInfo = TheAudio->findAudioEventInfo( ambientSound->getEventName() );
      }
    }
    
    if ( ambientInfo != NULL )
    {
      // Colors match those used in WorldBuilder
      drawDebugCircle( *drawable->getPosition(), ambientInfo->m_minDistance, 1.0f, GameMakeColor(0x00, 0x00, 0xFF, 0xFF) );
      drawDebugCircle( *drawable->getPosition(), ambientInfo->m_maxDistance, 1.0f, GameMakeColor(0xFF, 0x00, 0xFF, 0xFF) );
    }
  }
}

#endif

//-------------------------------------------------------------------------------------------------
/** An opportunity to draw something after all drawables have been drawn once */
//-------------------------------------------------------------------------------------------------
static void drawablePostDraw( Drawable *draw, void *userData )
{
	Real FXPitch = TheTacticalView->getFXPitch();
	if (draw->isDrawableEffectivelyHidden() || FXPitch < 0.0f)
		return;

	Object* obj = draw->getObject();
	Int localPlayerIndex = ThePlayerList ? ThePlayerList->getLocalPlayer()->getPlayerIndex() : 0;
#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
	ObjectShroudStatus ss = (!obj || !TheGlobalData->m_shroudOn) ? OBJECTSHROUD_CLEAR : obj->getShroudedStatus(localPlayerIndex);
#else
	ObjectShroudStatus ss = (!obj) ? OBJECTSHROUD_CLEAR : obj->getShroudedStatus(localPlayerIndex);
#endif
	if (ss > OBJECTSHROUD_PARTIAL_CLEAR)
		return;

	// draw the any "icon" UI for a drawable (health bars, veterency, etc);
	
	//*****
	//@TODO: Create a way to reject this call easily -- like objects that have no compatible modules.
	//*****
	//if( draw->getStatusBits() )
	//{
			draw->drawIconUI();
	//}

#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
	// debug collision extents
	if( TheGlobalData->m_showCollisionExtents )
	  drawDrawableExtents( draw, userData );

  if ( TheGlobalData->m_showAudioLocations )
    drawAudioLocations( draw, userData );
#endif

	// debug terrain normals at object positions
	if( TheGlobalData->m_showTerrainNormals )
	  drawTerrainNormal( draw, userData );

	TheGameClient->incrementRenderedObjectCount();

}  // end drawablePostDraw

//-------------------------------------------------------------------------------------------------
// Display AI debug visuals
//-------------------------------------------------------------------------------------------------
static void renderAIDebug( void )
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool W3DView::updateCameraMovements()
{
	Bool didUpdate = false;

	if (m_doingZoomCamera)
	{
		zoomCameraOneFrame();
		didUpdate = true;
	}
	if (m_doingPitchCamera)
	{
		pitchCameraOneFrame();
		didUpdate = true;
	}
	if (m_doingRotateCamera) {	
		m_previousLookAtPosition = *getPosition();
		rotateCameraOneFrame();
		didUpdate = true;
	} else if (m_doingMoveCameraOnWaypointPath) {
		m_previousLookAtPosition = *getPosition();
		moveAlongWaypointPath(TheW3DFrameLengthInMsec);
		didUpdate = true;
	}
	if (m_doingScriptedCameraLock)
	{
		didUpdate = true;
	}
	return didUpdate;
}


/** This function performs all actions which affect the camera transform or 3D objects
	rendered in this frame.

   MW: I moved this code out out W3DView::draw() so that we can get final camera and object
   positions before any rendering begins.  This was necessary so that reflection textures
   (which update before the main rendering loop) could get a correct version of the scene.
   Without this change, the reflections were always 1 frame behind the non-reflected view.
*/
void W3DView::updateView(void)
{
	UPDATE();
}

//DECLARE_PERF_TIMER(W3DView_updateView)
void W3DView::update(void)
{
	//USE_PERF_TIMER(W3DView_updateView)
	Bool recalcCamera = false;
	Bool didScriptedMovement = false;
#ifdef LOG_FRAME_TIMES
	__int64 curTime64,freq64;
	static __int64 prevTime64=0;
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq64);
	QueryPerformanceCounter((LARGE_INTEGER *)&curTime64);
	freq64 /= 1000;

	Int elapsedTimeMs = (curTime64 - prevTime64)/freq64;
	prevTime64 = curTime64;

#endif

//	Int elapsedTimeMs = TheW3DFrameLengthInMsec; // Assume a constant time flow.  It just works out better.  jba.

	if (TheTerrainRenderObject->doesNeedFullUpdate()) {
		RefRenderObjListIterator *it = W3DDisplay::m_3DScene->createLightsIterator();
		TheTerrainRenderObject->updateCenter(m_3DCamera, it);
		if (it) 
		{
		 W3DDisplay::m_3DScene->destroyLightsIterator(it);
		 it = NULL;
		}
	}

	static Real followFactor = -1;
	ObjectID cameraLock = getCameraLock();
	if (cameraLock == INVALID_ID) 
	{
		followFactor = -1;
	}
	if (cameraLock != INVALID_ID)
	{
		m_doingMoveCameraOnWaypointPath = false;
		Object* cameraLockObj = TheGameLogic->findObjectByID(cameraLock);
		Bool loseLock = false;

		// check if object has been destroyed or is dead -> lose lock
		if (cameraLockObj == NULL)
		{
			loseLock = true;
		}
#if 0
		else
		{
			AIUpdateInterface *ai = cameraLockObj->getAIUpdateInterface();
			if (ai && ai->isDead())
				loseLock = true;
		}
#endif


		if (loseLock)
		{
			setCameraLock(INVALID_ID);
			setCameraLockDrawable(NULL);
			followFactor = -1;
		}
		else
		{
			if (followFactor<0) {
				followFactor = 0.05f;
			} else {
				followFactor += 0.05f;
				if (followFactor>1.0f) followFactor = 1.0f;
			}
			if (getCameraLockDrawable() != NULL)
			{
				Drawable* cameraLockDrawable = (Drawable *)getCameraLockDrawable();

				if (!cameraLockDrawable)
				{
					setCameraLockDrawable(NULL);
				}
				else
				{
					Coord3D pos;
					Real boundingSphereRadius;
					Matrix3D transform;
					// this method must ONLY be called from the client, NEVER From the logic, not even indirectly.
					if (cameraLockDrawable->clientOnly_getFirstRenderObjInfo(&pos, &boundingSphereRadius, &transform))
					{
						Vector3 zaxis(0,0,1);

						Vector3 objPos;
						objPos.X = pos.x;
						objPos.Y = pos.y;
						objPos.Z = pos.z;

						//get position of top of object, assuming world z roughly along local z.
						objPos += boundingSphereRadius * 1.0f * zaxis;
						Vector3 objview = transform.Get_X_Vector();	//get view vector of object
						//move camera back behind object far enough not to intersect bounding sphere
						Vector3 camtran = objPos - objview * boundingSphereRadius*4.5f;

						Vector3 prevCamTran = m_3DCamera->Get_Position();	//get current camera position.

						Vector3 tranDiff = (camtran - prevCamTran);	//vector old position to new position.

						camtran = prevCamTran + tranDiff * 0.1f;	//slowly move camera to new position.

						Matrix3D camXForm;
						camXForm.Look_At(camtran,objPos,0);
						m_3DCamera->Set_Transform(camXForm);

						recalcCamera = false;	//we already did it
					}
				}
			}
			else
			{	Coord3D objpos = *cameraLockObj->getPosition();
				Coord3D curpos = *getPosition();
				// don't "snap" directly to the pos, but move there smoothly.
				Real snapThreshSqr = sqr(TheGlobalData->m_partitionCellSize);
				Real curDistSqr = sqr(curpos.x - objpos.x) + sqr(curpos.y - objpos.y);
				if ( m_snapImmediate)
				{
					// close enough.
					curpos.x = objpos.x;
					curpos.y = objpos.y;
				}
				else
				{
//					Real ratio = 1.0f - snapThreshSqr/curDistSqr;
					Real dx = objpos.x-curpos.x;
					Real dy = objpos.y-curpos.y;
					if (m_lockType == LOCK_TETHER)
					{
						//snapThreshSqr = sqr( m_lockDist * TheGlobalData->m_partitionCellSize );
						if (curDistSqr >= snapThreshSqr)
						{
							Real ratio = 1.0f - snapThreshSqr/curDistSqr;
							
							// move halfway there.
							curpos.x += dx*ratio*0.5f;
							curpos.y += dy*ratio*0.5f;
						}
						else
						{
							// we're inside our 'play' tolerance.  Move slowly to the obj
							Real ratio = 0.01f * m_lockDist;
							Real dx = objpos.x-curpos.x;
							Real dy = objpos.y-curpos.y;
							curpos.x += dx*ratio;
							curpos.y += dy*ratio;
						}
					}
					else
					{
						curpos.x += dx*followFactor;
						curpos.y += dy*followFactor;
					}
				}
				if (!(TheScriptEngine->isTimeFrozenDebug() || TheScriptEngine->isTimeFrozenScript()) && !TheGameLogic->isGamePaused()) {
					m_previousLookAtPosition = *getPosition();
				}
				setPosition(&curpos);

				if (m_lockType == LOCK_FOLLOW)
				{
					// camera follow objects if they are flying
					if (cameraLockObj->isUsingAirborneLocomotor() && cameraLockObj->isAboveTerrainOrWater())
					{
						Matrix3D camXForm;
						Real oldZRot = m_angle;
						Real idealZRot = cameraLockObj->getOrientation() - M_PI_2;
						normAngle(oldZRot);
						normAngle(idealZRot);
						Real diff = idealZRot - oldZRot;
						normAngle(diff);

						if (m_snapImmediate)
						{
							m_angle = idealZRot;
						}
						else
						{
							m_angle += diff * 0.1f;
						}
						normAngle(m_angle);
					}
				}
				if (m_snapImmediate)
					m_snapImmediate = FALSE;

				m_groundLevel = objpos.z;
				didScriptedMovement = true;
				recalcCamera = true;
			}
		}
	}	

	if (!(TheScriptEngine->isTimeFrozenDebug()/* || TheScriptEngine->isTimeFrozenScript()*/) && !TheGameLogic->isGamePaused()) {
		// If we aren't frozen for debug, allow the camera to follow scripted movements.
		if (updateCameraMovements()) {
			didScriptedMovement = true;
			recalcCamera = true;
		}
	} else {
		if (m_doingRotateCamera || m_doingMoveCameraOnWaypointPath || m_doingPitchCamera || m_doingZoomCamera || m_doingScriptedCameraLock) {
			didScriptedMovement = true; // don't mess up the scripted movement
		}
	}
	//
	// Process camera shake
	/// @todo Make this framerate-independent
	//
	if (m_shakeIntensity > 0.01f)
	{
		m_shakeOffset.x = m_shakeIntensity * m_shakeAngleCos;
		m_shakeOffset.y = m_shakeIntensity * m_shakeAngleSin;

		// fake a stiff spring/damper
		const Real dampingCoeff = 0.75f;
		m_shakeIntensity *= dampingCoeff;

		// spring is so "stiff", it pulls 180 degrees opposite each frame
		m_shakeAngleCos = -m_shakeAngleCos;
		m_shakeAngleSin = -m_shakeAngleSin;

		recalcCamera = true;
	}
	else
	{
		m_shakeIntensity = 0.0f;
		m_shakeOffset.x = 0.0f;
		m_shakeOffset.y = 0.0f;
	}

	/*
	 * In order to have the camera follow the terrain in a non-dizzying way, we will have a
	 * "desired height" value that the user sets.  While scrolling, the actual height (set by
	 * zoom) will not get updated unless we are scrolling uphill and our view either goes
	 * underground or higher than the max allowed height.  When the camera is at rest (not
	 * scrolling), the zoom will move toward matching the desired height.
	 */
	m_terrainHeightUnderCamera = getHeightAroundPos(m_pos.x, m_pos.y);
	m_currentHeightAboveGround = m_cameraOffset.z * m_zoom - m_terrainHeightUnderCamera;
	if (TheTerrainLogic && TheGlobalData && TheInGameUI && m_okToAdjustHeight && !TheGameLogic->isGamePaused())
	{
		Real desiredHeight = (m_terrainHeightUnderCamera + m_heightAboveGround);
		Real desiredZoom = desiredHeight / m_cameraOffset.z;
  	if (didScriptedMovement || (TheGameLogic->isInReplayGame() && TheGlobalData->m_useCameraInReplay))
  	{
  		// if we are in a scripted camera movement, take its height above ground as our desired height.
  		m_heightAboveGround = m_currentHeightAboveGround;
			//DEBUG_LOG(("Frame %d: height above ground: %g %g %g %g\n", TheGameLogic->getFrame(), m_heightAboveGround,
			//	m_cameraOffset.z, m_zoom, m_terrainHeightUnderCamera));
  	}
		if (TheInGameUI->isScrolling())
		{
			// if scrolling, only adjust if we're too close or too far
			if (m_scrollAmount.length() < m_scrollAmountCutoff || (m_currentHeightAboveGround < m_minHeightAboveGround) || (TheGlobalData->m_enforceMaxCameraHeight && m_currentHeightAboveGround > m_maxHeightAboveGround))
			{
				Real zoomAdj = (desiredZoom - m_zoom)*TheGlobalData->m_cameraAdjustSpeed;
				if (fabs(zoomAdj) >= 0.0001)	// only do positive
				{
					m_zoom += zoomAdj;
					recalcCamera = true;
				}
			}
		}
		else
		{
			// we're not scrolling; settle toward desired height above ground
			Real zoomAdj = (m_zoom - desiredZoom)*TheGlobalData->m_cameraAdjustSpeed;
			Real zoomAdjAbs = fabs(zoomAdj);
			if (zoomAdjAbs >= 0.0001 && !didScriptedMovement)
			{
				//DEBUG_LOG(("W3DView::update() - m_zoom=%g, desiredHeight=%g\n", m_zoom, desiredZoom));
				m_zoom -= zoomAdj;
				recalcCamera = true;
			}
		}
	}
	if (TheScriptEngine->isTimeFast()) {
		return; // don't draw - makes it faster :) jba.
	}
	if (recalcCamera)
		setCameraTransform();

	Region3D axisAlignedRegion;
	getAxisAlignedViewRegion(axisAlignedRegion);

	// render all of the visible Drawables
	/// @todo this needs to use a real region partition or something
	if (WW3D::Get_Frame_Time())	//make sure some time actually elapsed
		TheGameClient->iterateDrawablesInRegion( &axisAlignedRegion, drawDrawable, this );
}

//-------------------------------------------------------------------------------------------------
/** Find region which contains all drawables in 3D space. */
//-------------------------------------------------------------------------------------------------
void W3DView::getAxisAlignedViewRegion(Region3D &axisAlignedRegion)
{
	//
	// get the 4 points in 3D space of the 4 corners of the view, we will use a z = 0.0f
	// value so that we can get everything ... even stuff below the terrain
	//
	Coord3D box[ 4 ];
	getScreenCornerWorldPointsAtZ( &box[ 0 ], &box[ 1 ], &box[ 2 ], &box[ 3 ], 0.0f );

	//
	// take those 4 corners projected into the world and create an axis aligned bounding
	// box, we will use this box to iterate the drawables in 3D space
	//
	axisAlignedRegion.lo = box[ 0 ];
	axisAlignedRegion.hi = box[ 0 ];
	for( Int i = 0; i < 4; i++ )
	{

		if( box[ i ].x < axisAlignedRegion.lo.x )
			axisAlignedRegion.lo.x = box[ i ].x;
		if( box[ i ].y < axisAlignedRegion.lo.y )
			axisAlignedRegion.lo.y = box[ i ].y;
		if( box[ i ].x > axisAlignedRegion.hi.x )
		  axisAlignedRegion.hi.x = box[ i ].x;
		if( box[ i ].y > axisAlignedRegion.hi.y )
		  axisAlignedRegion.hi.y = box[ i ].y;

	}  // end for i

	// low and high regions will be based of the extent of the map
	Region3D mapExtent;
	Real safeValue = 999999;
	TheTerrainLogic->getExtent( &mapExtent );
	axisAlignedRegion.lo.z = mapExtent.lo.z - safeValue;
	axisAlignedRegion.hi.z = mapExtent.hi.z + safeValue;

	// we want to overscan a little bit so that we get objects that are partially offscreen
	axisAlignedRegion.lo.x -= (DRAWABLE_OVERSCAN + m_guardBandBias.x);
	axisAlignedRegion.lo.y -= (DRAWABLE_OVERSCAN + m_guardBandBias.y + 60.0f );
	axisAlignedRegion.hi.x += (DRAWABLE_OVERSCAN + m_guardBandBias.x);
	axisAlignedRegion.hi.y += (DRAWABLE_OVERSCAN + m_guardBandBias.y);

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::setFadeParameters(Int fadeFrames, Int direction)
{
	ScreenBWFilter::setFadeParameters(fadeFrames, direction);
	ScreenCrossFadeFilter::setFadeParameters(fadeFrames,direction);
}

void W3DView::set3DWireFrameMode(Bool enable)
{
	m_nextWireFrameEnabled = enable;
}

//-------------------------------------------------------------------------------------------------
/** Sets the view filter mode. */
//-------------------------------------------------------------------------------------------------
void W3DView::setViewFilterPos(const Coord3D *pos)
{
	ScreenMotionBlurFilter::setZoomToPos(pos);
}
//-------------------------------------------------------------------------------------------------
/** Sets the view filter mode. */
//-------------------------------------------------------------------------------------------------
Bool W3DView::setViewFilterMode(FilterModes filterMode)
{
	FilterModes oldMode = m_viewFilterMode;	//save previous mode in case setup fails.

	m_viewFilterMode = filterMode;
	if (m_viewFilterMode != FM_NULL_MODE && 
		m_viewFilter != FT_NULL_FILTER) {
		if (!W3DShaderManager::filterSetup(m_viewFilter, m_viewFilterMode))
		{	//setup failed so restore previous mode.
			m_viewFilterMode = oldMode;
			return FALSE;
		}
	}
	return TRUE;
}
//-------------------------------------------------------------------------------------------------
/** Sets the view filter. */
//-------------------------------------------------------------------------------------------------
Bool W3DView::setViewFilter(FilterTypes filter)
{
	FilterTypes oldFilter = m_viewFilter;	//save previous filter in case setup fails.

	m_viewFilter = filter;
	if (m_viewFilterMode != FM_NULL_MODE && 
		m_viewFilter != FT_NULL_FILTER) {
		if (!W3DShaderManager::filterSetup(m_viewFilter, m_viewFilterMode))
		{	//setup failed so restore previous mode.
			m_viewFilter = oldFilter;
			return FALSE;
		};
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
/** Calculates how many pixels we scrolled since last frame for motion blur calculations. */
//-------------------------------------------------------------------------------------------------
void W3DView::calcDeltaScroll(Coord2D &screenDelta)
{
	screenDelta.x = 0;
	screenDelta.y = 0;
	Vector3 prevPos(m_previousLookAtPosition.x,m_previousLookAtPosition.y, m_groundLevel);
	Vector3 prevScreen;
	if (m_3DCamera->Project( prevScreen, prevPos ) != CameraClass::INSIDE_FRUSTUM)
	{
		return;
	}
	Vector3 pos(m_pos.x,m_pos.y, m_groundLevel);
	Vector3 screen;
	if (m_3DCamera->Project( screen, pos ) != CameraClass::INSIDE_FRUSTUM)
	{
		return;
	}
	screenDelta.x = screen.X-prevScreen.X;
	screenDelta.y = screen.Y-prevScreen.Y;
}


//-------------------------------------------------------------------------------------------------
/** Draw member for the W3D window, this will literally draw the window 
  * for this view */
//-------------------------------------------------------------------------------------------------
void W3DView::drawView( void )
{
	DRAW();
}

//DECLARE_PERF_TIMER(W3DView_drawView)
void W3DView::draw( void )
{
	//USE_PERF_TIMER(W3DView_drawView)
	Bool skipRender = false;
	Bool doExtraRender = false;
	CustomScenePassModes customScenePassMode  = SCENE_PASS_DEFAULT;

	if (m_viewFilterMode && 
			m_viewFilter > FT_NULL_FILTER && 
			m_viewFilter < FT_MAX)
	{	
		// Most likely will redirect rendering to a texture.
		W3DShaderManager::filterPreRender(m_viewFilter, skipRender, customScenePassMode);
		if (!skipRender && getCameraLock()) 
		{
			Object* cameraLockObj = TheGameLogic->findObjectByID(getCameraLock());
			if (cameraLockObj) 
			{
				Drawable *drawable = cameraLockObj->getDrawable();
				drawable->setDrawableHidden(true);
			}
		}
	}

	if (!skipRender) 
	{
		// Render 3D scene from our camera
		W3DDisplay::m_3DScene->setCustomPassMode(customScenePassMode);
		if (m_isWireFrameEnabled)
			W3DDisplay::m_3DScene->Set_Extra_Pass_Polygon_Mode(SceneClass::EXTRA_PASS_CLEAR_LINE);
		W3DDisplay::m_3DScene->doRender( m_3DCamera );
		W3DDisplay::m_3DScene->Set_Extra_Pass_Polygon_Mode(SceneClass::EXTRA_PASS_DISABLE);
		m_isWireFrameEnabled = m_nextWireFrameEnabled;
	}

	if (m_viewFilterMode && 
			m_viewFilter > FT_NULL_FILTER && 
			m_viewFilter < FT_MAX)
	{	
		Coord2D deltaScroll;
		calcDeltaScroll(deltaScroll);
		Bool continueTheEffect;
		continueTheEffect = W3DShaderManager::filterPostRender(m_viewFilter, m_viewFilterMode, deltaScroll,doExtraRender);
		if (!skipRender && getCameraLock()) 
		{
			Object* cameraLockObj = TheGameLogic->findObjectByID(getCameraLock());
			if (cameraLockObj) 
			{
				Drawable *drawable = cameraLockObj->getDrawable();
				drawable->setDrawableHidden(false);
				RenderInfoClass rinfo(*m_3DCamera);
				// Apply the camera and viewport (including depth range)
				m_3DCamera->Apply();
				TheDX8MeshRenderer.Set_Camera(&rinfo.Camera);
				W3DDisplay::m_3DScene->renderSpecificDrawables(rinfo, 1, &drawable);
				WW3D::Flush(rinfo);
			}
		}
		if (!continueTheEffect) 
		{
			// shut it down.
			m_viewFilter = FT_NULL_FILTER;
			m_viewFilterMode = FM_NULL_MODE;
		}
	}

	//Some effects require that we render a modified version of the scene into a texture but also require
	//an unaltered version in the framebuffer.  So we re-render again into framebuffer after texture rendering
	//was turned off by filterPostRender().
	if (doExtraRender)
	{
		//Reset to normal scene rendering.
		//The pass that rendered into a texture may have left the z-buffer in a weird state
		//so clear it before rendering normal scene.
		///@todo: Don't clear z-buffer unless shader uses z-bias or anything else that would cause <= z to fail on normal render.
		DX8Wrapper::Clear(false, true, Vector3(0.0f,0.0f,0.0f), TheWaterTransparency->m_minWaterOpacity);	// Clear z but not color
		W3DDisplay::m_3DScene->setCustomPassMode(SCENE_PASS_DEFAULT);
		W3DDisplay::m_3DScene->doRender( m_3DCamera );
		Coord2D deltaScroll;
		W3DShaderManager::filterPostRender(m_viewFilter, m_viewFilterMode, deltaScroll, doExtraRender);
	}

	if( TheGlobalData->m_debugAI )
	{
		if (TheAI->pathfinder()->getDebugPath())
		{
			// setup screen clipping region
			IRegion2D clipRegion;
			clipRegion.lo.x = 0;
			clipRegion.lo.y = 0;
			clipRegion.hi.x = getWidth();
			clipRegion.hi.y = getHeight();

			UnsignedInt color = 0xFFFFFF00;  //0xAARRGGBB
			ICoord2D start, end;
			PathNode *prevNode = TheAI->pathfinder()->getDebugPath()->getFirstNode();

			if (worldToScreen( prevNode->getPosition(), &start )) {
				TheDisplay->drawLine( start.x-3, start.y-3, start.x+3, start.y-3, 1.0f, color );
				TheDisplay->drawLine( start.x+3, start.y-3, start.x+3, start.y+3, 1.0f, color );
				TheDisplay->drawLine( start.x+3, start.y+3, start.x-3, start.y+3, 1.0f, color );
				TheDisplay->drawLine( start.x-3, start.y+3, start.x-3, start.y-3, 1.0f, color );
			}
			for( PathNode *node = prevNode->getNext(); node; node = node->getNext() )
			{
				Int k;
				Coord3D s, e;
				Coord3D delta;
				s = *node->getPosition();
				e = *prevNode->getPosition();
				delta.x = e.x-s.x;
				delta.y = e.y-s.y;
				delta.z = e.z-s.z;
				for (k = 0; k<10; k++) {
					Real factor1 = (k)/10.0;
					Real factor2 = (k+1)/10.0;
					s = *node->getPosition();
					e = *node->getPosition();
					s.x += delta.x*factor1;
					s.y += delta.y*factor1;
					s.z += delta.z*factor1;
					e.x += delta.x*factor2;
					e.y += delta.y*factor2;
					e.z += delta.z*factor2;
					Bool onScreen1 = worldToScreen( &e, &end );
					Bool onScreen2 = worldToScreen( &s, &start );
					if (!onScreen1 && !onScreen2) {
						continue; // neither point visible.
					}
					ICoord2D clipStart, clipEnd;

					if( ClipLine2D( &start, &end, &clipStart, &clipEnd, &clipRegion ) ) {
						TheDisplay->drawLine( clipStart.x, clipStart.y, clipEnd.x, clipEnd.y, 1.0f, color );
					}
				}
				prevNode = node;
				if (node->getNext()) {
					if (worldToScreen( node->getPosition(), &start )) {
 						TheDisplay->drawLine( start.x-4, start.y, start.x+3, start.y, 1.0f, color );
					}
				}
			}
			if (prevNode && worldToScreen( prevNode->getPosition(), &start )) {
 				TheDisplay->drawLine( start.x-4, start.y, start.x+3, start.y, 1.0f, color );
				TheDisplay->drawLine( start.x, start.y-4, start.x, start.y+3, 1.0f, color );
			}
			color = 0xFFFF0000;  //0xAARRGGBB
			if (worldToScreen( TheAI->pathfinder()->getDebugPathPosition(), &start )) {
				TheDisplay->drawLine( start.x-3, start.y, start.x+3, start.y, 1.0f, color );
				TheDisplay->drawLine( start.x, start.y-3, start.x, start.y+3, 1.0f, color );
			}
		}

	}  // end if, show debug AI

#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
	if( TheGlobalData->m_debugCamera )
	{
		UnsignedInt c = 0xaaffffff;
		Coord3D worldPos = *getPosition();
		worldPos.z = TheTerrainLogic->getGroundHeight(worldPos.x, worldPos.y);

		Coord3D p1, p2;
		ICoord2D s1, s2;
		p1 = worldPos;
		p1.x += TERRAIN_SAMPLE_SIZE;
		p1.y += TERRAIN_SAMPLE_SIZE;
		p1.z = TheTerrainLogic->getGroundHeight(p1.x, p1.y);
		p2 = worldPos;
		p2.x += TERRAIN_SAMPLE_SIZE;
		p2.y -= TERRAIN_SAMPLE_SIZE;
		p2.z = TheTerrainLogic->getGroundHeight(p2.x, p2.y);
		worldToScreen( &p1, &s1 );
		worldToScreen( &p2, &s2 );
		TheDisplay->drawLine(s1.x, s1.y, s2.x, s2.y, 1.0f, c);

		p1 = worldPos;
		p1.x += TERRAIN_SAMPLE_SIZE;
		p1.y -= TERRAIN_SAMPLE_SIZE;
		p1.z = TheTerrainLogic->getGroundHeight(p1.x, p1.y);
		p2 = worldPos;
		p2.x -= TERRAIN_SAMPLE_SIZE;
		p2.y -= TERRAIN_SAMPLE_SIZE;
		p2.z = TheTerrainLogic->getGroundHeight(p2.x, p2.y);
		worldToScreen( &p1, &s1 );
		worldToScreen( &p2, &s2 );
		TheDisplay->drawLine(s1.x, s1.y, s2.x, s2.y, 1.0f, c);

		p1 = worldPos;
		p1.x -= TERRAIN_SAMPLE_SIZE;
		p1.y -= TERRAIN_SAMPLE_SIZE;
		p1.z = TheTerrainLogic->getGroundHeight(p1.x, p1.y);
		p2 = worldPos;
		p2.x -= TERRAIN_SAMPLE_SIZE;
		p2.y += TERRAIN_SAMPLE_SIZE;
		p2.z = TheTerrainLogic->getGroundHeight(p2.x, p2.y);
		worldToScreen( &p1, &s1 );
		worldToScreen( &p2, &s2 );
		TheDisplay->drawLine(s1.x, s1.y, s2.x, s2.y, 1.0f, c);

		p1 = worldPos;
		p1.x -= TERRAIN_SAMPLE_SIZE;
		p1.y += TERRAIN_SAMPLE_SIZE;
		p1.z = TheTerrainLogic->getGroundHeight(p1.x, p1.y);
		p2 = worldPos;
		p2.x += TERRAIN_SAMPLE_SIZE;
		p2.y += TERRAIN_SAMPLE_SIZE;
		p2.z = TheTerrainLogic->getGroundHeight(p2.x, p2.y);
		worldToScreen( &p1, &s1 );
		worldToScreen( &p2, &s2 );
		TheDisplay->drawLine(s1.x, s1.y, s2.x, s2.y, 1.0f, c);

	}

  if ( TheGlobalData->m_showAudioLocations )
  {
    // Draw audio radii for ALL drawables, not just those on screen
    const Drawable * drawable = TheGameClient->getDrawableList();

    while ( drawable != NULL )
    {
      drawAudioRadii( drawable );
      drawable = drawable->getNextDrawable();
    }
  }
#endif // DEBUG or INTERNAL

	Region3D axisAlignedRegion;
	getAxisAlignedViewRegion(axisAlignedRegion);

	//
	// there are several things we might want to do as a post pass on the objects after
	// they are all drawn
	/// @todo we might want to consider wiping this iterate out if there is nothing to post draw
	//
	TheGameClient->resetRenderedObjectCount();
	TheGameClient->iterateDrawablesInRegion( &axisAlignedRegion, drawablePostDraw, this );

	TheGameClient->flushTextBearingDrawables();

	// Render 2D scene
	W3DDisplay::m_2DScene->doRender( m_2DCamera );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::setCameraLock(ObjectID id)
{		 
	// If we're disabling camera movements, don't lock onto the object.
	if (TheGlobalData->m_disableCameraMovement && id!=INVALID_ID) {
		return;
	}
	View::setCameraLock(id);
	m_doingScriptedCameraLock = FALSE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::setSnapMode( CameraLockType lockType, Real lockDist )
{
	View::setSnapMode(lockType, lockDist);
	m_doingScriptedCameraLock = TRUE;
}

//-------------------------------------------------------------------------------------------------
/** Scroll the view by the given delta in SCREEN COORDINATES, this interface 
	* assumes we will be scrolling along the X,Y plane */
//-------------------------------------------------------------------------------------------------
void W3DView::scrollBy( Coord2D *delta )
{
	// if we haven't moved, ignore
	if( delta && (delta->x != 0 || delta->y != 0) )
	{
		const Real SCROLL_RESOLUTION = 250.0f;

		Vector3 world, worldStart, worldEnd;
		Vector2 screen, start, end;

		m_scrollAmount = *delta;

		screen.X = delta->x;
		screen.Y = delta->y;
													  
		start.X = getWidth();
		start.Y = getHeight();
		Real aspect = getHeight() == 0 ? 1 : getWidth()/getHeight();
		end.X = start.X + delta->x * SCROLL_RESOLUTION;
		end.Y = start.Y + delta->y * SCROLL_RESOLUTION*aspect;

		m_3DCamera->Device_To_World_Space( start, &worldStart );
		m_3DCamera->Device_To_World_Space( end, &worldEnd );

		world.X = worldEnd.X - worldStart.X;
		world.Y = worldEnd.Y - worldStart.Y;
		world.Z = worldEnd.Z - worldStart.Z;

		// scroll by delta
		Coord3D pos = *getPosition();
		pos.x += world.X;
		pos.y += world.Y;
		//DEBUG_LOG(("Delta %.2f, %.2f\n", world.X, world.Z));
		// no change to z
		setPosition(&pos);

		//m_cameraConstraintValid = false;	// pos change does NOT invalidate cam constraints

		m_doingRotateCamera = false;
		// set new camera position
		setCameraTransform();

	}  // end if

}  // end scrollBy

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::forceRedraw()
{

	// set the camera
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
/** Rotate the view around the up axis to the given angle. */
//-------------------------------------------------------------------------------------------------
void W3DView::setAngle( Real angle )
{
	// Normalize to +-PI.
	normAngle(angle);
	// call our base class, we are adding functionality
	View::setAngle( angle );


	m_doingMoveCameraOnWaypointPath = false;
	m_doingRotateCamera = false;
	m_doingPitchCamera = false;
	m_doingZoomCamera = false;
	m_doingScriptedCameraLock = false;
	// set the camera
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
/** Rotate the view around the horizontal (X) axis to the given angle. */
//-------------------------------------------------------------------------------------------------
void W3DView::setPitch( Real angle )
{
	// call our base class, we are extending functionality
	View::setPitch( angle );


	m_doingMoveCameraOnWaypointPath = false;
	m_doingRotateCamera = false;
	m_doingPitchCamera = false;
	m_doingZoomCamera = false;
	m_doingScriptedCameraLock = false;
	// set the camera
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
/** Set the view angle & pitch back to default */
//-------------------------------------------------------------------------------------------------
void W3DView::setAngleAndPitchToDefault( void )
{ 
	// call our base class, we are adding functionality
	View::setAngleAndPitchToDefault();

	this->m_FXPitch = 1.0;

	// set the camera
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::setDefaultView(Real pitch, Real angle, Real maxHeight)
{
	// MDC - we no longer want to rotate maps (design made all of them right to begin with)
	//	m_defaultAngle = angle * M_PI/180.0f;
	m_defaultPitchAngle = pitch;
	m_maxHeightAboveGround = TheGlobalData->m_maxCameraHeight*maxHeight;
	if (m_minHeightAboveGround > m_maxHeightAboveGround)
		m_maxHeightAboveGround = m_minHeightAboveGround;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::setHeightAboveGround(Real z)
{
	m_heightAboveGround = z;

  // if our zoom is limited, we will stay within a predefined distance from the terrain
	if( m_zoomLimited )
	{

		if (m_heightAboveGround < m_minHeightAboveGround)
			m_heightAboveGround = m_minHeightAboveGround;

		if (m_heightAboveGround > m_maxHeightAboveGround)
			m_heightAboveGround = m_maxHeightAboveGround;

	}  // end if

	m_doingMoveCameraOnWaypointPath = false;
	m_doingRotateCamera = false;
	m_doingPitchCamera = false;
	m_doingZoomCamera = false;
	m_doingScriptedCameraLock = false;
	m_cameraConstraintValid = false; // recalc it.
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::setZoom(Real z)
{
	m_zoom = z;

	if (m_zoom < m_minZoom)
		m_zoom = m_minZoom;

	if (m_zoom > m_maxZoom)
		m_zoom = m_maxZoom;

	m_doingMoveCameraOnWaypointPath = false;
	m_doingRotateCamera = false;
	m_doingPitchCamera = false;
	m_doingZoomCamera = false;
	m_doingScriptedCameraLock = false;
	m_cameraConstraintValid = false; // recalc it.
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::setZoomToDefault( void )
{
	// default zoom has to be max, otherwise players will just zoom to max always

	// terrain height + desired height offset == cameraOffset * actual zoom
	// find best approximation of max terrain height we can see
	Real terrainHeightMax = getHeightAroundPos(m_pos.x, m_pos.y);

	Real desiredHeight = (terrainHeightMax + m_maxHeightAboveGround);
	Real desiredZoom = desiredHeight / m_cameraOffset.z;

	m_mcwpInfo.cameraZoom[2] = desiredZoom;//m_maxZoom;
	DEBUG_LOG(("W3DView::setZoomToDefault() Current zoom: %g  Desired zoom: %g\n", m_zoom, desiredZoom));

	m_zoom = desiredZoom;
	m_heightAboveGround = m_maxHeightAboveGround;

	m_doingMoveCameraOnWaypointPath = false;
	m_doingRotateCamera = false;
	m_doingPitchCamera = false;
	m_doingZoomCamera = false;
	m_doingScriptedCameraLock = false;
	m_cameraConstraintValid = false; // recalc it.
	setCameraTransform();
}

//-------------------------------------------------------------------------------------------------
/** Set the horizontal field of view angle */
//-------------------------------------------------------------------------------------------------
void W3DView::setFieldOfView( Real angle )
{
	View::setFieldOfView( angle );

#if defined(RTS_DEBUG) || defined(RTS_INTERNAL)
	// this is only for testing, and recalculating the 
	// camera every frame is wasteful
	setCameraTransform();
#endif
}

//-------------------------------------------------------------------------------------------------
/** Using the W3D camera translate the world coordinate to a screen coord.
	Screen coordinates returned in absolute values relative to full display resolution.  
  Returns if the point is on screen, off screen, or not transformable */
//-------------------------------------------------------------------------------------------------
View::WorldToScreenReturn W3DView::worldToScreenTriReturn( const Coord3D *w, ICoord2D *s )
{
	// sanity
	if( w == NULL || s == NULL )
    return WTS_INVALID;

	if( m_3DCamera )
	{
		Vector3 world;
		Vector3 screen;

		world.Set( w->x, w->y, w->z );
		enum CameraClass::ProjectionResType projection = m_3DCamera->Project( screen, world );
		if (projection != CameraClass::INSIDE_FRUSTUM && projection!=CameraClass::OUTSIDE_FRUSTUM)
		{
			// Can't get a valid number if it's beyond the clip planes.  jba
			s->x = 0;
			s->y = 0;
      return WTS_INVALID;
		}

		//
		// note that the screen coord returned from the project W3D camera 
		// gave us a screen coords that range from (-1,-1) bottom left to
		// (1,1) top right ... we are turning that into (0,0) upper left
		// coords now
		//
		W3DLogicalScreenToPixelScreen( screen.X, screen.Y,
																	 &s->x, &s->y,
																	 getWidth(), getHeight());
		s->x += m_originX;	//convert viewport coordinates to full screen coordinates
		s->y += m_originY;

//		s->x = (getWidth()  * (screen.X + 1.0f)) / 2.0f;
//		s->y = (getHeight() * (-screen.Y + 1.0f)) / 2.0f;
		if (projection != CameraClass::INSIDE_FRUSTUM)
		{
      return WTS_OUTSIDE_FRUSTUM;
		}

    return WTS_INSIDE_FRUSTUM;

	}  // end if

  return WTS_INVALID;
}  // end worldToScreenTriReturn

//-------------------------------------------------------------------------------------------------
/** Using the W3D camera translate the screen coord to world coord */
//-------------------------------------------------------------------------------------------------
void W3DView::screenToWorld( const ICoord2D *s, Coord3D *w )
{

	// sanity
	if( s == NULL || w == NULL )
		return;

	if( m_3DCamera )
	{
		DEBUG_CRASH(("implement me"));
	}  // end if

}  // end screenToWorld

//-------------------------------------------------------------------------------------------------
/** all the drawables in the view, that fall within the 2D screen region
	* will call the callback function.  The number of drawables that passed
	* the test are returned.
	Screen coordinates assumed in absolute values relative to full display resolution. */
//-------------------------------------------------------------------------------------------------
Int W3DView::iterateDrawablesInRegion( IRegion2D *screenRegion,
																			 Bool (*callback)( Drawable *draw, void *userData ),
																			 void *userData )
{
	Bool inside = FALSE;
	Int count = 0;
	Drawable *draw;
	Vector3 screen, world;
	Coord3D pos;
	Region2D normalizedRegion;

	/** @todo we need to have partitions of which drawables are in the
	view so we don't have to march through the whole list */

	//
	// to do this we are projecting the drawable centers onto the screen,
	// the W3D camera->project method is used to do this and that method
	// will return normalized screen coords from (-1,-1) bottom left to 
	// (1,1) top right, normalize our screen region for comparison
	//
	/// @todo use fast int->real type casts here later

	Bool regionIsPoint = FALSE;

	if( screenRegion )
	{
		if (screenRegion->height() == 0 && screenRegion->width() == 0)
		{
			regionIsPoint = TRUE;
		} 

		normalizedRegion.lo.x = ((Real)(screenRegion->lo.x - m_originX) / (Real)getWidth()) * 2.0f - 1.0f;
		normalizedRegion.lo.y = -(((Real)(screenRegion->hi.y - m_originY) / (Real)getHeight()) * 2.0f - 1.0f);
		normalizedRegion.hi.x = ((Real)(screenRegion->hi.x - m_originX) / (Real)getWidth()) * 2.0f - 1.0f;
		normalizedRegion.hi.y = -(((Real)(screenRegion->lo.y - m_originY) / (Real)getHeight()) * 2.0f - 1.0f);

	}  // end if


	Drawable *onlyDrawableToTest = NULL;
	if (regionIsPoint)
	{
		// Allow all drawables to be picked.
		onlyDrawableToTest = pickDrawable(&screenRegion->lo, TRUE, (PickType) getPickTypesForContext(TheInGameUI->isInForceAttackMode()));
		if (onlyDrawableToTest == NULL) {
			return 0;
		}
	}

	for( draw = TheGameClient->firstDrawable();
			 draw;
			 draw = draw->getNextDrawable() )
	{
		if (onlyDrawableToTest)
		{
		 draw = onlyDrawableToTest;
		 inside = TRUE;
		}
		else
		{

			// not inside	
			inside = FALSE;

			// no screen region, means all drawbles
			if( screenRegion == NULL )
				inside = TRUE;
			else
			{

				// project the center of the drawable to the screen
				/// @todo use a real 3D position in the drawable
				pos = *draw->getPosition();
				world.X = pos.x;
				world.Y = pos.y;
				world.Z = pos.z;

				// project the world point to the screen
				if( m_3DCamera->Project( screen, world ) == CameraClass::INSIDE_FRUSTUM &&
						screen.X >= normalizedRegion.lo.x && 
						screen.X <= normalizedRegion.hi.x &&
						screen.Y >= normalizedRegion.lo.y && 
						screen.Y <= normalizedRegion.hi.y )
				{

					inside = TRUE;

				}  // end if
			}
	
		}  //end else

		// if inside do the callback and count up
		if( inside )
		{
			
			if( callback( draw, userData ) )
				++count;

		}  // end if

		// If onlyDrawableToTest, then we should bail out now.
		if (onlyDrawableToTest != NULL)
			break;

	}  // end for draw

	return count;

}  // end iterateDrawablesInRegion

//-------------------------------------------------------------------------------------------------
/** cast a ray from the screen coords into the scene and return a drawable
  * there if present. Screen coordinates assumed in absolute values relative
  * to full display resolution. */
//-------------------------------------------------------------------------------------------------
Drawable *W3DView::pickDrawable( const ICoord2D *screen, Bool forceAttack, PickType pickType )
{
	RenderObjClass *renderObj = NULL;
	Drawable *draw = NULL;
	DrawableInfo *drawInfo = NULL;

	// sanity
	if( screen == NULL )
		return NULL;

	// don't pick a drawable if there is a window under the cursor
	GameWindow *window = NULL;
	if (TheWindowManager)
		window = TheWindowManager->getWindowUnderCursor(screen->x, screen->y);

	while (window)
	{
		// check to see if it or any of its parents are opaque.  If so, we can't select anything.
		if (!BitIsSet( window->winGetStatus(), WIN_STATUS_SEE_THRU ))
			return NULL;

		window = window->winGetParent();
	}

	Vector3 rayStart,rayEnd;
	getPickRay(screen,&rayStart,&rayEnd);

	LineSegClass lineseg;
	lineseg.Set(rayStart,rayEnd);

	CastResultStruct result;

	if (forceAttack) 
		result.ComputeContactPoint = true;

	//Don't check against translucent or hidden objects
	RayCollisionTestClass raytest(lineseg,&result,COLL_TYPE_ALL,false,false);

	if( W3DDisplay::m_3DScene->castRay( raytest, false, (Int)pickType ) )
		renderObj = raytest.CollidedRenderObj;

	// for right now there is no drawable data in a render object which is			 	// if we've found a render object, return our drawable associated with it,

	// the terrain, therefore the userdata is NULL
	/// @todo terrain and picking!
	if( renderObj )
		drawInfo = (DrawableInfo *)renderObj->Get_User_Data();
	if (drawInfo)
		draw=drawInfo->m_drawable;

	return draw;

}  // end pickDrawable

//-------------------------------------------------------------------------------------------------
/** convert a pixel (x,y) to a location in the world on the terrain.
	Screen coordinates assumed in absolute values relative to full display resolution.  */
//-------------------------------------------------------------------------------------------------
void W3DView::screenToTerrain( const ICoord2D *screen, Coord3D *world )
{
	// sanity
	if( screen == NULL || world == NULL || TheTerrainRenderObject == NULL )
		return;

	if (m_cameraHasMovedSinceRequest) {
		m_locationRequests.clear();
		m_cameraHasMovedSinceRequest = false;
	}

	if (m_locationRequests.size() > MAX_REQUEST_CACHE_SIZE) {
		m_locationRequests.erase(m_locationRequests.begin(), m_locationRequests.begin() + 10);
	}


	// We insert them at the end for speed (no copies needed), but using the princ of locality, we should 
	// start searching where we most recently inserted
	for (int i = m_locationRequests.size() - 1; i >= 0; --i) {
		if (m_locationRequests[i].first.x == screen->x && m_locationRequests[i].first.y == screen->y) {
			(*world) = m_locationRequests[i].second;
			return;
		}
	}

	Vector3 rayStart,rayEnd;
	LineSegClass lineseg;
	CastResultStruct result;
	Vector3 intersection(0,0,0);

	getPickRay(screen,&rayStart,&rayEnd);

	lineseg.Set(rayStart,rayEnd);

	RayCollisionTestClass raytest(lineseg,&result);

	if( TheTerrainRenderObject->Cast_Ray(raytest) )
	{
		// get the point of intersection according to W3D
		intersection = result.ContactPoint;
		
	}  // end if

	// Pick bridges.  
	Vector3 bridgePt;
	Drawable *bridge = TheTerrainLogic->pickBridge(rayStart, rayEnd, &bridgePt);
	if (bridge && bridgePt.Z > intersection.Z) {
		intersection = bridgePt;
	}

	world->x = intersection.X;
	world->y = intersection.Y;
	world->z = intersection.Z;

	PosRequest req;
	req.first = (*screen);
	req.second = (*world);
	m_locationRequests.push_back(req);	// Insert this request at the end, requires no extra copies

}  // end screenToTerrain

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::lookAt( const Coord3D *o ) 
{
	Coord3D pos = *o;


// no, don't call the super-lookAt, since it will munge our coords
// as for a 2d view. just call setPosition.
//View::lookAt(&pos);

	if (o->z > PATHFIND_CELL_SIZE_F+TheTerrainLogic->getGroundHeight(pos.x, pos.y)) {
		// Pos.z is not used, so if we want to look at something off the ground, 
		// we have to look at the spot on the ground such that the object intersects
		// with the look at vector in the center of the screen.  jba.
		Vector3 rayStart,rayEnd;
		LineSegClass lineseg;
		CastResultStruct result;
		Vector3 intersection(0,0,0);

		rayStart = m_3DCamera->Get_Position();	//get camera location
		m_3DCamera->Un_Project(rayEnd,Vector2(0.0f,0.0f));	//get world space point
		rayEnd -= rayStart;	//vector camera to world space point
		rayEnd.Normalize();	//make unit vector
		rayEnd *= m_3DCamera->Get_Depth();	//adjust length to reach far clip plane
		rayStart.Set(pos.x, pos.y, pos.z);
		rayEnd += rayStart;	//get point on far clip plane along ray from camera.
		lineseg.Set(rayStart,rayEnd);

		RayCollisionTestClass raytest(lineseg,&result);

		if( TheTerrainRenderObject->Cast_Ray(raytest) )
		{
			// get the point of intersection according to W3D
			pos.x = result.ContactPoint.X;
			pos.y = result.ContactPoint.Y;
			
		}  // end if
	}			 
	pos.z = 0;
	setPosition(&pos); 
	m_doingRotateCamera = false;
	m_doingMoveCameraOnWaypointPath = false;
	m_doingScriptedCameraLock = false;

	setCameraTransform();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::initHeightForMap( void ) 
{
	m_groundLevel = TheTerrainLogic->getGroundHeight(m_pos.x, m_pos.y);
	const Real MAX_GROUND_LEVEL = 120.0; // jba - starting ground level can't exceed this height.
	if (m_groundLevel>MAX_GROUND_LEVEL) {
		m_groundLevel = MAX_GROUND_LEVEL;
	}

	m_cameraOffset.z = m_groundLevel+TheGlobalData->m_cameraHeight;
	m_cameraOffset.y = -(m_cameraOffset.z / tan(TheGlobalData->m_cameraPitch * (PI / 180.0)));
	m_cameraOffset.x = -(m_cameraOffset.y * tan(TheGlobalData->m_cameraYaw * (PI / 180.0)));
	m_cameraConstraintValid = false;	// possible ground level change invalidates cam constraints
	setCameraTransform();

}

//-------------------------------------------------------------------------------------------------
/** Move camera to in an interesting fashion.  Sets up parameters that get
 * evaluated in draw(). */
//-------------------------------------------------------------------------------------------------
void W3DView::moveCameraTo(const Coord3D *o, Int milliseconds, Int shutter, Bool orient)
{
	m_mcwpInfo.waypoints[0] = *getPosition();	
	m_mcwpInfo.cameraAngle[0] = getAngle();	
	m_mcwpInfo.waySegLength[0] = 0;	

	m_mcwpInfo.waypoints[1] = *getPosition();	
	m_mcwpInfo.waySegLength[1] = 0;	

	m_mcwpInfo.waypoints[2] = *o;	
	m_mcwpInfo.waySegLength[2] = 0;	

	m_mcwpInfo.numWaypoints = 2;
	if (milliseconds<1) milliseconds = 1;
	m_mcwpInfo.totalTimeMilliseconds = milliseconds;
	m_mcwpInfo.shutter = 1;
	m_mcwpInfo.curSegment = 1;
	m_mcwpInfo.curSegDistance = 0;
	m_mcwpInfo.totalDistance = 0;

	setupWaypointPath(orient);
	if (m_mcwpInfo.totalTimeMilliseconds==1) {
		// do it instantly.
		moveAlongWaypointPath(1);
		m_doingMoveCameraOnWaypointPath = true;
	}
}

//-------------------------------------------------------------------------------------------------
/** Rotate the camera */
//-------------------------------------------------------------------------------------------------
void W3DView::rotateCamera(Real rotations, Int milliseconds)
{
	m_rcInfo.numHoldFrames = 0;
	m_rcInfo.trackObject = FALSE;

	if (milliseconds<1) milliseconds = 1;
	m_rcInfo.numFrames = milliseconds/TheW3DFrameLengthInMsec;
	if (m_rcInfo.numFrames < 1) {
		m_rcInfo.numFrames = 1;
	}
	m_rcInfo.curFrame = 0;
	m_doingRotateCamera = true;
	m_rcInfo.angle = 2*PI*rotations/m_rcInfo.numFrames;
	m_rcInfo.startTimeMultiplier = m_timeMultiplier;
	m_rcInfo.endTimeMultiplier = m_timeMultiplier;

	m_doingMoveCameraOnWaypointPath = false;
}

//-------------------------------------------------------------------------------------------------
/** Rotate the camera to follow a unit */
//-------------------------------------------------------------------------------------------------
void W3DView::rotateCameraTowardObject(ObjectID id, Int milliseconds, Int holdMilliseconds)
{
	m_rcInfo.trackObject = TRUE;
	if (holdMilliseconds<1) holdMilliseconds = 0;
	m_rcInfo.numHoldFrames = holdMilliseconds/TheW3DFrameLengthInMsec;
	if (m_rcInfo.numHoldFrames < 1) {
		m_rcInfo.numHoldFrames = 0;
	}

	if (milliseconds<1) milliseconds = 1;
	m_rcInfo.numFrames = milliseconds/TheW3DFrameLengthInMsec;
	if (m_rcInfo.numFrames < 1) {
		m_rcInfo.numFrames = 1;
	}
	m_rcInfo.curFrame = 0;
	m_doingRotateCamera = true;
	m_rcInfo.angle = m_angle; // not used here
	m_rcInfo.targetObjectID = id;
	m_rcInfo.startTimeMultiplier = m_timeMultiplier;
	m_rcInfo.endTimeMultiplier = m_timeMultiplier;

	m_doingMoveCameraOnWaypointPath = false;
}

//-------------------------------------------------------------------------------------------------
/** Rotate camera to face a location */
//-------------------------------------------------------------------------------------------------
void W3DView::rotateCameraTowardPosition(const Coord3D *pLoc, Int milliseconds)
{
	m_rcInfo.numHoldFrames = 0;
	m_rcInfo.trackObject = FALSE;


	if (milliseconds<1) milliseconds = 1;
	m_rcInfo.numFrames = milliseconds/TheW3DFrameLengthInMsec;
	if (m_rcInfo.numFrames < 1) {
		m_rcInfo.numFrames = 1;
	}
	Coord3D curPos = *getPosition();
	Vector3 dir(pLoc->x-curPos.x, pLoc->y-curPos.y, 0);
	if (dir.Length()<0.1) return;
	dir.Normalize();
	Real angle = WWMath::Acos(dir.X);
	if (dir.Y<0) {
		angle = -angle;
	}
	// Default camera is rotated 90 degrees, so match.
	angle -= PI/2;
	normAngle(angle);

	m_rcInfo.curFrame = 0;
	m_doingRotateCamera = true;
	m_rcInfo.angle = angle/m_rcInfo.numFrames; // not used here
	m_rcInfo.targetObjectID = INVALID_ID;
	m_rcInfo.startTimeMultiplier = m_timeMultiplier;
	m_rcInfo.endTimeMultiplier = m_timeMultiplier;

	m_doingMoveCameraOnWaypointPath = false;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::zoomCamera( Real finalZoom, Int milliseconds )
{
	if (milliseconds<1) milliseconds = 1;
	m_zcInfo.numFrames = milliseconds/TheW3DFrameLengthInMsec;
	if (m_zcInfo.numFrames < 1) {
		m_zcInfo.numFrames = 1;
	}
	m_zcInfo.curFrame = 0;
	m_doingZoomCamera = TRUE;
	m_zcInfo.startZoom = m_zoom;
	m_zcInfo.endZoom = finalZoom;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DView::pitchCamera( Real finalPitch, Int milliseconds )
{
	if (milliseconds<1) milliseconds = 1;
	m_pcInfo.numFrames = milliseconds/TheW3DFrameLengthInMsec;
	if (m_pcInfo.numFrames < 1) {
		m_pcInfo.numFrames = 1;
	}
	m_pcInfo.curFrame = 0;
	m_doingPitchCamera = TRUE;
	m_pcInfo.startPitch = m_FXPitch;
	m_pcInfo.endPitch = finalPitch;
}

//-------------------------------------------------------------------------------------------------
/** Sets the final zoom for a camera movement. */
//-------------------------------------------------------------------------------------------------
void W3DView::cameraModFinalZoom( Real finalZoom ) 
{

	if (m_doingRotateCamera) 
	{
		Real terrainHeightMax = getHeightAroundPos(m_pos.x, m_pos.y);
		Real maxHeight = (terrainHeightMax + m_maxHeightAboveGround);
		Real maxZoom = maxHeight / m_cameraOffset.z;

		zoomCamera( finalZoom*maxZoom, (m_rcInfo.numFrames + m_rcInfo.numHoldFrames - m_rcInfo.curFrame)*TheW3DFrameLengthInMsec );
	}
	if (m_doingMoveCameraOnWaypointPath) 
	{
		Coord3D pos = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
		Real terrainHeightMax = getHeightAroundPos(pos.x, pos.y);
		Real maxHeight = (terrainHeightMax + m_maxHeightAboveGround);
		Real maxZoom = maxHeight / m_cameraOffset.z;

		zoomCamera( finalZoom*maxZoom, m_mcwpInfo.totalTimeMilliseconds - m_mcwpInfo.elapsedTimeMilliseconds );
	}
}

//-------------------------------------------------------------------------------------------------
/** Sets the final zoom for a camera movement. */
//-------------------------------------------------------------------------------------------------
void W3DView::cameraModFreezeAngle(void) 
{
	if (m_doingRotateCamera) {
		m_rcInfo.angle = 0; // Silly, but consistent.
		m_rcInfo.targetObjectID = INVALID_ID;
	}
	if (m_doingMoveCameraOnWaypointPath) {
		Int i;
//		Real curDistance = 0;
		for (i=0; i<m_mcwpInfo.numWaypoints; i++) {
			m_mcwpInfo.cameraAngle[i+1] = m_mcwpInfo.cameraAngle[0];
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** Sets the look toward point for a camera movement. */
// ------------------------------------------------------------------------------------------------
void W3DView::cameraModLookToward(Coord3D *pLoc) 
{
	if (m_doingRotateCamera) {
		return; // Doesn't apply to rotate about a point.
	}
	if (m_doingMoveCameraOnWaypointPath) {
		Int i;
//		Real curDistance = 0;
		for (i=2; i<=m_mcwpInfo.numWaypoints; i++) {
			Coord3D start, mid, end;
			Real factor = 0.5;
			start = m_mcwpInfo.waypoints[i-1];
			start.x += m_mcwpInfo.waypoints[i].x;
			start.y += m_mcwpInfo.waypoints[i].y;
			start.x /= 2;
			start.y /= 2;
			mid = m_mcwpInfo.waypoints[i];
			end = m_mcwpInfo.waypoints[i];
			end.x += m_mcwpInfo.waypoints[i+1].x;
			end.y += m_mcwpInfo.waypoints[i+1].y;
			end.x /= 2;
			end.y /= 2;
			Coord3D result = start;
			result.x += factor*(end.x-start.x);
			result.y += factor*(end.y-start.y);
			result.x += (1-factor)*factor*(mid.x-end.x + mid.x-start.x);
			result.y += (1-factor)*factor*(mid.y-end.y + mid.y-start.y);
			result.z = 0;
			Vector3 dir(pLoc->x-result.x, pLoc->y-result.y, 0);
			if (dir.Length()<0.1) continue;
			dir.Normalize();
			Real angle = WWMath::Acos(dir.X);
			if (dir.Y<0) {
				angle = -angle;
			}
			// Default camera is rotated 90 degrees, so match.
			angle -= PI/2;
			normAngle(angle);
			m_mcwpInfo.cameraAngle[i] = angle;
		}
		if (m_mcwpInfo.totalTimeMilliseconds==1) {
			// do it instantly.
			moveAlongWaypointPath(1);
			m_doingMoveCameraOnWaypointPath = true;
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** Sets the look toward point for the end of a camera movement. */
// ------------------------------------------------------------------------------------------------
void W3DView::cameraModFinalMoveTo(Coord3D *pLoc) 
{
	if (m_doingRotateCamera) {
		return; // Doesn't apply to rotate about a point.
	}
	if (m_doingMoveCameraOnWaypointPath) {
		Int i;
		Coord3D start, delta;
		start = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
		delta.x = pLoc->x - start.x;
		delta.y = pLoc->y - start.y;
		for (i=2; i<=m_mcwpInfo.numWaypoints; i++) {
			Coord3D result = m_mcwpInfo.waypoints[i];
			result.x += delta.x;
			result.y += delta.y;
			m_mcwpInfo.waypoints[i] = result;
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** Sets the look toward point for the end of a camera movement. */
// ------------------------------------------------------------------------------------------------
void W3DView::cameraModFinalLookToward(Coord3D *pLoc) 
{
	if (m_doingRotateCamera) {
		return; // Doesn't apply to rotate about a point.
	}
	if (m_doingMoveCameraOnWaypointPath) {
		Int i;
		Int min = m_mcwpInfo.numWaypoints-1;
		if (min<2) min=2;
//		Real curDistance = 0;
		for (i=min; i<=m_mcwpInfo.numWaypoints; i++) {
			Coord3D start, mid, end;
			Real factor = 0.5;
			start = m_mcwpInfo.waypoints[i-1];
			start.x += m_mcwpInfo.waypoints[i].x;
			start.y += m_mcwpInfo.waypoints[i].y;
			start.x /= 2;
			start.y /= 2;
			mid = m_mcwpInfo.waypoints[i];
			end = m_mcwpInfo.waypoints[i];
			end.x += m_mcwpInfo.waypoints[i+1].x;
			end.y += m_mcwpInfo.waypoints[i+1].y;
			end.x /= 2;
			end.y /= 2;
			Coord3D result = start;
			result.x += factor*(end.x-start.x);
			result.y += factor*(end.y-start.y);
			result.x += (1-factor)*factor*(mid.x-end.x + mid.x-start.x);
			result.y += (1-factor)*factor*(mid.y-end.y + mid.y-start.y);
			result.z = 0;
			Vector3 dir(pLoc->x-result.x, pLoc->y-result.y, 0);
			if (dir.Length()<0.1) continue;
			dir.Normalize();
			Real angle = WWMath::Acos(dir.X);
			if (dir.Y<0) {
				angle = -angle;
			}
			// Default camera is rotated 90 degrees, so match.
			angle -= PI/2;
			normAngle(angle);
			if (i==m_mcwpInfo.numWaypoints) { 
				m_mcwpInfo.cameraAngle[i] = angle;
			} else {
				Real deltaAngle = angle - m_mcwpInfo.cameraAngle[i];
				normAngle(deltaAngle);
				angle = m_mcwpInfo.cameraAngle[i] + deltaAngle/2;
				normAngle(angle);
				m_mcwpInfo.cameraAngle[i] = angle;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** Sets the final time multiplier for a camera movement. */
// ------------------------------------------------------------------------------------------------
void W3DView::cameraModFinalTimeMultiplier(Int finalMultiplier) 
{
	if (m_doingZoomCamera)
		m_zcInfo.endTimeMultiplier = finalMultiplier;
	if (m_doingPitchCamera)
		m_pcInfo.endTimeMultiplier = finalMultiplier;
	if (m_doingRotateCamera) {
		m_rcInfo.endTimeMultiplier = finalMultiplier;
	} else if (m_doingMoveCameraOnWaypointPath) {
		Int i;
		Real curDistance = 0;
		for (i=0; i<m_mcwpInfo.numWaypoints; i++) {
			curDistance += m_mcwpInfo.waySegLength[i];
			Real factor2 = curDistance / m_mcwpInfo.totalDistance;
			Real factor1 = 1.0-factor2;
			m_mcwpInfo.timeMultiplier[i+1] = REAL_TO_INT_FLOOR(0.5+m_mcwpInfo.timeMultiplier[i+1]*factor1 + finalMultiplier*factor2);
		}
	} else {
		// If we aren't doing a camera movement, just set the time.
		m_timeMultiplier = finalMultiplier;
	}
}

// ------------------------------------------------------------------------------------------------
/** Sets the number of frames to average motion for a camera movement */
// ------------------------------------------------------------------------------------------------
void W3DView::cameraModRollingAverage(Int framesToAverage) 
{
	if (framesToAverage < 1) framesToAverage = 1;
	m_mcwpInfo.rollingAverageFrames = framesToAverage;
}
 
// ------------------------------------------------------------------------------------------------
/** Sets the final pitch for a camera movement. */
// ------------------------------------------------------------------------------------------------
void W3DView::cameraModFinalPitch(Real finalPitch) {
	if (m_doingRotateCamera) {
		pitchCamera( finalPitch, ((m_rcInfo.numFrames + m_rcInfo.numHoldFrames) - m_rcInfo.curFrame) * TheW3DFrameLengthInMsec );
	}
	if (m_doingMoveCameraOnWaypointPath) {
		pitchCamera( finalPitch, m_mcwpInfo.totalTimeMilliseconds - m_mcwpInfo.elapsedTimeMilliseconds );
	}
}

// ------------------------------------------------------------------------------------------------
/** Move camera to a waypoint, resetting the default angle, pitch & zoom along the way.. */
// ------------------------------------------------------------------------------------------------
void W3DView::resetCamera(const Coord3D *location, Int milliseconds)
{
	moveCameraTo(location, milliseconds, 0, false);
	m_mcwpInfo.cameraAngle[2] = 0.0; // default angle.
	m_angle = m_mcwpInfo.cameraAngle[0];
	m_mcwpInfo.cameraFXPitch[2] = 1.0;

	// terrain height + desired height offset == cameraOffset * actual zoom
	// find best approximation of max terrain height we can see
	//Real terrainHeightMax = getHeightAroundPos(m_pos.x, m_pos.y);
	Real terrainHeightMax = getHeightAroundPos(location->x, location->y);

	Real desiredHeight = (terrainHeightMax + m_maxHeightAboveGround);
	Real desiredZoom = desiredHeight / m_cameraOffset.z;

	m_mcwpInfo.cameraZoom[2] = desiredZoom;//m_maxZoom;

	//Real terrainHeightMax = getHeightAroundPos(location->x, location->y);
	//Real desiredHeight = (terrainHeightMax + m_maxHeightAboveGround);
	//Real desiredZoom = desiredHeight / m_cameraOffset.z;
	zoomCamera( desiredZoom, milliseconds );	// this isn't right... or is it?

	pitchCamera( 1.0, milliseconds );
	DEBUG_LOG(("W3DView::resetCamera() Current zoom: %g  Desired zoom: %g  Current pitch: %g  Desired pitch: %g\n",
		m_zoom, desiredZoom, m_pitchAngle, m_defaultPitchAngle));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool W3DView::isCameraMovementFinished(void)
{
	if (m_viewFilter == FT_VIEW_MOTION_BLUR_FILTER) {
		// Several of the motion blur effects are similar to camera movements.
		if (m_viewFilterMode == FM_VIEW_MB_IN_AND_OUT_ALPHA ||
				m_viewFilterMode == FM_VIEW_MB_IN_AND_OUT_SATURATE ||
				m_viewFilterMode == FM_VIEW_MB_IN_ALPHA ||
				m_viewFilterMode == FM_VIEW_MB_OUT_ALPHA ||
				m_viewFilterMode == FM_VIEW_MB_IN_SATURATE ||
				m_viewFilterMode == FM_VIEW_MB_OUT_SATURATE ) {
			return true;
		}
	}
	return !m_doingMoveCameraOnWaypointPath && !m_doingRotateCamera && !m_doingPitchCamera && !m_doingZoomCamera;
}

// ------------------------------------------------------------------------------------------------
/** Move camera along a waypoint path in an interesting fashion.  Sets up parameters that get
 * evaluated in draw(). */
 // ------------------------------------------------------------------------------------------------
void W3DView::moveCameraAlongWaypointPath(Waypoint *pWay, Int milliseconds, Int shutter, Bool orient)
{
	const Real MIN_DELTA = MAP_XY_FACTOR;

	m_mcwpInfo.waypoints[0] = *getPosition();	
	m_mcwpInfo.cameraAngle[0] = getAngle();	
	m_mcwpInfo.waySegLength[0] = 0;	
	m_mcwpInfo.waypoints[1] = *getPosition();	
	m_mcwpInfo.numWaypoints = 1;
	if (milliseconds<1) milliseconds = 1;
	m_mcwpInfo.totalTimeMilliseconds = milliseconds;
	m_mcwpInfo.shutter = shutter/TheW3DFrameLengthInMsec;
	if (m_mcwpInfo.shutter<1) m_mcwpInfo.shutter = 1;

	while (pWay && m_mcwpInfo.numWaypoints <MAX_WAYPOINTS) {
		m_mcwpInfo.numWaypoints++;
		m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints] = *pWay->getLocation();
		if (pWay->getNumLinks()>0) {
			pWay = pWay->getLink(0);
		} else {
			pWay = NULL;
		}	
		Vector3 dir(m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints].x-m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints-1].x, m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints].y-m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints-1].y, 0);
		if (dir.Length()<MIN_DELTA) {
			if (pWay) {
				m_mcwpInfo.numWaypoints--; // drop this one.
			} else {
				m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints-1] = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
				m_mcwpInfo.numWaypoints--; // Push this one back.
			}
		}
	}
	setupWaypointPath(orient);
}

// ------------------------------------------------------------------------------------------------
/** Calculates angles and distances for moving along a waypoint path.  Sets up parameters that get
 * evaluated in draw(). */
// ------------------------------------------------------------------------------------------------
void W3DView::setupWaypointPath(Bool orient)
{
	m_mcwpInfo.curSegment = 1;
	m_mcwpInfo.curSegDistance = 0;
	m_mcwpInfo.totalDistance = 0;
	m_mcwpInfo.rollingAverageFrames = 1;
	Int i;
	for (i=1; i<m_mcwpInfo.numWaypoints; i++) {
		Vector3 dir(m_mcwpInfo.waypoints[i+1].x-m_mcwpInfo.waypoints[i].x, m_mcwpInfo.waypoints[i+1].y-m_mcwpInfo.waypoints[i].y, 0);
		m_mcwpInfo.waySegLength[i] = dir.Length();
		m_mcwpInfo.totalDistance += m_mcwpInfo.waySegLength[i];
		dir.Normalize();
		Real angle = getAngle();
		if (orient) {
			angle = WWMath::Acos(dir.X);
			if (dir.Y<0) {
				angle = -angle;
			}

			// Default camera is rotated 90 degrees, so match.
			angle -= PI/2;
			normAngle(angle);
		}
		//DEBUG_LOG(("Original Index %d, angle %.2f\n", i, angle*180/PI));
		m_mcwpInfo.cameraAngle[i] = angle;
	}
	m_mcwpInfo.cameraAngle[1] = getAngle();	
	m_mcwpInfo.cameraAngle[m_mcwpInfo.numWaypoints] = m_mcwpInfo.cameraAngle[m_mcwpInfo.numWaypoints-1];	
	for (i=m_mcwpInfo.numWaypoints-1; i>1; i--) {
		m_mcwpInfo.cameraAngle[i] = (m_mcwpInfo.cameraAngle[i] + m_mcwpInfo.cameraAngle[i-1]) / 2;  
	}
	m_mcwpInfo.waySegLength[m_mcwpInfo.numWaypoints+1] = m_mcwpInfo.waySegLength[m_mcwpInfo.numWaypoints+1];	
	if (m_mcwpInfo.totalDistance<1.0) {
		m_mcwpInfo.waySegLength[m_mcwpInfo.numWaypoints-1] += 1.0-m_mcwpInfo.totalDistance;
		m_mcwpInfo.totalDistance = 1.0;
	}
	Real curDistance = 0;
	Coord3D finalPos = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
	Real newGround = TheTerrainLogic->getGroundHeight(finalPos.x, finalPos.y);
	for (i=0; i<=m_mcwpInfo.numWaypoints+1; i++) {
		Real factor2 = curDistance / m_mcwpInfo.totalDistance;
		Real factor1 = 1.0-factor2;
		m_mcwpInfo.cameraFXPitch[i] = m_FXPitch;
		m_mcwpInfo.cameraZoom[i] = m_zoom;
		m_mcwpInfo.timeMultiplier[i] = m_timeMultiplier;
		m_mcwpInfo.groundHeight[i] = m_groundLevel*factor1 + newGround*factor2;
		curDistance += m_mcwpInfo.waySegLength[i];
		//DEBUG_LOG(("New Index %d, angle %.2f\n", i, m_mcwpInfo.cameraAngle[i]*180/PI));
	}

	// Pad the end.
	m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints+1] = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
	Coord3D cur = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
	Coord3D prev = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints-1];
	m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints+1].x += cur.x-prev.x;
	m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints+1].y += cur.y-prev.y;
	m_mcwpInfo.cameraAngle[m_mcwpInfo.numWaypoints+1] = m_mcwpInfo.cameraAngle[m_mcwpInfo.numWaypoints];	
	m_mcwpInfo.groundHeight[m_mcwpInfo.numWaypoints+1] = newGround;	


	cur = m_mcwpInfo.waypoints[2];
	prev = m_mcwpInfo.waypoints[1];
	m_mcwpInfo.waypoints[0].x -= cur.x-prev.x;
	m_mcwpInfo.waypoints[0].y -= cur.y-prev.y;

	m_doingMoveCameraOnWaypointPath = m_mcwpInfo.numWaypoints>1;
	m_doingRotateCamera = false;

	m_mcwpInfo.elapsedTimeMilliseconds = 0;
	m_mcwpInfo.curShutter = m_mcwpInfo.shutter;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static Real makeQuadraticS(Real t) 
{
	// for t = linear 0-1, convert to quadratic s where 0==0, 0.5==0.5 && 1.0 == 1.0.
	Real tPrime = t;
	if (t<0.5) {
		tPrime = 0.5 * (2*t*2*t);
	} else {
		tPrime = (t-0.5)*2;
		tPrime = WWMath::Sqrt(tPrime);
		tPrime = 0.5 + 0.5*(tPrime);
	}
	return tPrime*0.5 + t*0.5;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::rotateCameraOneFrame(void)
{
	m_rcInfo.curFrame++;
	if (TheGlobalData->m_disableCameraMovement) {
		if (m_rcInfo.curFrame >= m_rcInfo.numFrames + m_rcInfo.numHoldFrames) {
			m_doingRotateCamera = false;
			m_freezeTimeForCameraMovement = false;
		}
		return;
	}

	if (m_rcInfo.curFrame <= m_rcInfo.numFrames)
	{
		// not just holding; do the camera adjustment
		Real factor = ((Real)m_rcInfo.curFrame)/m_rcInfo.numFrames;
		if (m_rcInfo.trackObject)
		{
			const Object *obj = TheGameLogic->findObjectByID(m_rcInfo.targetObjectID);
			if (obj)
			{
				// object has not been destroyed
				m_rcInfo.targetObjectPos = *obj->getPosition();
			}

			Vector3 dir(m_rcInfo.targetObjectPos.x - m_pos.x, m_rcInfo.targetObjectPos.y - m_pos.y, 0);
			if (dir.Length()>=0.1)
			{
				dir.Normalize();
				Real angle = WWMath::Acos(dir.X);
				if (dir.Y<0) {
					angle = -angle;
				}
				// Default camera is rotated 90 degrees, so match.
				angle -= PI/2;
				normAngle(angle);

				factor = 1.0f / (m_rcInfo.numFrames - m_rcInfo.curFrame + 1);
				Real angleDiff = (angle - m_angle);
				normAngle(angleDiff);
				angleDiff *= factor;

				m_angle += angleDiff;
			}
		}
		else
		{
			m_angle += m_rcInfo.angle;
		}
		//m_zoom = m_rcInfo.startZoom + (m_rcInfo.endZoom-m_rcInfo.startZoom)*factor;
		//m_FXPitch = m_rcInfo.startPitch + (m_rcInfo.endPitch-m_rcInfo.startPitch)*factor;
		normAngle(m_angle);
		//DEBUG_LOG(("\tm_angle:%g\n", m_angle));
		m_timeMultiplier = m_rcInfo.startTimeMultiplier + REAL_TO_INT_FLOOR(0.5 + (m_rcInfo.endTimeMultiplier-m_rcInfo.startTimeMultiplier)*factor);
	}
	else if (m_rcInfo.curFrame <= m_rcInfo.numFrames + m_rcInfo.numHoldFrames && m_rcInfo.trackObject)
	{
		const Object *obj = TheGameLogic->findObjectByID(m_rcInfo.targetObjectID);
		if (obj)
		{
			// object has not been destroyed
			m_rcInfo.targetObjectPos = *obj->getPosition();
		}

		Vector3 dir(m_rcInfo.targetObjectPos.x - m_pos.x, m_rcInfo.targetObjectPos.y - m_pos.y, 0);
		if (dir.Length()>=0.1)
		{
			dir.Normalize();
			Real angle = WWMath::Acos(dir.X);
			if (dir.Y<0) {
				angle = -angle;
			}
			// Default camera is rotated 90 degrees, so match.
			angle -= PI/2;
			normAngle(angle);
			m_angle = angle;
		}
	}


	if (m_rcInfo.curFrame >= m_rcInfo.numFrames + m_rcInfo.numHoldFrames) {
		m_doingRotateCamera = false;
		m_freezeTimeForCameraMovement = false;
		//m_zoom = m_rcInfo.endZoom;
		//m_FXPitch = m_rcInfo.endPitch;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::zoomCameraOneFrame(void)
{
	m_zcInfo.curFrame++;
	if (TheGlobalData->m_disableCameraMovement) {
		if (m_zcInfo.curFrame >= m_zcInfo.numFrames) {
			m_doingZoomCamera = false;
		}
		return;
	}
	if (m_zcInfo.curFrame <= m_zcInfo.numFrames)
	{
		// not just holding; do the camera adjustment
		Real factor = ((Real)m_zcInfo.curFrame)/m_zcInfo.numFrames;
		m_zoom = m_zcInfo.startZoom + (m_zcInfo.endZoom-m_zcInfo.startZoom)*factor;
	}

	if (m_zcInfo.curFrame >= m_zcInfo.numFrames) {
		m_doingZoomCamera = false;
		m_zoom = m_zcInfo.endZoom;
	}

	//DEBUG_LOG(("W3DView::zoomCameraOneFrame() - m_zoom = %g\n", m_zoom));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::pitchCameraOneFrame(void)
{
	m_pcInfo.curFrame++;
	if (TheGlobalData->m_disableCameraMovement) {
		if (m_pcInfo.curFrame >= m_pcInfo.numFrames) {
			m_doingPitchCamera = false;
		}
		return;
	}
	if (m_pcInfo.curFrame <= m_pcInfo.numFrames)
	{
		// not just holding; do the camera adjustment
		Real factor = ((Real)m_pcInfo.curFrame)/m_pcInfo.numFrames;
		m_FXPitch = m_pcInfo.startPitch + (m_pcInfo.endPitch-m_pcInfo.startPitch)*factor;
	}

	if (m_pcInfo.curFrame >= m_pcInfo.numFrames) {
		m_doingPitchCamera = false;
		m_FXPitch = m_pcInfo.endPitch;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DView::moveAlongWaypointPath(Int milliseconds)
{
	m_mcwpInfo.elapsedTimeMilliseconds += milliseconds;
	if (TheGlobalData->m_disableCameraMovement) {
		if (m_mcwpInfo.elapsedTimeMilliseconds>m_mcwpInfo.totalTimeMilliseconds) {
			m_doingMoveCameraOnWaypointPath = false;
			m_freezeTimeForCameraMovement = false;
		}
		return;
	}
	if (m_mcwpInfo.elapsedTimeMilliseconds>m_mcwpInfo.totalTimeMilliseconds) {
		m_doingMoveCameraOnWaypointPath = false;
		m_freezeTimeForCameraMovement = false;
		m_angle = m_mcwpInfo.cameraAngle[m_mcwpInfo.numWaypoints];
		//m_zoom = m_mcwpInfo.cameraZoom[m_mcwpInfo.numWaypoints];
		//m_FXPitch = m_mcwpInfo.cameraFXPitch[m_mcwpInfo.numWaypoints];

		m_groundLevel = m_mcwpInfo.groundHeight[m_mcwpInfo.numWaypoints];
		/////////////////////m_cameraOffset.z = m_groundLevel+TheGlobalData->m_cameraHeight;
		m_cameraOffset.y = -(m_cameraOffset.z / tan(TheGlobalData->m_cameraPitch * (PI / 180.0)));
		m_cameraOffset.x = -(m_cameraOffset.y * tan(TheGlobalData->m_cameraYaw * (PI / 180.0)));

		Coord3D pos = m_mcwpInfo.waypoints[m_mcwpInfo.numWaypoints];
		pos.z = 0;
		setPosition(&pos);
		// Note - assuming that the scripter knows what he is doing, we adjust the constraints so that
		// the scripted action can occur.
		m_cameraConstraint.lo.x = minf(m_cameraConstraint.lo.x, pos.x);
		m_cameraConstraint.hi.x = maxf(m_cameraConstraint.hi.x, pos.x);
		m_cameraConstraint.lo.y = minf(m_cameraConstraint.lo.y, pos.y);
		m_cameraConstraint.hi.y = maxf(m_cameraConstraint.hi.y, pos.y);
		return;
	}
	Int deltaTime = milliseconds;

	Real distance = m_mcwpInfo.totalDistance * deltaTime / m_mcwpInfo.totalTimeMilliseconds;
	m_mcwpInfo.curSegDistance += distance;
	while (m_mcwpInfo.curSegDistance > m_mcwpInfo.waySegLength[m_mcwpInfo.curSegment]) {
		m_mcwpInfo.curSegDistance -= m_mcwpInfo.waySegLength[m_mcwpInfo.curSegment];
		m_mcwpInfo.curSegment++;
		if (m_mcwpInfo.curSegment >= m_mcwpInfo.numWaypoints) { 
			m_mcwpInfo.totalTimeMilliseconds = 0; // Will end following next frame.
			return;
		}
	}
	Real avgFactor = 1.0/m_mcwpInfo.rollingAverageFrames;
	m_mcwpInfo.curShutter--;
	if (m_mcwpInfo.curShutter>0) {
		return;
	}
	m_mcwpInfo.curShutter = m_mcwpInfo.shutter;
	Real factor = m_mcwpInfo.curSegDistance / m_mcwpInfo.waySegLength[m_mcwpInfo.curSegment];
	if (m_mcwpInfo.curSegment == m_mcwpInfo.numWaypoints-1) {
		avgFactor = avgFactor + (1.0-avgFactor)*factor;
	}
	Real angle = getAngle();
	Real factor1;
	Real factor2;
	factor1 = 1.0-factor;
	//factor1 = makeQuadraticS(factor1);
	factor2 = 1.0-factor1;
	Real angle1 = m_mcwpInfo.cameraAngle[m_mcwpInfo.curSegment];
	Real angle2 = m_mcwpInfo.cameraAngle[m_mcwpInfo.curSegment+1];
	if (angle2-angle1 > PI) angle1 += 2*PI;
	if (angle2-angle1 < -PI) angle1 -= 2*PI;
	angle = angle1 * (factor1) + angle2 * (factor2); 

	normAngle(angle);
	Real deltaAngle = angle-m_angle;
	normAngle(deltaAngle);
	if (fabs(deltaAngle) > PI/10) {
		DEBUG_LOG(("Huh.\n"));
	}
	m_angle += avgFactor*(deltaAngle);
	normAngle(m_angle);

	/*
	Real pitchFX = m_mcwpInfo.cameraFXPitch[m_mcwpInfo.curSegment]*factor1 + 
			m_mcwpInfo.cameraFXPitch[m_mcwpInfo.curSegment+1]*factor2;
	m_FXPitch += avgFactor*(pitchFX-m_FXPitch);

	Real cameraZoom = m_mcwpInfo.cameraZoom[m_mcwpInfo.curSegment]*factor1 + 
			m_mcwpInfo.cameraZoom[m_mcwpInfo.curSegment+1]*factor2;
	m_zoom += avgFactor*(cameraZoom-m_zoom);
	*/

	Real timeMultiplier = m_mcwpInfo.timeMultiplier[m_mcwpInfo.curSegment]*factor1 + 
			m_mcwpInfo.timeMultiplier[m_mcwpInfo.curSegment+1]*factor2;
	m_timeMultiplier = REAL_TO_INT_FLOOR(0.5 + timeMultiplier);

	m_groundLevel = m_mcwpInfo.groundHeight[m_mcwpInfo.curSegment]*factor1 + 
			m_mcwpInfo.groundHeight[m_mcwpInfo.curSegment+1]*factor2;
	//////////////m_cameraOffset.z = m_groundLevel+TheGlobalData->m_cameraHeight;
	m_cameraOffset.y = -(m_cameraOffset.z / tan(TheGlobalData->m_cameraPitch * (PI / 180.0)));
	m_cameraOffset.x = -(m_cameraOffset.y * tan(TheGlobalData->m_cameraYaw * (PI / 180.0)));

	Coord3D start, mid, end;
	if (factor<0.5) {
		start = m_mcwpInfo.waypoints[m_mcwpInfo.curSegment-1];
		start.x += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment].x;
		start.y += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment].y;
		start.x /= 2;
		start.y /= 2;
		mid = m_mcwpInfo.waypoints[m_mcwpInfo.curSegment];
		end = m_mcwpInfo.waypoints[m_mcwpInfo.curSegment];
		end.x += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+1].x;
		end.y += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+1].y;
		end.x /= 2;
		end.y /= 2;
		factor += 0.5;
	} else {
		start = m_mcwpInfo.waypoints[m_mcwpInfo.curSegment];
		start.x += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+1].x;
		start.y += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+1].y;
		start.x /= 2;
		start.y /= 2;
		mid = m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+1];
		end = m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+1];
		end.x += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+2].x;
		end.y += m_mcwpInfo.waypoints[m_mcwpInfo.curSegment+2].y;
		end.x /= 2;
		end.y /= 2;
		factor -= 0.5;
	}

	Coord3D result = start;
	result.x += factor*(end.x-start.x);
	result.y += factor*(end.y-start.y);
	result.x += (1-factor)*factor*(mid.x-end.x + mid.x-start.x);
	result.y += (1-factor)*factor*(mid.y-end.y + mid.y-start.y);
	result.z = 0;
/*
	static Real prevGround = 0;
	DEBUG_LOG(("Dx %.2f, dy %.2f, DeltaANgle = %.2f, %.2f DeltaGround %.2f\n", m_pos.x-result.x, m_pos.y-result.y, deltaAngle, m_groundLevel, m_groundLevel-prevGround));
	prevGround = m_groundLevel;
*/
	setPosition(&result);
	// Note - assuming that the scripter knows what he is doing, we adjust the constraints so that
	// the scripted action can occur.
	m_cameraConstraint.lo.x = minf(m_cameraConstraint.lo.x, result.x);
	m_cameraConstraint.hi.x = maxf(m_cameraConstraint.hi.x, result.x);
	m_cameraConstraint.lo.y = minf(m_cameraConstraint.lo.y, result.y);
	m_cameraConstraint.hi.y = maxf(m_cameraConstraint.hi.y, result.y);

}


// ------------------------------------------------------------------------------------------------
/** Add an impulse force to shake the camera.
 * The camera shake is a simple simulation of an oscillating spring/damper.
 * The idea is that some sort of shock has "pushed" the camera once, as an
 * impluse, after which the camera vibrates back to its rest position.
 * @todo This should be part of "View", not "W3DView". */
// ------------------------------------------------------------------------------------------------
void W3DView::shake( const Coord3D *epicenter, CameraShakeType shakeType )
{
	Real angle = GameClientRandomValueReal( 0, 2*PI );

	m_shakeAngleCos = (Real)cos( angle );
	m_shakeAngleSin = (Real)sin( angle );

	Real intensity = 0.0f;
	switch( shakeType )
	{
		case SHAKE_SUBTLE:
			intensity = TheGlobalData->m_shakeSubtleIntensity;
			break;

		case SHAKE_NORMAL:
			intensity = TheGlobalData->m_shakeNormalIntensity;
			break;

		case SHAKE_STRONG:
			intensity = TheGlobalData->m_shakeStrongIntensity;
			break;

		case SHAKE_SEVERE:
			intensity = TheGlobalData->m_shakeSevereIntensity;
			break;
		
		case SHAKE_CINE_EXTREME:
			intensity = TheGlobalData->m_shakeCineExtremeIntensity;
			break;

		case SHAKE_CINE_INSANE:
			intensity = TheGlobalData->m_shakeCineInsaneIntensity;
			break;
	}

	// intensity falls off with distance
	const Coord3D *viewPos = getPosition();
	Coord3D d;
	d.x = epicenter->x - viewPos->x;
	d.y = epicenter->y - viewPos->y;
	/// @todo make this 3D once we have the real "lookat" spot
	//d.z = epicenter->z - viewPos->z;

	Real dist = (Real)sqrt( d.x*d.x + d.y*d.y );

	if (dist > TheGlobalData->m_maxShakeRange)
		return;

	intensity *= 1.0f - (dist/TheGlobalData->m_maxShakeRange);

	// add intensity and clamp
	m_shakeIntensity += intensity;

	//const Real maxIntensity = 10.0f;
	const Real maxIntensity = 3.0f;
	if (m_shakeIntensity > TheGlobalData->m_maxShakeIntensity)
		m_shakeIntensity = maxIntensity;
}

//-------------------------------------------------------------------------------------------------
/** Transformt he screen pixel coord passed in, to a world coordinate at the specified
	* z value */
//-------------------------------------------------------------------------------------------------
void W3DView::screenToWorldAtZ( const ICoord2D *s, Coord3D *w, Real z )
{
	Vector3 rayStart, rayEnd;
	
	getPickRay( s, &rayStart, &rayEnd );
	w->x = Vector3::Find_X_At_Z( z, rayStart, rayEnd );
	w->y = Vector3::Find_Y_At_Z( z, rayStart, rayEnd );
	w->z = z;

}  // end screenToWorldAtZ

