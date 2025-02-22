/*
	Simbicon 1.5 Controller Editor Framework, 
	Copyright 2009 Stelian Coros, Philippe Beaudoin and Michiel van de Panne.
	All rights reserved. Web: www.cs.ubc.ca/~van/simbicon_cef

	This file is part of the Simbicon 1.5 Controller Editor Framework.

	Simbicon 1.5 Controller Editor Framework is free software: you can 
	redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Simbicon 1.5 Controller Editor Framework is distributed in the hope 
	that it will be useful, but WITHOUT ANY WARRANTY; without even the 
	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Simbicon 1.5 Controller Editor Framework. 
	If not, see <http://www.gnu.org/licenses/>.
*/

#include "ControllerEditor.h"
#include "Globals.h"


/**
 * Constructor.
 */
ControllerEditor::ControllerEditor(void){
	tprintf("Loading Controller Editor...\n");
	strcpy(inputFile,  "init/input.conF");

	loadFramework();

	Globals::changeCurrControlShotStr( -1 );
	conF->getState(&conState);

	nextControlShot = 0;
	maxControlShot = -1;

	registerTclFunctions();
}


/**
 * Destructor.
 */
ControllerEditor::~ControllerEditor(void){

	nextControlShot = 0;
	maxControlShot = -1;

	clearEditedCurves();
}

/**
 * This method is used to create a physical world and load the objects in it.
 */
void ControllerEditor::loadFramework(){
	loadFramework( -1 );
}


/**
 * This method is used to create a physical world and load the objects in it.
 */
void ControllerEditor::loadFramework( int controlShot ){
	this->world = NULL;
	//create a new world, and load some bodies
	try{
		if( controlShot < 0 ) {
			conF = new SimBiConFramework(inputFile, NULL);
		}
		else {
			char conFile[256];
			sprintf(conFile, "..\\controlShots\\cs%05d.sbc", controlShot);
			conF = new SimBiConFramework(inputFile, conFile);
		}
	
		avgSpeed = 0;
		timesVelSampled = 0;

		Globals::changeCurrControlShotStr( controlShot );
		conF->getState(&conState);
		this->world = conF->getWorld();
	}catch(const char* msg){
		conF = NULL;
		tprintf("Error: %s\n", msg);
		logPrint("Error: %s\n", msg);
	}

}


/**
	This method draws the desired target that is to be tracked.
*/
void ControllerEditor::drawDesiredTarget(){
	Character* ch = conF->getCharacter();
	//we need to get the desired pose, and set it to the character
	DynamicArray<double> pose;
	conF->getController()->updateTrackingPose(pose, Globals::targetPosePhase);

	glPushMatrix();
	Point3d p = ch->getRoot()->getCMPosition();
	//this is where we will be drawing the target pose
	p.x += 2;
	p.y = 1.35;
	p.z += 0;

	worldState.clear();
	conF->getWorld()->getState(&worldState);
	
	pose[0] = 0;
	pose[1] = 0;
	pose[2] = 0;
	
	ch->setState(&pose);
	
	glTranslated(p.x, p.y, p.z);

	TransformationMatrix tmp;
	double val[16];
	conF->getCharacterFrame().getRotationMatrix(&tmp);
	tmp.getOGLValues(val);
	glMultMatrixd(val);

	float tempColor[] = {0.5, 0.5, 0.5, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tempColor);

	conF->getWorld()->drawRBs(SHOW_MESH);
	p = ch->getRoot()->getCMPosition();
	glDisable(GL_LIGHTING);
	glTranslated(p.x,p.y,p.z);
	GLUtils::drawAxes(0.2);
	glPopMatrix();
	glEnable(GL_LIGHTING);
	//set the state back to the original...
	conF->getWorld()->setState(&worldState);
}


/**
 * This method is called whenever the window gets redrawn.
 */
void ControllerEditor::draw(bool shadowMode){
	int flags = SHOW_MESH;
	//if we are drawing shadows, we don't need to enable textures or lighting, since we only want the projection anyway
	if (shadowMode == false){
		flags |= SHOW_COLOURS;

		glEnable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		if (Globals::drawCollisionPrimitives)
			flags |= SHOW_CD_PRIMITIVES | SHOW_FRICTION_PARTICLES;
		if (Globals::drawJoints){
			flags |= SHOW_JOINTS | SHOW_BODY_FRAME;
		}

		if (Globals::drawContactForces){
			//figure out if we should draw the contacts, desired pose, etc.
			glColor3d(1, 0, 0);
			DynamicArray<ContactPoint>* cfs = conF->getWorld()->getContactForces();
			for (uint i=0;i<cfs->size();i++){
				ContactPoint *c = &((*cfs)[i]);
				
				GLUtils::drawCylinder(0.01, c->f * 0.09, c->cp);
				GLUtils::drawCone(0.03, c->f * 0.01, c->cp+c->f*0.09);
			}
		}
	}
	else{
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
	}
	
	if (conF == NULL)
		return;

	AbstractRBEngine* rbc = conF->getWorld();

	if (rbc)
		rbc->drawRBs(flags);

	if (shadowMode == false){
		//draw the pose if desirable...
		if (Globals::drawDesiredPose && conF && conF->getWorld())
			drawDesiredTarget();
	}
}

/**
 * This method is used to draw extra stuff on the screen (such as items that need to be on the screen at all times)
 */
void ControllerEditor::drawExtras(){
	if (Globals::drawCurveEditor == 1) {
		for( uint i = 0; i < curveEditors.size(); ++i )
			curveEditors[i]->draw();
	}else
		InteractiveWorld::drawExtras();
}


/**
 * This method is used to restart the application.
 */
void ControllerEditor::restart(){
	conF->setState(conState);
	avgSpeed = 0;
	timesVelSampled = 0;
}


/**
 * This method is used to reload the application.
 */
void ControllerEditor::reload(){
	nextControlShot = 0;
	clearEditedCurves();

	delete conF;
	loadFramework();
}


/**
	This method is used to undo the last operation
*/
void ControllerEditor::undo(){
	if( nextControlShot <= 0 ) return;
	nextControlShot--;
	clearEditedCurves();
	delete conF;
	loadFramework(nextControlShot-1);	
}

/**
	This method is used to redo the last operation
*/
void ControllerEditor::redo(){
	if( nextControlShot >= maxControlShot+1 ) return;
	nextControlShot++;
	clearEditedCurves();
	delete conF;
	loadFramework(nextControlShot-1);	
}


void ControllerEditor::stepTaken() {


	if( Globals::updateDVTraj ) {
		
		if (conF) {
			SimBiConState *state = conF->getController()->states[ lastFSMState ];

			dTrajX.simplify_catmull_rom( 0.05 );
			dTrajZ.simplify_catmull_rom( 0.05 );
			vTrajX.simplify_catmull_rom( 0.05 );
			vTrajZ.simplify_catmull_rom( 0.05 );

			state->updateDVTrajectories(NULL, NULL, dTrajX, dTrajZ, vTrajX, vTrajZ );

			clearEditedCurves();
			addEditedCurve( state->dTrajX );
			addEditedCurve( state->dTrajZ );
			addEditedCurve( state->vTrajX );
			addEditedCurve( state->vTrajZ );
		}

	}

	if( Globals::drawControlShots ) {
		char stateFileName[100], fName[100];
		sprintf(stateFileName, "..\\controlShots\\cs%05d.rs", nextControlShot);
		conF->getCharacter()->saveReducedStateToFile(stateFileName);
		sprintf(fName, "..\\controlShots\\cs%05d.sbc", nextControlShot);
		conF->getController()->writeToFile(fName,stateFileName);
		Globals::changeCurrControlShotStr( nextControlShot );
		maxControlShot = nextControlShot;
		nextControlShot++;
		Globals::drawControlShots = false;
		Tcl_UpdateLinkedVar( Globals::tclInterpreter, "toggleControlshots" );
		conF->getState(&conState);
	}
}


/**
 *	This method is used when a mouse event gets generated. This method returns true if the message gets processed, false otherwise.
 */
bool ControllerEditor::onMouseEvent(int eventType, int button, int mouseX, int mouseY){

	//need to figure out if the mouse is in the curve editor (and if we care)...
	if ( Globals::drawCurveEditor == 1 ) {
		for( uint i = 0; i < curveEditors.size(); ++i )
			if( curveEditors[i]->onMouseEvent( eventType, button, mouseX, mouseY ) ) {
				return true;
			}
	}

	return InteractiveWorld::onMouseEvent(eventType, button, mouseX, mouseY);

}

/**
 * This method gets called when the application gets initialized. 
 */
void ControllerEditor::init(){
	InteractiveWorld::init();
}

/**
 * This method returns the target that the camera should be looking at
 */
Point3d ControllerEditor::getCameraTarget(){
	if (conF == NULL)
		return Point3d(0,1,0);
	Character* ch = conF->getCharacter();
	if (ch == NULL)
		return Point3d(0,1,0);
	//we need to get the character's position. We'll hack that a little...
	return Point3d(ch->getRoot()->getCMPosition().x, 1, ch->getRoot()->getCMPosition().z);
}

/**
* This method will get called on idle. This method should be responsible with doing the work that the application needs to do 
* (i.e. run simulations, and so on).
*/
void ControllerEditor::processTask(){
	double simulationTime = 0;
	double maxRunningTime = 0.98/Globals::desiredFrameRate;
	//if we still have time during this frame, or if we need to finish the physics step, do this until the simulation time reaches the desired value
	while (simulationTime/maxRunningTime < Globals::animationTimeToRealTimeRatio){
		simulationTime += SimGlobals::dt;
		double phi = conF->getController()->getPhase();
		lastFSMState = conF->getController()->getFSMState();
		double signChange = (conF->getController()->getStance() == RIGHT_STANCE)?-1:1;
		Globals::targetPosePhase = phi;
		Tcl_UpdateLinkedVar( Globals::tclInterpreter, "targetPosePhase" );

//		tprintf("d = %2.4lf, v = %2.4lf\n", conF->con->d.x, conF->con->v.x);

		avgSpeed += conF->getCharacter()->getHeading().getComplexConjugate().rotate(conF->getCharacter()->getRoot()->getCMVelocity()).z;
		timesVelSampled++;

		if (conF) {
			if( phi < dTrajX.getMaxPosition() ) {
				dTrajX.clear();
				dTrajZ.clear();
				vTrajX.clear();
				vTrajZ.clear();
			}
			Vector3d d = conF->getController()->d;
			Vector3d v = conF->getController()->v;
			dTrajX.addKnot( phi, d.x * signChange);
			dTrajZ.addKnot( phi, d.z  );
			vTrajX.addKnot( phi, v.x * signChange );
			vTrajZ.addKnot( phi, v.z  );					

			bool newStep = conF->advanceInTime(SimGlobals::dt);

			if( newStep ) {
				avgSpeed /= timesVelSampled;
				Vector3d v = conF->getLastStepTaken();
				tprintf("step: %lf %lf %lf (phi = %lf, speed = %lf)\n", v.x, v.y, v.z, phi, avgSpeed);
//				Globals::animationRunning = false;

				avgSpeed = 0;
				timesVelSampled = 0;

				stepTaken();

				//get the reversed new state...
				DynamicArray<double> newState;
				if (conF->getController()->getStance() == RIGHT_STANCE)
					conF->getCharacter()->getReverseStanceState(&newState);
				else
					conF->getCharacter()->getState(&newState);

				ReducedCharacterState rNew(&newState);

				conF->getCharacter()->saveReducedStateToFile("out\\reducedCharacterState.rs", newState);
			}

//			if (conF->getController()->isBodyInContactWithTheGround()){
//				tprintf("Lost control of the biped...\n");
//				Globals::animationRunning = false;
//				break;
//			}
//		break;
		}
	}
}




int trajectoryToEdit(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){

	// selectCurveToEdit stateIdx trajectoryIdx
	if( argc != 3 ) return TCL_ERROR;

	ControllerEditor* obj = (ControllerEditor*)clientData;

	int idx = atoi( argv[1] );
	SimBiConState* state = obj->getFramework()->getController()->getState( idx );
	dInitODE2(0);
	if( !state ) return  TCL_ERROR;
	idx = atoi( argv[2] );
	Trajectory* trajectory = state->getTrajectory( idx );
	if( !trajectory ) return  TCL_ERROR;

	obj->clearEditedCurves();
	for( uint i = 0; i < trajectory->components.size(); ++i ) {
		obj->addEditedCurve( &trajectory->components[i]->baseTraj );
	}
	if( trajectory->strengthTraj != NULL ) {
		obj->addEditedCurve( trajectory->strengthTraj );
	}

	return TCL_OK;

}



int updateTargetPose(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){
	ControllerEditor* obj = (ControllerEditor*)clientData;
	return TCL_OK;
}

// Following are wrappers for TCL functions that can access the object
int controllerUndo (ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){

	ControllerEditor* obj = (ControllerEditor*)clientData;
	obj->undo();
	return TCL_OK;
}

// Following are wrappers for TCL functions that can access the object
int controllerRedo (ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){

	ControllerEditor* obj = (ControllerEditor*)clientData;
	obj->redo();
	return TCL_OK;
}


// Following are wrappers for TCL functions that can access the object
int getStateNames (ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){

	ControllerEditor* obj = (ControllerEditor*)clientData;

	SimBiController* controller = obj->getFramework()->getController();

	DynamicArray<const char*> stateNames;
	uint i = 0;
	while( true ) {
		SimBiConState* state = controller->getState( i++ );
		if( !state ) break;
		stateNames.push_back( state->getDescription() );
	}	

	char* result = stringListToTclList( stateNames );
	Tcl_SetResult(interp, result, TCL_DYNAMIC);

	return TCL_OK;
}


int getTrajectoryNames (ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){

	// getComponentNames stateIdx 
	if( argc != 2 ) return TCL_ERROR;

	ControllerEditor* obj = (ControllerEditor*)clientData;

	int idx = atoi( argv[1] );
	SimBiConState* state = obj->getFramework()->getController()->getState( idx );

	if( !state ) return  TCL_ERROR;

	DynamicArray<const char*> trajectoryNames;
	for( int i = 0; i < state->getTrajectoryCount(); ++i ) {
		Trajectory* trajectory = state->getTrajectory( i );
		trajectoryNames.push_back( trajectory->jName );
	}	

	char* result = stringListToTclList( trajectoryNames );
	Tcl_SetResult(interp, result, TCL_DYNAMIC);

	return TCL_OK;

}

int getComponentNames (ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char **argv){

	// getComponentNames stateIdx trajectoryIdx
	if( argc != 3 ) return TCL_ERROR;

	ControllerEditor* obj = (ControllerEditor*)clientData;

	int idx = atoi( argv[1] );
	SimBiConState* state = obj->getFramework()->getController()->getState( idx );
	if( !state ) return  TCL_ERROR;
	idx = atoi( argv[2] );
	Trajectory* trajectory = state->getTrajectory( idx );
	if( !trajectory ) return  TCL_ERROR;

	DynamicArray<const char*> componentNames;
	for( uint i = 0; i < trajectory->components.size(); ++i ) {
		char* componentName = new char[ 32 ];
		sprintf( componentName, "Component %d", i );
		componentNames.push_back( componentName );
	}	

	char* result = stringListToTclList( componentNames );

	for( uint i = 0; i < componentNames.size(); ++i )
		delete[] componentNames[i];

	Tcl_SetResult(interp, result, TCL_DYNAMIC);
	return TCL_OK;

}


/**
 * Registers TCL functions specific to this application
 */
void ControllerEditor::registerTclFunctions() {	

	Application::registerTclFunctions();

	Tcl_CreateCommand(Globals::tclInterpreter, "getStateNames", getStateNames, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateCommand(Globals::tclInterpreter, "getTrajectoryNames", getTrajectoryNames, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateCommand(Globals::tclInterpreter, "getComponentNames", getComponentNames, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateCommand(Globals::tclInterpreter, "trajectoryToEdit", trajectoryToEdit, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateCommand(Globals::tclInterpreter, "updateTargetPose", updateTargetPose, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateCommand(Globals::tclInterpreter, "controllerUndo", controllerUndo, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

	Tcl_CreateCommand(Globals::tclInterpreter, "controllerRedo", controllerRedo, (ClientData)this, (Tcl_CmdDeleteProc *) NULL);

}

/**
 * This method is to be implemented by classes extending this one. The output of this function is a point that
 * represents the world-coordinate position of the dodge ball, when the position in the throw interface is (x, y).
 */
void ControllerEditor::getDodgeBallPosAndVel(double x, double y, double strength, Point3d* pos, Vector3d* vel){
	vel->x = x;
	vel->y = 0;
	vel->z = y;

	*vel = conF->getCharacter()->getHeading().rotate(*vel) * 20;
	*pos = conF->getCharacter()->getRoot()->getCMPosition();
	*pos = *pos + conF->getCharacter()->getRoot()->getCMVelocity() * 0.5;
	pos->y +=1;
	*pos = *pos + vel->unit() * (-2);
}