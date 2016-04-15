#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

using namespace std;
int cube_key=0;
float camera_rotation_angle = 0;
int x_pos=-10,z_pos=8;
float x_theta=70,y_theta=0;
float z_closness=20;
float y_height=0;
int cam_mode=0 ;// 0-Default 1-Chase 


struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
	GLenum FillMode; // GL_FILL, GL_LINE
	int NumVertices;
};

typedef struct VAO VAO;

struct Cube {
       int alive;
       int mobile;
       float jump;
       VAO *vao;
} cube[130];

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID; // For use with normal shader
	GLuint TexMatrixID; // For use with texture shader
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	cout << "Compiling shader : " <<  vertex_file_path << endl;
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	cout << VertexShaderErrorMessage.data() << endl;

	// Compile Fragment Shader
	cout << "Compiling shader : " << fragment_file_path << endl;
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	cout << FragmentShaderErrorMessage.data() << endl;

	// Link the program
	cout << "Linking program" << endl;
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	cout << ProgramErrorMessage.data() << endl;

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
 {
	cout << "Error: " << description << endl;
 }

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  1,                  // attribute 1. Color
						  3,                  // size (r,g,b)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}
void drawFont(const char *c,float x,float y,float z,int color,int effect)
{
  glm::mat4 MVP;  // MVP = Projection * View * Model

  static int fontScale = 0;
  float fontScaleValue = 0.75;
  static int color_val=4;
  if(color==1)
   color_val = (color_val+1) % 360;
  glm::vec3 fontColor = getRGBfromHue (color_val);

  
  glUseProgram(fontProgramID);
  // Transform the text
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateText = glm::translate(glm::vec3(x,y,0));
  glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
  Matrices.model *= (translateText * scaleText);
  MVP = Matrices.projection * Matrices.view * Matrices.model;
  // send font's MVP and font color to fond shaders
  glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
  glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

  // Render font
  GL3Font.font->Render(c);

  // font size and color changes
  if(effect==1)
   fontScale = (fontScale + 1) % 360;
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = -1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;

            case GLFW_KEY_W:
                if(x_theta<90)
                x_theta+=10;
                break;

            case GLFW_KEY_S:
                if(x_theta>-90)
                x_theta-=10;
                break;

            case GLFW_KEY_T:
                z_closness=20;
                x_theta=90;
                break;

            case GLFW_KEY_C:
                cam_mode=1;
                break;    

            case GLFW_KEY_E:
                cam_mode=0;
                x_theta=70;
                z_closness=20;
                break;

            case GLFW_KEY_N:
				 z_closness-=2;
				break;
            case GLFW_KEY_F:
				 z_closness+=2;
				break;
            case GLFW_KEY_P:
				 y_height-=2;
				break;
            case GLFW_KEY_O:
				 y_height+=2;
				break;

            case GLFW_KEY_UP:
				if(z_pos>-10) z_pos-=2;
				break;

            case GLFW_KEY_DOWN:
                if(z_pos<8)   z_pos+=2;
				break;

			case GLFW_KEY_LEFT:
				if(x_pos>-10) x_pos-=2;
				break;

			case GLFW_KEY_RIGHT:
                if(x_pos<8)   x_pos+=2;
				break;				
			default:
				break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			if (action == GLFW_RELEASE)
				triangle_rot_dir *= -1;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_RELEASE) {
				rectangle_rot_dir *= -1;
			}
			break;
		default:
			break;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	 is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);
	/* glMatrixMode (GL_PROJECTION);
	 glLoadIdentity ();
	 gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

}


// Creates the rectangle object used in this sample code
void createCube (GLfloat red, GLfloat blue, GLfloat green,float l, float b,float h,const char *c)
{ 
  
   const GLfloat vertex_buffer_data [] = {

     0,0,0, // triangle 1 : begin
     0,0,h,
     0,b,h, // triangle 1 : end

     l,b,0, // triangle 2 : begin
     0,0,0,
     0, b,0, // triangle 2 : end

     l,0, h,
     0,0,0,
     l,0,0,

     l, b,0,
     l,0,0,
     0,0,0,

     0,0,0,
     0, b, h,
     0, b,0,

     l,0, h,
     0,0, h,
	 0,0,0,

     0, b, h,
     0,0, h,
     l,0, h,

     l, b, h,
     l,0,0,
     l, b,0,

     l,0,0,
     l, b, h,
     l,0, h,

     l, b, h,
     l, b,0,
     0, b,0,

     l, b, h,
     0, b,0,
     0, b, h,

     l, b, h,
     0, b, h,
     l,0, h

 };

  
  GLfloat color_buffer_data[36*3];
  for(int i = 0; i < 36; i++)
  {
    color_buffer_data[i + 0] = 1;
    color_buffer_data[i + 1] = 0;
    color_buffer_data[i + 2] = 0;
  }

  if(c==NULL)
  cube[cube_key++].vao = create3DObject(GL_TRIANGLES,36, vertex_buffer_data, color_buffer_data, GL_FILL);
  else
  {
	glActiveTexture(GL_TEXTURE0);
	GLuint textureID = createTexture(c);
	if(textureID == 0 )
    cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;
	
	// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
	const GLfloat texture_buffer_data [] =
	 {
		0,1, 
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,  
		0,1, 
		0,1,
		0,1,
		0,1,
		0,1,  // top rectangle
		1,1,
		1,0, //
		0,1,
		1,0,
		0,0, // top rectangle ends
		0,1,
		0,1, 
		0,1
	};
     cube[cube_key ++].vao= create3DTexturedObject(GL_TRIANGLES,36,vertex_buffer_data, texture_buffer_data,textureID, GL_FILL);
}
}
void moveCube(int key , float x, float y,float z,float cube_rotation)
{  glUseProgram (programID);

  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP; 
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRec = glm::translate (glm::vec3(x, y, z)); // glTranslatef
  glm::mat4 rotateRec = glm::rotate((float)(cube_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRec * rotateRec); 
  MVP = VP * Matrices.model;

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(cube[key].vao);
   
}

void handle_jump(int key)
 {
   if(cube[key].jump<=3)
	cube[key].jump+=0.01;
   else
	cube[key].jump=0;

  }

void move_Text_Cube(int key , float x, float y,float z,float cube_rotation)
{
	glUseProgram(textureProgramID);

  glm::mat4 VP = Matrices.projection * Matrices.view;
  glm::mat4 MVP; 
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRec = glm::translate (glm::vec3(x, y, z)); // glTranslatef
  
   if(cube[key].mobile==1 && key!=100)
  	 {handle_jump(key);
  	 translateRec = glm::translate (glm::vec3(x,cube[key].jump, z)); // glTranslatef
     }
  glm::mat4 rotateRec = glm::rotate((float)(cube_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRec * rotateRec); 
  MVP = VP * Matrices.model;


   glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
   glUseProgram(textureProgramID);
   glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);
   draw3DTexturedObject(cube[key].vao);	
 
}




/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // use the loaded shader program
  glUseProgram (programID);
  // glm::vec4(0,0,10,1);
  // glm::mat4 rotate = glm::rotate((float)(x_theta*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
  // Eye - Location of camera. Don't change unless you are sure!!
  // glm::vec3 eye ( 15*cos(camera_rotation_angle*M_PI/180.0f),0, 15.0*sin(camera_rotation_angle*M_PI/180.0f) );
  float x=0,y=y_height,z=z_closness;
  float length=sqrt(x*x+y*y+z*z);
  // about y axis
  // z=length*cos(y_theta*M_PI/180.0f);
  // x=length*sin(y_theta*M_PI/180.0f);
  // about x axis
  y=length*sin(x_theta*M_PI/180.0f);
  z=length*cos(x_theta*M_PI/180.0f);
  if(cam_mode==0)
  {
  glm::vec3 eye ( x, y, z );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  }
  if(cam_mode==1)
   { 
  glm::vec3 eye ( x_pos, y+3, z_pos+3 );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (x_pos, 8, z_pos);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
     glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
   }


}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "BOARD GAME", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}

void initGL (GLFWwindow* window, int width, int height)
{
	

	// Create and compile our GLSL program from the texture shaders
	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" ); // HALA
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (0.0f, 0.4f, 0.6f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise FTGL stuff
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create and compile our GLSL program from the font shaders
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

}
  
int main (int argc, char** argv)
{
	int width = 1300;
	int height = 600;
    GLFWwindow* window = initGLFW(width, height);
    initGL (window, width, height);
 
    int x=0;
	char land[]="land.jpeg" ,player[20]="player.jpeg",water[20]="water.jpeg",jumper[20]="jumper.jpeg";
	char last[]="last.jpeg";
	// 		MAKE BOARD
    for(int i =0;i<10;i++)
    	for (int j=0;j<10;j++)
    	  {
    	  if(rand()%10==9 && x!=90 && x!=9) // SET some cubes to be holes // avoid cube 10 ,which is the starting point
    	  	cube[x].alive=0;
          else
            cube[x].alive=1;              
          if (x==9)
          	 createCube(0,0,0,2,8,2,&last[0]); // use normal texture
          else if(rand()%8==7 && x!=90) // SET some jumping cubes    // avoid cube 10 ,which is the starting point
           {
           	cube[x].mobile=1;
           	cube[x].jump=rand()%3;
            createCube(0,0,0,2,8,2,&jumper[0]); // use jumper texture
           }
          else
             createCube(0,0,0,2,8,2,&land[0]);	
          x++;
	      }

    createCube(0,0,0,2,1,2,&player[0]); // PLAYER  
     cube[100].mobile=0;
	createCube(0,0,0,6,8,20,&water[0]); // WATER
	createCube(0,0,0,6,8,20,&water[0]); // WATER
	createCube(0,0,0,32,8,6,&water[0]); // WATER
	createCube(0,0,0,32,8,6,&water[0]); // WATER
	while (!glfwWindowShouldClose(window)) {
		draw();
		move_Text_Cube(101,-16,0,-10,0);// water
		move_Text_Cube(102,10,0,-10,0);// water
		move_Text_Cube(103,-16,0,-16,0);// water
        move_Text_Cube(104,-16,0,8,0);// water

		move_Text_Cube(100,x_pos,8,z_pos,0); //player
		//board
		 x=0;
		for(int i=-5;i<5;i++)
			for (int j=-5;j<5;j++)
			{   if(cube[x].alive==1)
                move_Text_Cube(x,j*2,0,i*2,0);
                x++;
			}
	// char h[]="sdgdgdg";		
    // drawFont(&h[0],-3,5,3,1,0);		
    glfwSwapBuffers(window);
    glfwPollEvents();
	}
    glfwTerminate();
	exit(EXIT_SUCCESS);
}

