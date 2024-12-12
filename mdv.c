/***********************************************************************
  Program atomv.c--ball representation of atoms.
  Required files
    atomv.h:   Include file
    md.conf:   MD configuration file containing atomic coordinates
***********************************************************************/
#include "mdv.h"
#include <stdio.h>
#include <math.h>
#include <OpenGL/gl.h>    /* Header file for the OpenGL library */
#include <OpenGL/glu.h>   /* Header file for the GLu library */
#include <GLUT/glut.h>    /* Header file for the GLut library */

GLuint sphereid;          /* display-list id of atom sphere geom */
GLuint atomsid;           /* display-list id of all atoms */
GLdouble fovy, aspect, near_clip, far_clip;  
                          /* parameters for gluPerspective() */
FILE *fp;                 /* pointer to open an MD-configuration file */

/* Function prototypes ************************************************/
void reshape(int, int);
void makeFastNiceSphere(GLuint, double);
void makeAtoms(void);
void makeCurframeGeom(void);
void drawScene(void);
void display(void);
void initView(float *, float *);
void readConf(void);

/**********************************************************************/
void reshape (int w, int h) {
/***********************************************************************
  Callback for glutReshapeFunc()
***********************************************************************/
  /* set the GL viewport to match the full size of the window */
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  aspect = w/(float)h;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(fovy,aspect,near_clip,far_clip);
  glMatrixMode(GL_MODELVIEW);
}

/**********************************************************************/
void makeFastNiceSphere(GLuint listid, double radius) {
/***********************************************************************
Called once to generate and compile sphere geometry into the given
display list id.
***********************************************************************/
  int i,j;
  float lon,lat;
  float loninc,latinc;
  float x,y,z;

  loninc = 2*M_PI/nlon;
  latinc = M_PI/nlat;

  glNewList(listid,GL_COMPILE);

    /* South-pole triangular fan */
    glBegin(GL_TRIANGLE_FAN);
      glNormal3f(0,-1,0);
      glVertex3f(0,-radius,0);
      lon = 0;
      lat = -M_PI/2 + latinc;
      y = sin(lat);
      for (i=0; i<=nlon; i++) {
        x = cos(lon)*cos(lat);
        z = -sin(lon)*cos(lat);
        glNormal3f(x,y,z);
        glVertex3f(x*radius,y*radius,z*radius);
        lon += loninc;
      }
    glEnd();

    /* Quadrilateral stripes to cover the sphere */
    for (j=1; j<nlat-1; j++) {
      lon = 0;
      glBegin(GL_QUAD_STRIP);
        for (i=0; i<=nlon; i++) {
          x = cos(lon)*cos(lat);
          y = sin(lat);
          z = -sin(lon)*cos(lat);
          glNormal3f(x,y,z);
          glVertex3f(x*radius,y*radius,z*radius);
          x = cos(lon)*cos(lat+latinc);
          y = sin(lat+latinc);
          z = -sin(lon)*cos(lat+latinc);
          glNormal3f(x,y,z);
          glVertex3f(x*radius,y*radius,z*radius);
          lon += loninc;
        }
      glEnd();
      lat += latinc;
    }

    /* North-pole triangular fan */
    glBegin(GL_TRIANGLE_FAN);
      glNormal3f(0,1,0);
      glVertex3f(0,radius,0);
      y = sin(lat);
      lon = 0;
      for (i=0; i<=nlon; i++) {
        x = cos(lon)*cos(lat);
        z = -sin(lon)*cos(lat);
        glNormal3f(x,y,z);
        glVertex3f(x*radius,y*radius,z*radius);
        lon += loninc;
      }
    glEnd();

  glEndList();
}

/**********************************************************************/
void makeAtoms() {
/***********************************************************************
  Makes display-list of all atoms in the current frame using spheres.
***********************************************************************/
  int i;
  int k;
  float rval,gval,bval;
  double vv;

  glNewList(atomsid, GL_COMPILE);
  rval = Ratom; gval = Gatom; bval = Batom;  /* RGB color of an atom */
  for (i=0; i < nAtom; i++) {
    glPushMatrix();
    glTranslatef(r[i][0],r[i][1],r[i][2]);

    vv = 0.0;
    for (k=0; k<3; k++)
      vv = vv + rv[i][k]*rv[i][k];

    /* Color atom according to its velocity */
    rval = vv/max_vv;
    if(rval > 1.0){
      rval = 1.0;
    }
    bval = 1.0 - rval;

    glColor3f(rval,gval,bval);
    glCallList(sphereid);
    glPopMatrix();
  }
  glEndList();
}

/**********************************************************************/
void makeCurframeGeom() {
/***********************************************************************
  Reads the atoms information for the current time frame and makes the
  display-list of all the atoms' geometry.
***********************************************************************/
  makeAtoms();
}

/**********************************************************************/
void drawScene() {
/***********************************************************************
  Called by display() to draw the view of the current scene.
***********************************************************************/
  /* Define viewing transformation */
  gluLookAt(
    (GLdouble)eye[0],(GLdouble)eye[1],(GLdouble)eye[2],
    (GLdouble)center[0],(GLdouble)center[1],(GLdouble)center[2],
    (GLdouble)up[0],(GLdouble)up[1],(GLdouble)up[2]);
  glCallList(atomsid);
}

/**********************************************************************/
void display() {
/***********************************************************************
  Callback for glutDisplayFunc().  It clears the frame and depth 
  buffers and draws the atoms in the current frame.
***********************************************************************/
  glClearColor(105.0/255.0, 105.0/255.0, 105.0/255.0, 1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  drawScene();
  glutSwapBuffers();
}

/**********************************************************************/
void initView (float *min_ext, float *max_ext) {
/***********************************************************************
  Initializes global viewing, lighting, and projection values.
***********************************************************************/
  GLfloat light_diffuse[]   = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position1[] = {0.5, 0.5, 1.0, 0.0};
  float dif_ext[3],dis;
  int i;

  /* Define normal light */
  glLightfv(GL_LIGHT0,GL_DIFFUSE,light_diffuse);
  glLightfv(GL_LIGHT0,GL_POSITION,light_position1);

  /* Enable a single OpenGL light */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  /* Use depth buffering for hidden surface elimination */
  glEnable(GL_DEPTH_TEST);

  /* get diagonal and average distance of extent */
  for (i=0; i<3; i++) dif_ext[i] = max_ext[i] - min_ext[i];
  dis = 0.0;
  for (i=0; i<3; i++) dis += dif_ext[i]*dif_ext[i];
  dis = (float)sqrt((double)dis);

  /* set center in world space */
  for (i=0; i<3; i++) center[i] = min_ext[i] + dif_ext[i]/2.0;

  /* set initial eye & look at location in world space */
  eye[0] = center[0] + dis;
  eye[1] = center[1];
  eye[2] = center[2];
  up[0] = 0.0;
  up[1] = 1.0;
  up[2] = 0.0;

  /* set parameters for gluPerspective() */
  /* Near- & far clip-plane distances */
  near_clip = (GLdouble)( 0.5*(dis-0.5*dif_ext[2]) );
  far_clip  = (GLdouble)( 2.0*(dis+0.5*dif_ext[2]) );
  /* Field of view */
  fovy = (GLdouble)( 0.5*dif_ext[1]/(dis-0.5*dif_ext[2]) );
  fovy = (GLdouble)( 2*atan((double)fovy)/M_PI*180.0 );
  fovy = (GLdouble)(1.2*fovy);

  /* Enable the color material mode */
  glEnable(GL_COLOR_MATERIAL);
}

// /**********************************************************************/
// void readConf() {
// /***********************************************************************
// Read atomic coordinates from an MD-configuration file & allocates 
// necessary arrays.
// ***********************************************************************/
//   int l, j;

//   /* Open an MD-configuration file */
//   fp = fopen("md.conf","r");
//   /* Read the # of atoms */
//   fscanf(fp,"%d",&natoms);
//   /* allocate atoms array */
//   atoms = (AtomType *) malloc(sizeof(AtomType)*natoms);
//   /* Maximum & minimum extent of system in angstroms */
//   for (l=0; l<3; l++) fscanf(fp,"%f%f",&min_ext[l],&max_ext[l]);
//   /* Atomic coordinates */
//   for (j=0; j<natoms; j++)
//     fscanf(fp,"%f %f %f",&(atoms[j].crd[0]),&(atoms[j].crd[1]),
//                          &(atoms[j].crd[2]));
//   fclose(fp);
// }

void animate() { /* Callback function for idle events */
  /* Keep updating the scene until the last MD step is reached */
  if (stepCount <= StepLimit) {
    SingleStep(); /* One MD-step integration */
    if (stepCount%StepAvg == 0) EvalProps();
    makeCurframeGeom(); /* Redraw the scene (make a display list) */
    glutPostRedisplay(); // Force display event ~ user request for display() call
    ++stepCount;
  }
}

/**********************************************************************/
int main(int argc, char **argv) {
/**********************************************************************/

  glutInit(&argc, argv);

  // /* Read atomic coordinates from an MD-configuration file */
  // readConf();

  InitParams(); // Read and initialize MD parameters
  InitConf();
  ComputeAccel(); // Compute initial accelerations
  stepCount = 1; // Initialize the MD step count

  /* Set up an window */
  /* Initialize display mode */
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
  /* Specify window size */
  glutInitWindowSize(winx, winy);
  /* Open window */
  glutCreateWindow("Lennard-Jones Atoms");

  /* Initialize view */
  initView(min_ext, max_ext);

  /* Set a glut callback functions */
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(animate);

  /* generate an OpenGL display list for single sphere */
  sphereid = glGenLists(1);
  makeFastNiceSphere(sphereid,atom_radius);
  
  /* generate an OpenGL display list for the atoms' geometry */
  atomsid = glGenLists(1);
  /* make the geometry of the current frame's atoms */
  makeCurframeGeom();

  /* Start main display loop */
  glutMainLoop();
  
  return 0;
}
/**********************************************************************/

/*----------------------------------------------------------------------------*/
void InitParams() {
/*------------------------------------------------------------------------------
  Initializes parameters.
------------------------------------------------------------------------------*/
  int k;
  double rr,ri2,ri6,r1;

  /* Reads control parameters */
  scanf("%d%d%d",&InitUcell[0],&InitUcell[1],&InitUcell[2]);
  scanf("%le",&Density);
  scanf("%le",&InitTemp);
  scanf("%le",&DeltaT);
  scanf("%d",&StepLimit);
  scanf("%d",&StepAvg);

  /* Computes basic parameters */
  DeltaTH = 0.5*DeltaT;
  for (k=0; k<3; k++) {
    Region[k] = InitUcell[k]/pow(Density/4.0,1.0/3.0);
    RegionH[k] = 0.5*Region[k];
  }

  /* Constants for potential truncation */
  rr = RCUT*RCUT; ri2 = 1.0/rr; ri6 = ri2*ri2*ri2; r1=sqrt(rr);
  Uc = 4.0*ri6*(ri6 - 1.0);
  Duc = -48.0*ri6*(ri6 - 0.5)/r1;

  for(k=0; k<3; k++){
    min_ext[k] = 0.000000; max_ext[k] = Region[k];
  }
}

/*----------------------------------------------------------------------------*/
void InitConf() {
/*------------------------------------------------------------------------------
  r are initialized to face-centered cubic (fcc) lattice positions.  
  rv are initialized with a random velocity corresponding to Temperature.  
------------------------------------------------------------------------------*/
  double c[3],gap[3],e[3],vSum[3],vMag;
  int j,n,k,nX,nY,nZ;
  double seed;
  /* FCC atoms in the original unit cell */
  double origAtom[4][3] = {{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5},
                           {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}}; 
  double vv;

  /* Sets up a face-centered cubic (fcc) lattice */
  for (k=0; k<3; k++) gap[k] = Region[k]/InitUcell[k];
  nAtom = 0;
  for (nZ=0; nZ<InitUcell[2]; nZ++) {
    c[2] = nZ*gap[2];
    for (nY=0; nY<InitUcell[1]; nY++) {
      c[1] = nY*gap[1];
      for (nX=0; nX<InitUcell[0]; nX++) {
        c[0] = nX*gap[0];
        for (j=0; j<4; j++) {
          for (k=0; k<3; k++)
            r[nAtom][k] = c[k] + gap[k]*origAtom[j][k];
          ++nAtom;
        }
      }
    }
  }

  /* Generates random velocities */
  seed = 13597.0;
  vMag = sqrt(3*InitTemp);
  for(k=0; k<3; k++) vSum[k] = 0.0;
  /* Initialize half the MD box at a high velocity */
  for(n=0; n<nAtom*0.5; n++){
    RandVec3(e,&seed);
    for (k=0; k<3; k++) {
      rv[n][k] = vMag*e[k];
      vSum[k] = vSum[k] + rv[n][k];
    }
    n++;
    /* Ensure total momentum is zero so that velocity is kept as is later on */
    for (k=0; k<3; k++) {
      rv[n][k] = vMag*e[k]*-1;
      vSum[k] = vSum[k] + rv[n][k];
    }
  }
  /* Initialize the other half at a low velocity */
  for(; n<nAtom; n++) {
    for (k=0; k<3; k++) {
      rv[n][k] = 0;
      vSum[k] = vSum[k] + rv[n][k];
    }
  }
  /* Makes the total momentum zero */
  for (k=0; k<3; k++) vSum[k] = vSum[k]/nAtom;
  for (n=0; n<nAtom; n++) for(k=0; k<3; k++) rv[n][k] = rv[n][k] - vSum[k];

  /* Keep a record of max velocity squared for later color calculations */
  max_vv = 0.0;
  for (n=0; n<nAtom; n++){
    vv = 0.0;
    for (k=0; k<3; k++)
      vv = vv + rv[n][k]*rv[n][k];
    if(vv > max_vv){
      max_vv = vv;
    }
  }
}

/*----------------------------------------------------------------------------*/
void ComputeAccel() {
/*------------------------------------------------------------------------------
  Acceleration, ra, are computed as a function of atomic coordinates, r,
  using the Lennard-Jones potential.  The sum of atomic potential energies,
  potEnergy, is also computed.   
------------------------------------------------------------------------------*/
  double dr[3],f,fcVal,rrCut,rr,ri2,ri6,r1;
  int j1,j2,n,k;

  rrCut = RCUT*RCUT;
  for (n=0; n<nAtom; n++) for (k=0; k<3; k++) ra[n][k] = 0.0;
  potEnergy = 0.0;

  /* Doubly-nested loop over atomic pairs */
  for (j1=0; j1<nAtom-1; j1++) {
    for (j2=j1+1; j2<nAtom; j2++) {
      /* Computes the squared atomic distance */
      for (rr=0.0, k=0; k<3; k++) {
        dr[k] = r[j1][k] - r[j2][k];
        /* Chooses the nearest image */
        dr[k] = dr[k] - SignR(RegionH[k],dr[k]-RegionH[k])
                      - SignR(RegionH[k],dr[k]+RegionH[k]);
        rr = rr + dr[k]*dr[k];
      }
      /* Computes acceleration & potential within the cut-off distance */
      if (rr < rrCut) {
        ri2 = 1.0/rr; ri6 = ri2*ri2*ri2; r1 = sqrt(rr);
        fcVal = 48.0*ri2*ri6*(ri6-0.5) + Duc/r1;
        for (k=0; k<3; k++) {
          f = fcVal*dr[k];
          ra[j1][k] = ra[j1][k] + f;
          ra[j2][k] = ra[j2][k] - f;
        }
        potEnergy = potEnergy + 4.0*ri6*(ri6-1.0) - Uc - Duc*(r1-RCUT);
      } 
    } 
  }
}

/*----------------------------------------------------------------------------*/
void SingleStep() {
/*------------------------------------------------------------------------------
  r & rv are propagated by DeltaT in time using the velocity-Verlet method.
------------------------------------------------------------------------------*/
  int n,k;

  HalfKick(); /* First half kick to obtain v(t+Dt/2) */
  for (n=0; n<nAtom; n++) /* Update atomic coordinates to r(t+Dt) */
    for (k=0; k<3; k++) r[n][k] = r[n][k] + DeltaT*rv[n][k];
  ApplyBoundaryCond();
  ComputeAccel(); /* Computes new accelerations, a(t+Dt) */
  HalfKick(); /* Second half kick to obtain v(t+Dt) */
}

/*----------------------------------------------------------------------------*/
void HalfKick() {
/*------------------------------------------------------------------------------
  Accelerates atomic velocities, rv, by half the time step.
------------------------------------------------------------------------------*/
  int n,k;
  for (n=0; n<nAtom; n++)
    for (k=0; k<3; k++) rv[n][k] = rv[n][k] + DeltaTH*ra[n][k];
}

/*----------------------------------------------------------------------------*/
void ApplyBoundaryCond() {
/*------------------------------------------------------------------------------
  Applies periodic boundary conditions to atomic coordinates.
------------------------------------------------------------------------------*/
  int n,k;
  for (n=0; n<nAtom; n++) 
    for (k=0; k<3; k++) 
      r[n][k] = r[n][k] - SignR(RegionH[k],r[n][k])
                        - SignR(RegionH[k],r[n][k]-Region[k]);
}

/*----------------------------------------------------------------------------*/
void EvalProps() {
/*------------------------------------------------------------------------------
  Evaluates physical properties: kinetic, potential & total energies.
------------------------------------------------------------------------------*/
  double vv;
  int n,k;

  kinEnergy = 0.0;
  for (n=0; n<nAtom; n++) {
    vv = 0.0;
    for (k=0; k<3; k++)
      vv = vv + rv[n][k]*rv[n][k];
    kinEnergy = kinEnergy + vv;
  }
  kinEnergy *= (0.5/nAtom);
  potEnergy /= nAtom;
  totEnergy = kinEnergy + potEnergy;
  temperature = kinEnergy*2.0/3.0;

  /* Print the computed properties */
  printf("%9.6f %9.6f %9.6f %9.6f\n",
  stepCount*DeltaT,temperature,potEnergy,totEnergy);
}