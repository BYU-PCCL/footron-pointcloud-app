// Based on https://github.com/glfw/glfw/blob/537ea4ccf1deb7b5e00c3745ab51e82a8c0696dc/examples/gears.c

// Using glad config (no extensions yet):
// https://glad.dav1d.de/#language=c&specification=gl&api=gl%3D4.6&api=gles1%3D1.0&api=gles2%3D3.2&api=glsc2%3Dnone&profile=compatibility&loader=on

#if defined(_MSC_VER)
// Make MS math.h define M_PI
#define _USE_MATH_DEFINES
#endif

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>

#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static GLfloat view_translation_x = 0.f, view_translation_y = 0.f, view_translation_z = -15.f;
static GLfloat view_rot_x = -40.f;
//static GLfloat view_roty = 0.f, view_rotz = 0.f;
static GLfloat angle = 0.f;

static void draw(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &cloud) {
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  // Pull out from center of focus
  glTranslatef(view_translation_x, view_translation_y, view_translation_z);
  // Slight overhead angle
  glRotatef(view_rot_x, 1.0, 0.0, 0.0);
  // Constant animating rotation
  glRotatef(angle, 0.0, 1.0, 0.0);
  // This number (40) is really specific to the dimensions of the data I'm working with to test this
  glScalef(40.f, 40.f, 40.f);
  // glRotatef(view_roty, 0.0, 1.0, 0.0);
  // glRotatef(view_rotz, 0.0, 0.0, 1.0);

  glBegin(GL_POINTS);
  for (auto point : cloud->points) {
	glColor3f(point.r / 255.f, point.g / 255.f, point.b / 255.f);
	// The 7.f Y translation accounts for translation in the demo content I found online. Will have to adjust this for
	// real thing.
	glVertex3f(point.x, -(point.y - 0.7f), point.z);
  }
  glEnd();

  glPopMatrix();
}

static void animate() {
  angle = 20.f * (float)glfwGetTime();
}

void key(GLFWwindow *window, int k, int s, int action, [[maybe_unused]] int mods) {
  if (action != GLFW_PRESS)
	return;

  switch (k) {
  case GLFW_KEY_ESCAPE:glfwSetWindowShouldClose(window, GLFW_TRUE);
	break;
  case GLFW_KEY_UP:view_rot_x += 5.0;
	break;
  case GLFW_KEY_DOWN:view_rot_x -= 5.0;
	break;
  default:return;
  }
}

void reshape([[maybe_unused]] GLFWwindow *window, int width, int height) {
  GLfloat h = (GLfloat)height / (GLfloat)width;
  GLfloat xmax, znear, zfar;

  // TODO @vinhowe: I modified this until I got the result I wanted. We should these values more intentionally.
  znear = 2.0f;
  zfar = 300.0f;
  xmax = znear * 0.5f;

  glViewport(0, 0, (GLint)width, (GLint)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-xmax, xmax, -xmax * h, xmax * h, znear, zfar);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -20.0);
}

static void init() {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glPointSize(2.f);
}

// Adapted from https://pcl.readthedocs.io/en/latest/reading_pcd.html
pcl::PointCloud<pcl::PointXYZRGB>::Ptr loadPointCloudFile(const std::string &name) {
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);

  if (pcl::io::loadPCDFile<pcl::PointXYZRGB>(name, *cloud) == -1) {
	std::cerr << "Couldn't load pcd file "
			  << name
			  << std::endl;
	return nullptr;
  }

  std::cout << "Loaded "
			<< cloud->width * cloud->height
			<< " data points from "
			<< name
			<< std::endl;

  return cloud;
}

/* program entry */
int main(int argc, char *argv[]) {
  GLFWwindow *window;
  int width, height;

  auto cellPhoneCloud = loadPointCloudFile("cell_phone_1.pcd");
  auto waterBottleCloud = loadPointCloudFile("water_bottle_1.pcd");

  if (!cellPhoneCloud || !waterBottleCloud) {
	std::cerr << "Failed to load one or more point clouds" << std::endl;
	return 1;
  }

  if (!glfwInit()) {
	std::cerr << "Failed to initialize GLFW" << std::endl;
	return 1;
  }

  glfwWindowHint(GLFW_DEPTH_BITS, 16);
  // @vinhowe: Don't know why we'd want the window to be transparent (that's what setting this to true does)
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

  window = glfwCreateWindow(300, 300, "point cloud renderer", nullptr, nullptr);
  if (!window) {
	std::cerr << "Failed to open GLFW window" << std::endl;
	glfwTerminate();
	exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(window, reshape);
  glfwSetKeyCallback(window, key);

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);

  glfwGetFramebufferSize(window, &width, &height);
  reshape(window, width, height);

  init();

  int count = 0;
  double lastTime = 0;
  while (!glfwWindowShouldClose(window)) {
	draw(count % 2 == 0 ? waterBottleCloud : cellPhoneCloud);

	animate();

	glfwSwapBuffers(window);
	glfwPollEvents();

	double currentTime = glfwGetTime() * 1000.f;
	if (lastTime != 0) {
	  std::cout << currentTime - lastTime << std::endl;
	}
	lastTime = currentTime;
	count++;
  }

  glfwTerminate();

  exit(EXIT_SUCCESS);
}
