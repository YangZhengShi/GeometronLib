/*
 * test2.cpp
 * 
 * This file is part of the "GeometronLib" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

//#define GS_ROW_MAJOR_STORAGE
#define GS_ENABLE_SWIZZLE_OPERATOR
//#define GS_HIGH_PRECISION_FLOAT
//#define GS_ROW_VECTORS

#include <Gauss/Gauss.h>
#include <Gauss/GLSLTypes.h>
#include <Geom/Geom.h>

#ifdef WIN32
#include <Windows.h>
#endif

#include <gl/glut.h>


// ----- MACROS -----

#ifdef GS_HIGH_PRECISION_FLOAT
#   define glLoadMatrix_T(p) glLoadMatrixd(p)
#else
#   define glLoadMatrix_T(p) glLoadMatrixf(p)
#endif

#define DEG_TO_RAD(x) ((x)*pi/Gs::Real(180))

#define TEST_PROJECTION_MORPHING


// ----- STRUCTURES -----

struct Model
{
    Gm::TriangleMesh    mesh;
    Gm::Transform3      transform;
    Gs::Vector4         color { 1, 1, 1, 1 };
};


// ----- VARIABLES -----

static const Gs::Real pi = Gs::Real(3.141592654);

Gs::Vector2i            resolution;
Gs::ProjectionMatrix4   projection;

Gm::Transform3          cameraTransform;
Gs::AffineMatrix4       viewMatrix;

Gs::Real                fov = Gs::Real(74.0);

std::vector<Model>      models;

bool                    projMorphing        = false;
bool                    projMorphingOrtho   = false;

Gm::Spline2             spline;


// ----- FUNCTIONS -----

Model* createCuboidModel(const Gm::MeshGenerator::CuboidDescription& desc)
{
    models.resize(models.size() + 1);
    auto mdl = &(models.back());

    mdl->mesh = Gm::MeshGenerator::Cuboid(desc);

    return mdl;
}

void updateProjection()
{
    #ifdef TEST_PROJECTION_MORPHING

    // setup perspective projection
    auto perspProj = Gs::ProjectionMatrix4::Perspective(
        static_cast<Gs::Real>(resolution.x) / resolution.y,
        0.1f,
        100.0f,
        DEG_TO_RAD(fov),
        Gs::ProjectionFlags::OpenGLPreset
    );

    // setup orthogonal projection
    const auto orthoZoom = Gs::Real(0.005);
    auto orthoProj = Gs::ProjectionMatrix4::Orthogonal(
        static_cast<Gs::Real>(resolution.x) * orthoZoom,
        static_cast<Gs::Real>(resolution.y) * orthoZoom,
        0.1f,
        100.0f,
        Gs::ProjectionFlags::OpenGLPreset
    );

    // update morphing animation
    static Gs::Real morphing;

    if (projMorphing)
    {
        const auto speed = Gs::Real(0.1);

        if (projMorphingOrtho)
        {
            morphing += speed;

            if (morphing >= 1.0f - Gs::epsilon)
            {
                morphing = 1.0f;
                projMorphing = false;
            }
        }
        else
        {
            morphing -= speed;

            if (morphing <= 0.0f + Gs::epsilon)
            {
                morphing = 0.0f;
                projMorphing = false;
            }
        }
    }

    // setup final projection
    projection = Gs::Lerp(perspProj, orthoProj, morphing);

    #else

    // setup perspective projection
    projection = Gs::ProjectionMatrix4::Perspective(
        static_cast<Gs::Real>(resolution.x) / resolution.y,
        0.1f,
        100.0f,
        DEG_TO_RAD(fov),
        Gs::ProjectionFlags::OpenGLPreset
    );

    #endif
}

void initGL()
{
    // setup GL configuration
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // initialize projection
    updateProjection();

    // create model
    Gm::MeshGenerator::CuboidDescription mdlDesc;
    mdlDesc.size = { 1, 1.5f, 0.5f };
    auto mdl = createCuboidModel(mdlDesc);

    mdl->transform.SetPosition({ 0, 0, -2 });
}

void initScene()
{
    spline.AddPoint({ 50, 50 }, 0.0f);
    spline.AddPoint({ 150, 200 }, 1.0f);
    spline.AddPoint({ 320, 170 }, 2.0f);
    spline.AddPoint({ 500, 80 }, 3.0f);
    spline.AddPoint({ 400, 350 }, 4.0f);
    spline.AddPoint({ 460, 500 }, 5.0f);
    spline.AddPoint({ 250, 350 }, 6.0f);
    spline.SetOrder(3);
}

void emitVertex(const Gm::TriangleMesh::Vertex& vert)
{
    // generate color from vertex data
    Gs::Vector4 color(vert.texCoord.x, vert.texCoord.y, 0.5f, 1);

    // emit vertex data
    glNormal3fv(vert.normal.Ptr());
    glTexCoord2fv(vert.texCoord.Ptr());
    glColor4fv(color.Ptr());
    glVertex3fv(vert.position.Ptr());
}

void drawModel(const Model& mdl)
{
    // setup world-view matrix
    auto modelView = (viewMatrix * mdl.transform.GetMatrix()).ToMatrix4();
    glLoadMatrix_T(modelView.Ptr());

    // draw model
    glBegin(GL_TRIANGLES);

    for (const auto& tri : mdl.mesh.triangles)
    {
        const auto& v0 = mdl.mesh.vertices[tri.a];
        const auto& v1 = mdl.mesh.vertices[tri.b];
        const auto& v2 = mdl.mesh.vertices[tri.c];

        emitVertex(v0);
        emitVertex(v1);
        emitVertex(v2);
    }

    glEnd();
}

void drawLine(const Gs::Vector3& a, const Gs::Vector3& b)
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex3fv(a.Ptr());

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex3fv(b.Ptr());
}

void drawAABB(const Gm::AABB3& box)
{
    glLoadIdentity();

    glBegin(GL_LINES);

    auto v0 = box.min;
    auto v1 = box.max;

    drawLine({ v0.x, v0.y, v0.z }, { v1.x, v0.y, v0.z });
    drawLine({ v0.x, v1.y, v0.z }, { v1.x, v1.y, v0.z });
    drawLine({ v0.x, v0.y, v0.z }, { v0.x, v1.y, v0.z });
    drawLine({ v1.x, v0.y, v0.z }, { v1.x, v1.y, v0.z });

    drawLine({ v0.x, v0.y, v1.z }, { v1.x, v0.y, v1.z });
    drawLine({ v0.x, v1.y, v1.z }, { v1.x, v1.y, v1.z });
    drawLine({ v0.x, v0.y, v1.z }, { v0.x, v1.y, v1.z });
    drawLine({ v1.x, v0.y, v1.z }, { v1.x, v1.y, v1.z });

    drawLine({ v0.x, v0.y, v0.z }, { v0.x, v0.y, v1.z });
    drawLine({ v1.x, v0.y, v0.z }, { v1.x, v0.y, v1.z });
    drawLine({ v1.x, v1.y, v0.z }, { v1.x, v1.y, v1.z });
    drawLine({ v0.x, v1.y, v0.z }, { v0.x, v1.y, v1.z });

    glEnd();
}

void drawSpline(const Gm::Spline2& spline, Gs::Real a, Gs::Real b, std::size_t details)
{
    // draw spline curve
    glBegin(GL_LINE_STRIP);

    Gs::Real step = (b - a) / details;

    for (std::size_t i = 0; i <= details; ++i)
    {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // interpolate vertex
        auto p = spline(a);
        glVertex2fv(p.Ptr());
        a += step;
    }

    glEnd();

    // draw spline control points
    glPointSize(5.0f);

    glBegin(GL_POINTS);

    for (const auto& p : spline.GetPoints())
    {
        glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
        glVertex2fv(p.point.Ptr());
    }

    glEnd();

    glPointSize(1.0f);
}

void updateScene()
{
    auto& trans = models[0].transform;

    auto rotation = trans.GetRotation().ToMatrix3();
    const auto motion = Gs::Real(0.002);

    Gs::RotateFree(rotation, Gs::Vector3(1, 1, 1).Normalized(), pi*motion);
    trans.SetRotation(Gs::Quaternion(rotation));
}

void drawScene3D()
{
    // setup projection
    #ifdef TEST_PROJECTION_MORPHING
    updateProjection();
    #endif

    glMatrixMode(GL_PROJECTION);
    auto proj = projection.ToMatrix4();
    glLoadMatrix_T(proj.Ptr());

    // update view matrix
    glMatrixMode(GL_MODELVIEW);
    viewMatrix = cameraTransform.GetMatrix().Inverse();

    // draw models
    for (const auto& mdl : models)
    {
        drawModel(mdl);
        drawAABB(mdl.mesh.BoundingBox(mdl.transform.GetMatrix()));
    }
}

void drawSceen2D()
{
    // setup projection
    glMatrixMode(GL_PROJECTION);
    auto proj = Gs::ProjectionMatrix4::Planar(resolution.x, resolution.y);
    glLoadMatrix_T(proj.Ptr());

    // setup model-view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // draw scene
    drawSpline(spline, 0.0f, 6.0f, 500);
}

void displayCallback()
{
    // update scene
    updateScene();

    // draw frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    {
        //drawScene3D();
        drawSceen2D();
    }
    glutSwapBuffers();
}

void idleCallback()
{
    glutPostRedisplay();
}

void reshapeCallback(GLsizei w, GLsizei h)
{
    resolution.x = w;
    resolution.y = h;

    glViewport(0, 0, w, h);

    #ifndef TEST_PROJECTION_MORPHING
    updateProjection();
    #endif
}

void keyboardCallback(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27: // ESC
            exit(0);
            break;

        case '\r': // ENTER
            projMorphing = true;
            projMorphingOrtho = !projMorphingOrtho;
            break;
    }
}

void specialCallback(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_UP:
            spline.SetOrder(spline.GetOrder() + 1);
            std::cout << "Spline Order = " << spline.GetOrder() << std::endl;
            break;

        case GLUT_KEY_DOWN:
            spline.SetOrder(spline.GetOrder() - 1);
            std::cout << "Spline Order = " << spline.GetOrder() << std::endl;
            break;
    }
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

    glutInitWindowSize(800, 600);
    glutInitWindowPosition(350, 250);
    glutCreateWindow("GeometronLib Test 2 (OpenGL, GLUT)");
    
    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutIdleFunc(idleCallback);
    glutSpecialFunc(specialCallback);
    glutKeyboardFunc(keyboardCallback);

    initGL();
    initScene();

    glutMainLoop();

    return 0;
}
