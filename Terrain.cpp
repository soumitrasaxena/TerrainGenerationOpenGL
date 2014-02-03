#include <windows.h>  // for MS Windows
#include <GL/glut.h>  // GLUT, include glu.h and gl.h
#include <stdio.h>
#include <stdlib.h>
#include <math.h>   //to use sin and cos
//#include <math3d.h>
#include "vec3f.cpp"
#include "imageloader.cpp"

		

#define GL_PI 3.1415f

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

using namespace std;

GLuint textureID;
unsigned int texWidth , texHeight ;
unsigned char* texData;

GLfloat xRot = 0.0f , yRot = -0.0f , angle = 0.0f , zoom_out = -400.0f;
bool* keySpecialStates = new bool[256];

GLboolean pan =0 , rot = 1;

GLint modifier;

bool* keyNormalStates = new bool[256];

GLfloat xInitial=0 , yInitial=0 , xFinal=0 , yFinal=0 , x_trans=0.0f , y_trans=0.0f , light_y = 100.0f;

void keyOperations(void)
{
	if(keySpecialStates[GLUT_KEY_LEFT] == 1)
	{
		printf("left \n");
		yRot = yRot+1;
	}
	if(keySpecialStates[GLUT_KEY_RIGHT] ==1)
	{
		printf("right \n");
		yRot = yRot-1;
	}
	if(keySpecialStates[GLUT_KEY_UP] ==1)
	{
		printf("up \n");
		xRot = xRot-1;
	}
	if(keySpecialStates[GLUT_KEY_DOWN] ==1)
	{
		printf("down \n");
		xRot = xRot+1;
	}
	if(keyNormalStates['z'] == 1)
	{
		printf("z \n");
		zoom_out = zoom_out + 10;
	}

	if(keyNormalStates['x'] == 1)
	{
		printf("x \n");
		zoom_out = zoom_out - 10;
	}

	if(keyNormalStates['o'] == 1)
	{
		printf("o \n");
		light_y = light_y + 10;
	}

	if(keyNormalStates['l'] == 1)
	{
		printf("l \n");
		light_y = light_y - 10;
	}
}

GLuint loadTexture(Image *image)
{
	GLuint textureId;
	glGenTextures(1 , &textureId);

	glBindTexture(GL_TEXTURE_2D , textureId);
	glTexImage2D(GL_TEXTURE_2D , 0 , GL_RGB , image->width , image->height , 0 , GL_RGB , GL_UNSIGNED_BYTE , image->pixels);

	return textureId;
}

	
class Terrain
{
private :
	int w ;
	int l ;
	float** hs;
	Vec3f** normals;
	bool computedNormals;
	
public :

Terrain ( int w2 , int l2 )
{
	w = w2;
	l = l2;

	hs = new float*[l];
	for( int i=0 ; i<l ; i++)
	{
		hs[i] = new float[w];
	}

	normals = new Vec3f*[l];
	for(int i=0 ; i<l ; i++)
	{
		normals[i] = new Vec3f[w];
	}

	computedNormals = false;
}

~Terrain()
{
	for(int i=0;i<l;i++)
	{
		delete[] hs[i];
	}
	delete[] hs;

	for(int i=0 ; i<l ; i++)
	{
		delete[] normals[i];
	}
	delete[] normals;
}

int width()
{
	return w;

}

int length()
{
	return l;
}

//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y)
		{
			hs[z][x] = y;
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z)
		{
			return hs[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() 
		{
			if (computedNormals)
			{
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) 
			{
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) 
			{
				for(int x = 0; x < w; x++) 
				{
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) 
					{
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) 
					{
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) 
					{
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) 
					{
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) 
					{
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) 
					{
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) 
					{
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) 
					{
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) 
			{
				for(int x = 0; x < w; x++) 
				{
					Vec3f sum = normals2[z][x];
					
					if (x > 0) 
					{
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) 
					{
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) 
					{
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) 
					{
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) 
					{
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++)
			{
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) 
		{
			if (!computedNormals) 
			{
				computeNormals();
			}
			return normals[z][x];
		}

};

Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for(int y = 0; y < image->height; y++) {
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f)-0.5f);
			t->setHeight(x, y, h);
		}
	}
	
	delete image;
	t->computeNormals();
	return t;
}

Terrain* newTerrain;

void cleanup()
{
	delete newTerrain;
}

GLuint _textureId;

void SetupRC(void)
{
	glClearColor(0.0f , 0.0f , 0.0f , 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_NORMALIZE);
	//glDisable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	

	newTerrain = loadTerrain("height2.BMP" , 40);

	//Image* image = loadBMP("sky.BMP");
	//_textureId = loadTexture(image);
	//delete image;		
}

void RenderScene( void )
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	
	glPushMatrix();	

	glTranslatef(x_trans, y_trans , zoom_out);	
	glRotatef(xRot , 1.0f , 0.0f , 0.0f);
	glRotatef(yRot , 0.0f , 1.0f , 0.0f);

	glColor3f(1.0f , 1.0f , 1.0f);
	
	glBegin(GL_LINES);

	glVertex2f(-1000.0f , 0.0f);
	glVertex2f(1000.0f , 0.0f );

	glVertex2f(0.0f , 1000.0f);
	glVertex2f(0.0f , -1000.0f );

	glVertex3f(0.0f , 0.0f , -1000.0f);
	glVertex3f(0.0f , 0.0f , 1000.0f);

	glEnd();

	

	GLfloat light0_ambient[] = {0.0f, 0.0f , 0.0f , 1.0f};
	GLfloat light0_diffuse[] = {1.0f , 1.0f , 1.0f , 1.0};
	GLfloat light0_specular[] = {1.0f , 1.0f , 1.0f , 1.0};

	GLfloat light0_position[] = { 0.0f , light_y , 0.0f , 1.0f};

	GLfloat light0_spot_direction[] = {0.0f, -1.0f , 0.0f };

	glLightfv(GL_LIGHT0 , GL_AMBIENT , light0_ambient);
	glLightfv(GL_LIGHT0 , GL_DIFFUSE ,light0_diffuse);
	//glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT0 , GL_POSITION , light0_position);
	
	
	GLfloat lightColor1[] = {0.11f , 1.0f , 0.85f , 0.1f };
	GLfloat lightPos1[] = {200.0f , 500.0f , 200.0f , 1.0f } ;

	//glLightfv(GL_LIGHT1 , GL_DIFFUSE , lightColor1);
	//glLightfv(GL_LIGHT1 , GL_SPECULAR , lightColor1);
	//glLightfv(GL_LIGHT1 , GL_POSITION ,lightPos1);
	
	GLfloat material_diffuse[] = { 0.0f , 1.0f , 0.6f , 0.3f };
	
	glMaterialfv(GL_FRONT , GL_DIFFUSE , material_diffuse);
	
	
	if(_textureId != 0 )
	{
		printf("Texture Loaded \n ");
	}
		
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D , _textureId);
	
	//glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_MIN_FILTER , GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_MAG_FILTER , GL_LINEAR);
	
		
	float scale = 50.0f / max( newTerrain->width() -1 , newTerrain->length() -1 );
	/*	
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f , 0.0f , 1.0f);
	glTexCoord2f(0.0f , 0.0f );
	glVertex2f(-200.0f ,-200.0f);
	glTexCoord2f(1.0f , 0.0f );
	glVertex2f(200.0f , -200.0f);
	glTexCoord2f(1.0f , 1.0f );
	glVertex2f(200.0f ,	 200.0f);
	glTexCoord2f(0.0f , 1.0f );
	glVertex2f(-200.0f , 200.0f);
	glEnd();
	*/
	for(int translate_x = 0 ; translate_x <= 3*(newTerrain->width()-1) ; translate_x = translate_x+newTerrain->width()-1)
	{
		glPushMatrix();
		glTranslatef(translate_x , 0 , 0);
				for(int z=0 ; z < newTerrain->length()-1 ; z++)
				{
					glBegin(GL_TRIANGLE_STRIP);
						for( int x=0 ; x< newTerrain->width() ; x++)
						{
							Vec3f normal = newTerrain->getNormal(x , z);
							glNormal3f(normal[0] , normal[1] , normal [2]);
							glVertex3f(x , newTerrain->getHeight(x,z) , z);
							normal = newTerrain->getNormal(x , z+1);
							glNormal3f(normal[0], normal[1], normal[2]);
							glVertex3f(x, newTerrain->getHeight(x, z + 1), z+1);
						}
					glEnd();
				}
		
		glPopMatrix();			
	}
		
	printf("Render scene working \n");
	
	glPopMatrix();
	glutSwapBuffers();

}
/*

void Timer (int value)
{
	angle = angle+10;	
	

	glutPostRedisplay();
	glutTimerFunc(33 , Timer , 1);
	
}

*/
void ChangeSize(GLsizei w , GLsizei h)
{
	GLfloat aspectRatio;

	if(h==0)
	{
		h=1;
	}

	glViewport(0,0,w,h);

	//Resetting coordinate system
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	aspectRatio = (GLfloat) w / (GLfloat) h ;

	gluPerspective(45.0f , aspectRatio , 1.0f , 800.0f );
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void processSpecialKeys( int key , int x , int y)
{
	keySpecialStates[key] = true;
	printf("Special Keys DOWN registered \n");	
	printf("%d \t %d \n",x,y);
	keyOperations();
	glutPostRedisplay();
}

void processSpecialKeysUp( int key , int x , int y)
{
	keySpecialStates[key] = false;
	printf("Special Keys UP registered \n");	
	printf("%d \t %d \n",x,y);
	//glutPostRedisplay();
}

void processKeyboardKeys( unsigned char key , int x , int y)
{
	keyNormalStates[key] = true;
	printf("Normal Keys DOWN registered \n");	
	printf(" %f \t %f \t %f \n" , xRot , yRot , zoom_out);
	keyOperations();
	glutPostRedisplay();
}

void processKeyboardKeysUP( unsigned char key , int x , int y)
{
	keyNormalStates[key] = false;
	printf("Normal Keys UP registered \n");	
	printf(" %f \t %f \t %f \n" , xRot , yRot , zoom_out);
	keyOperations();
	glutPostRedisplay();
}

void processMouse(int button , int state , int x , int y)
{
	if ( state == GLUT_DOWN)
	{
		printf("mouse DOWN captured \n");
		xInitial = x;
		yInitial = y;
	}

	if(keyNormalStates['c'] == 1 && state == GLUT_DOWN)
	{
		pan = 1;
		rot = 0;
	}
	
	if( state == GLUT_UP )
	{
		pan = 0;
		rot = 1;
	}
	
}

void processActiveMotion( int x , int y)
{
	xFinal=x;
	yFinal=y;

	if(rot == 1)
	{
	xRot = xRot + (yFinal-yInitial)/50;
	yRot = yRot + (xFinal-xInitial)/50;
	}

	printf(" xRot = %f and yRot = %f \n " , xRot , yRot);

	if(pan == 1 )
	{
		x_trans = x_trans+(xFinal - xInitial)/50;
		y_trans = y_trans-(yFinal - yInitial)/50;

	}


	glutPostRedisplay();
}

int main( int argc , char* argv[])
{
	glutInit(&argc , argv);
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	
	glutCreateWindow("TERRAIN");
	
	glutKeyboardFunc(processKeyboardKeys);
	glutKeyboardUpFunc(processKeyboardKeysUP);
	
	glutSpecialFunc(processSpecialKeys);
	glutSpecialUpFunc(processSpecialKeysUp);
	
	glutMouseFunc(processMouse);
	glutMotionFunc(processActiveMotion);

	glutDisplayFunc(RenderScene);
	//glutIdleFunc(RenderScene);
	glutReshapeFunc(ChangeSize);
	//glutTimerFunc(33 , Timer , 1 );
	

	SetupRC();
	
	glutMainLoop();
	return 0;

}