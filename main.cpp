// Based on https://github.com/glfw/glfw/blob/537ea4ccf1deb7b5e00c3745ab51e82a8c0696dc/examples/gears.c

// Using glad config (no extensions yet):
// https://glad.dav1d.de/#language=c&specification=gl&api=gl%3D4.6&api=gles1%3D1.0&api=gles2%3D3.2&api=glsc2%3Dnone&profile=compatibility&loader=on

#if defined(_MSC_VER)
// Make MS math.h define M_PI
#define _USE_MATH_DEFINES
#endif

#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include "websocketpp/config/asio_no_tls_client.hpp"
#include "websocketpp/client.hpp"
#include "websocketpp/common/asio.hpp"

static GLfloat view_translation_x = 0.f, view_translation_y = 0.f, view_translation_z = -10.f;
static GLfloat view_rot = 10.f, view_rot_x = 15.f;
static GLfloat view_scale = 20.f;
//static GLfloat view_roty = 0.f, view_rotz = 0.f;
static GLfloat angle = 0.f;

std::mutex double_buffer_mutex;

struct point {
  float x;
  float y;
  float z;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class CombinedPointCloudClient : public std::enable_shared_from_this<CombinedPointCloudClient> {
public:
  explicit CombinedPointCloudClient()
	  : ws_client(), service() {
	// The most raw data we expect to get from 4 cameras with all data is ~47.19MB
	// (1024x768 * (xyz float32s + rgb bytes) * 4 cameras), so a 60MB buffer gives us some room
	points = new point[1024 * 768 * 4];
	back_points = new point[1024 * 768 * 4];
	point_count = 0;
	ws_client.init_asio(&service);
	ws_client.set_open_handler([this](auto &&PH1) { on_open(std::forward<decltype(PH1)>(PH1)); });
	ws_client.set_close_handler([this](auto &&PH1) { on_close(std::forward<decltype(PH1)>(PH1)); });
	ws_client.set_message_handler([this](auto &&PH1, auto &&PH2) {
	  on_message(std::forward<decltype(PH1)>(PH1),
				 std::forward<decltype(PH2)>(PH2));
	});

	ws_client.set_access_channels(websocketpp::log::alevel::none);

  }

  ~CombinedPointCloudClient() {
	this->stop();
  }

  void run() {
	std::string uri = "ws://localhost:9002";
	websocketpp::lib::error_code ec;
	websocketpp::client<websocketpp::config::asio_client>::connection_ptr
		connection = ws_client.get_connection(uri, ec);
	if (ec) {
	  std::cout << "could not create connection because: " << ec.message() << std::endl;
	  return;
	}
	ws_client.connect(connection);
	service.run();
  }

  void on_open(websocketpp::connection_hdl hdl) {
  }

  void on_close(websocketpp::connection_hdl hdl) {
  }

  void on_message(websocketpp::connection_hdl hdl,
	  // TODO: typedef this
				  websocketpp::config::asio_client::message_type::ptr message_ptr) {
	char *payload_ptr = (char *)message_ptr->get_raw_payload().c_str();
	size_t temp_point_count;
	size_t offset = 2;
	// After "pc"
	// TODO: Should actually check for the magic bytes (and make them more specific to begin with)

	temp_point_count = *((uint64_t *)(payload_ptr + offset));
	offset += sizeof(uint64_t);

	for (unsigned long i = 0; i < temp_point_count; i++) {
	  point curr_point{};

	  curr_point = *((struct point *)(payload_ptr + offset));
	  offset += sizeof(float) * 3 + sizeof(uint8_t) * 3;

	  this->back_points[i] = curr_point;
	}
//	const std::lock_guard<std::mutex> lock(double_buffer_mutex);
	this->point_count = temp_point_count;
	auto tmp_ptr = this->points;
	this->points = this->back_points;
	this->back_points = tmp_ptr;
  }

  void on_fail(websocketpp::connection_hdl hdl) {
	// TODO: Get some typedefs in here
	websocketpp::client<websocketpp::config::asio_client>::connection_ptr con = ws_client.get_con_from_hdl(hdl);

	std::cout << "Fail handler" << std::endl;
	std::cout << con->get_state() << std::endl;
	std::cout << con->get_local_close_code() << std::endl;
	std::cout << con->get_local_close_reason() << std::endl;
	std::cout << con->get_remote_close_code() << std::endl;
	std::cout << con->get_remote_close_reason() << std::endl;
	std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
  }

  void stop() {
	ws_client.stop_listening();
  }

  size_t point_count;
  point *points;
private:
  point *back_points;
  websocketpp::client<websocketpp::config::asio_client> ws_client{};
  boost::asio::io_service service{};
};

CombinedPointCloudClient *client;

void signal_handler(int signum) {
  client->stop();
  exit(signum);
}

static void draw(std::vector<point> &points) {
//  const std::lock_guard<std::mutex> lock(double_buffer_mutex);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  // Pull out from center of focus
  glTranslatef(view_translation_x, view_translation_y, view_translation_z);
  // Slight overhead angle
  glRotatef(view_rot_x, 1.0, 0.0, 0.0);
  // Constant animating rotation
  glRotatef(view_rot, 0.0, 1.0, 0.0);
//  glRotatef(angle, 0.0, 1.0, 0.0);

  glScalef(view_scale, view_scale, view_scale);

  glBegin(GL_POINTS);

  uint64_t i = 0;
//  auto point_count = points.size();
//  for (auto &point : points) {1024 * 768 * 4
  size_t max_buffer_size = 1024 * 768 * 1;
  while (i < client->point_count) {
	auto point = client->points[i];
//	glColor3f(point.r / 255.f, point.g / 255.f, point.b / 255.f);
	glColor3f((i % 2048) / 1023.f, (i % 65536) / 65535.f, i / (float) max_buffer_size);
//	glColor3f((rand() % 255) / 255.f, (rand() % 255) / 255.f, (rand() % 255) / 255.f);
	// The 7.f Y translation accounts for translation in the demo content I found online. Will have to adjust this for
	// the real thing.
	glVertex3f(point.x, -point.y, point.z);
	i++;
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
  case GLFW_KEY_W:view_translation_y -= 5.0;
	break;
  case GLFW_KEY_S:view_translation_y += 5.0;
	break;
  case GLFW_KEY_A:view_scale -= 5.0;
	break;
  case GLFW_KEY_D:view_scale += 5.0;
	break;
  case GLFW_KEY_LEFT:view_rot += 5.0;
	break;
  case GLFW_KEY_RIGHT:view_rot -= 5.0;
	break;
  default:return;
  }
}

void reshape([[maybe_unused]] GLFWwindow *window, int width, int height) {
  GLfloat h = (GLfloat)height / (GLfloat)width;
  GLfloat xmax, znear, zfar;

  // TODO @vinhowe: I modified this until I got the result I wanted. We should these values more intentionally.
  znear = 2.0f;
  zfar = 400.0f;
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
  glPointSize(5.f);
}

std::vector<point> load_points(const std::string &filename) {
  std::vector<point> points;
  std::ifstream ifile("test.txt", std::ios::in | std::ios::binary);

  ifile.seekg(2);
  uint64_t points_count = 0;
  ifile.read((char *)&points_count, sizeof(points_count));

  // Based on
  // https://github.com/IntelRealSense/realsense-ros/blob/fc11bfcd845825f0e44b9917ecac6b109839b1f5/realsense2_camera/src/base_realsense_node.cpp#L2415

  for (size_t i = 0; i < points_count; i++) {
	point curr_point{};

	ifile.read((char *)&curr_point, sizeof(float) * 3 + sizeof(uint8_t) * 3);

	points.push_back(curr_point);
  }

  ifile.close();
  return points;
}

/* program entry */
int main(int argc, char *argv[]) {
  GLFWwindow *window;
  int width, height;

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

  client = new CombinedPointCloudClient();
  std::thread client_thread{[]() { client->run(); }};

  auto points = load_points("test.txt");

  while (!glfwWindowShouldClose(window)) {
	draw(points);

	animate();

	glfwSwapBuffers(window);
	glfwPollEvents();
  }

  client->stop();
  client_thread.join();

  glfwTerminate();

  exit(EXIT_SUCCESS);
}
