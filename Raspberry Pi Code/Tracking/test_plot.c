// gcc live_pointcloud.c -lglfw -lGL -lm -ldl -o live_pointcloud

#include <GLFW/glfw3.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_POINTS 10000

typedef struct {
    float x, y, z;
} Point3D;

Point3D points[MAX_POINTS];
int num_points = 0;

// Generate fake new data
void add_point() {
    if (num_points >= MAX_POINTS) return;

    float pan = rand() % 1000;
    float tilt = rand() % 1000;
    float distance = rand() % 1000;

    points[num_points].x = pan / 500.0f - 1.0f;   // normalize to [-1, 1]
    points[num_points].y = tilt / 500.0f - 1.0f;
    points[num_points].z = distance / 1000.0f;    // [0, 1]
    num_points++;
}

void draw_points() {
    glBegin(GL_POINTS);
    for (int i = 0; i < num_points; i++) {
        float d = points[i].z; // use distance for color
        glColor3f(1.0f - d, 0.0f, d); // near = red, far = blue
        glVertex3f(points[i].x, points[i].y, points[i].z);
    }
    glEnd();
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Live Lidar Plot", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to open window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glPointSize(3.0f);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        glTranslatef(0, 0, -2); // move back to see points
        glRotatef(glfwGetTime() * 10, 0, 1, 0); // spin

        add_point(); // fake new point

        draw_points();

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (num_points >= MAX_POINTS) num_points = 0; // loop
    }

    glfwTerminate();
    return 0;
}
