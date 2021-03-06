/*
 * Test2_Mesh.cpp
 *
 * This file is part of the "GeometronLib" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include <Gauss/Gauss.h>
#include <Gauss/GLSLTypes.h>
#include <Geom/Geom.h>

#if defined(_WIN32)
#   include <Windows.h>
#   include <gl/glut.h>
#elif defined(__APPLE__)
#   include <GLUT/GLUT.h>
#elif defined(__linux__)
#   include <GL/glut.h>
#endif


// ----- MACROS -----

#ifdef GS_HIGH_PRECISION_FLOAT
#   define glLoadMatrix_T(p) glLoadMatrixd(p)
#else
#   define glLoadMatrix_T(p) glLoadMatrixf(p)
#endif

#define DEG_TO_RAD(x) ((x)*Gs::pi/Gs::Real(180))

#define TEST_PROJECTION_MORPHING
#define TEST_MESH_CLIPPING
#define TEST_SHOW_SPLIT
//#define TEST_TRIANGLE_NEIGHBORS
//#define TEST_SHOW_EDGES
//#define TEST_BLOATED_CUBE


// ----- STRUCTURES -----

struct Model
{
    Gm::TriangleMesh    mesh;
    Gm::Transform3      transform;
    Gs::Vector4         color { 1, 1, 1, 1 };
};


// ----- VARIABLES -----

Gs::Vector2i            resolution(800, 600);
Gs::ProjectionMatrix4   projection;

Gm::Transform3          cameraTransform;
Gs::AffineMatrix4       viewMatrix;

Gs::Real                fov = Gs::Real(74.0);

std::vector<Model>      models;

bool                    projMorphing        = false;
bool                    projMorphingOrtho   = false;
bool                    wireframeMode       = false;
bool                    autoRotate          = true;
Gs::Real                rotateModel         = 0.0;
std::size_t             neighborSearchDepth = 1;

Gm::Spline2             spline;

Gm::BezierCurve2        bezierCurve;

Gm::Frustum             frustum;

const Gs::Real          epsilon             = Gs::Epsilon<Gs::Real>();

int                     showScene           = 0;


// ----- FUNCTIONS -----

Model* createCuboidModel(const Gm::MeshGenerator::CuboidDescriptor& desc)
{
    models.resize(models.size() + 1);
    auto mdl = &(models.back());

    #if 1
    mdl->mesh = Gm::MeshGenerator::GenerateCuboid(desc);
    #else
    mdl->mesh = Gm::MeshGenerator::GenerateCapsule({});
    #endif

    return mdl;
}

void updateProjection()
{
    int flags = Gs::ProjectionFlags::OpenGLPreset;

    if (resolution.y > resolution.x)
        flags |= Gs::ProjectionFlags::HorizontalFOV;

    #ifdef TEST_PROJECTION_MORPHING

    // setup perspective projection
    auto perspProj = Gs::ProjectionMatrix4::Perspective(
        static_cast<Gs::Real>(resolution.x) / resolution.y,
        0.1f,
        100.0f,
        DEG_TO_RAD(fov),
        flags
    );

    // setup orthogonal projection
    const auto orthoZoom = Gs::Real(0.005);
    auto orthoProj = Gs::ProjectionMatrix4::Orthogonal(
        static_cast<Gs::Real>(resolution.x) * orthoZoom,
        static_cast<Gs::Real>(resolution.y) * orthoZoom,
        0.1f,
        100.0f,
        flags
    );

    // update morphing animation
    static Gs::Real morphing;

    if (projMorphing)
    {
        const auto speed = Gs::Real(0.1);

        if (projMorphingOrtho)
        {
            morphing += speed;

            if (morphing >= 1.0f - epsilon)
            {
                morphing = 1.0f;
                projMorphing = false;
            }
        }
        else
        {
            morphing -= speed;

            if (morphing <= 0.0f + epsilon)
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
        flags
    );

    #endif
}

void initGL()
{
    // setup GL configuration
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // initialize projection
    updateProjection();

    // create model
    Gm::MeshGenerator::CuboidDescriptor mdlDesc;

    #ifdef TEST_BLOATED_CUBE

    mdlDesc.segments = { 5, 5, 5 };
    mdlDesc.size = { 1, 1, 1 };
    auto mdl = createCuboidModel(mdlDesc);

    for (auto& v : mdl->mesh.vertices)
        v.position = Gs::Lerp(v.position, v.position.Normalized(), 0.5f);

    #else

    mdlDesc.segments = { 1, 2, 3 };
    mdlDesc.size = { 1, 1.5f, 0.5f };
    auto mdl = createCuboidModel(mdlDesc);

    #endif

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

    bezierCurve.controlPoints.push_back({ 100, 450 });
    bezierCurve.controlPoints.push_back({ 250, 150 });
    bezierCurve.controlPoints.push_back({ 600, 350 });
    bezierCurve.controlPoints.push_back({ 400, 100 });
}

void emitVertex(const Gm::TriangleMesh::Vertex& vert, const Gs::Vector4& color)
{
    // emit vertex data
    glNormal3fv(vert.normal.Ptr());
    glTexCoord2fv(vert.texCoord.Ptr());
    glColor4fv(color.Ptr());
    glVertex3fv(vert.position.Ptr());
}

void emitVertex(const Gm::TriangleMesh::Vertex& vert)
{
    // generate color from vertex data
    Gs::Vector4 color(vert.texCoord.x, vert.texCoord.y, 0.5f, 1);
    emitVertex(vert, color);
}

void drawMesh(const Gm::TriangleMesh& mesh, bool wireframe = false)
{
    #ifdef TEST_TRIANGLE_NEIGHBORS
    auto neighbors = mesh.TriangleNeighbors({ 4, 5 }, neighborSearchDepth, false, true);
    #endif

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    glBegin(GL_TRIANGLES);

    for (std::size_t i = 0; i < mesh.triangles.size(); ++i)
    {
        const auto& tri = mesh.triangles[i];

        const auto& v0 = mesh.vertices[tri.a];
        const auto& v1 = mesh.vertices[tri.b];
        const auto& v2 = mesh.vertices[tri.c];

        #ifdef TEST_TRIANGLE_NEIGHBORS

        // Highlight test triangle and its neighbors
        int c = 0;

        for (auto t : neighbors)
        {
            if (t == i)
            {
                c = 2;
                break;
            }
        }

        Gs::Vector4 colors[] = { Gs::Vector4(1, 0, 0, 1), Gs::Vector4(0, 1, 0, 1), Gs::Vector4(0, 0, 1, 1) };

        emitVertex(v0, colors[c]);
        emitVertex(v1, colors[c]);
        emitVertex(v2, colors[c]);

        #else

        emitVertex(v0);
        emitVertex(v1);
        emitVertex(v2);

        #endif
    }

    glEnd();
}

void drawMeshEdges(const Gm::TriangleMesh& mesh)
{
    auto edges = mesh.SilhouetteEdges(Gs::pi*0.01f);

    const auto color = Gs::Vector4(1, 1, 0, 1);

    glLineWidth(5);

    glBegin(GL_LINES);
    {
        for (const auto& edge : edges)
        {
            const auto& v0 = mesh.vertices[edge.a];
            const auto& v1 = mesh.vertices[edge.b];

            emitVertex(v0, color);
            emitVertex(v1, color);
        }
    }
    glEnd();

    glLineWidth(1);
}

void drawModel(const Model& mdl)
{
    // setup world-view matrix
    auto modelView = (viewMatrix * mdl.transform.GetMatrix()).ToMatrix4();
    glLoadMatrix_T(modelView.Ptr());

    #ifdef TEST_MESH_CLIPPING

    Gm::Plane clipPlane(
        Gs::Vector3(1, 0, 0).Normalized(),
        -0.3f
    );
    clipPlane = Gm::TransformPlane(modelView.Inverse(), clipPlane);

    Gm::TriangleMesh front, back;
    Gm::MeshModifier::ClipMesh(mdl.mesh, clipPlane, front, back);

    drawMesh(front, wireframeMode);

    #ifdef TEST_SHOW_SPLIT
    auto trans2 = mdl.transform;
    trans2.MoveGlobal({ -0.1f, 0, 0 });
    modelView = (viewMatrix * trans2.GetMatrix()).ToMatrix4();
    glLoadMatrix_T(modelView.Ptr());
    #endif

    drawMesh(back, wireframeMode);

    #else

    // draw model
    drawMesh(mdl.mesh, wireframeMode);

    #endif

    #ifdef TEST_SHOW_EDGES

    // draw edges
    drawMeshEdges(mdl.mesh);

    #endif
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

    for (const auto& edge : box.Edges())
        drawLine(edge.a, edge.b);

    glEnd();
}

void drawSpline(const Gm::Spline2& spline, Gs::Real a, Gs::Real b, std::size_t segments)
{
    // draw curve
    glBegin(GL_LINE_STRIP);

    auto step = (b - a) / segments;

    for (std::size_t i = 0; i <= segments; ++i)
    {
        glColor4f(1, 1, 1, 1);

        // interpolate vertex
        auto p = spline(a);
        glVertex2fv(p.Ptr());
        a += step;
    }

    glEnd();

    // draw control points
    glPointSize(5.0f);

    glBegin(GL_POINTS);

    for (const auto& p : spline.GetPoints())
    {
        glColor4f(1, 0, 0, 1);
        glVertex2fv(p.point.Ptr());
    }

    glEnd();

    glPointSize(1.0f);
}

void drawBezierCurve(const Gm::BezierCurve2& curve, std::size_t segments)
{
    // draw curve
    glBegin(GL_LINE_STRIP);

    auto t = Gs::Real(0);
    auto step = Gs::Real(1) / segments;

    for (std::size_t i = 0; i <= segments; ++i)
    {
        glColor4f(1, 1, 1, 1);

        // interpolate vertex
        auto p = curve(t);
        glVertex2fv(p.Ptr());
        t += step;
    }

    glEnd();

    // draw control points
    glPointSize(5.0f);

    glBegin(GL_POINTS);

    for (const auto& p : curve.controlPoints)
    {
        glColor4f(1, 1, 0, 1);
        glVertex2fv(p.Ptr());
    }

    glEnd();

    glPointSize(1.0f);
}

void updateScene()
{
    const auto motion = (autoRotate ? Gs::Real(0.002) : rotateModel);

    auto& trans = models[0].transform;

    auto rotation = trans.GetRotation().ToMatrix3();
    Gs::RotateFree(rotation, Gs::Vector3(1, 1, 1).Normalized(), Gs::pi*motion);
    trans.SetRotation(Gs::Quaternion(rotation));
}

void updateFrustum()
{
    // build view frustum from projection and view matrix
    frustum.SetFromMatrix(
        Gs::ProjectionMatrix4::Perspective(
            static_cast<Gs::Real>(resolution.x) / resolution.y,
            0.1f,
            100.0f,
            DEG_TO_RAD(fov)
        ).ToMatrix4()// * viewMatrix.ToMatrix4()
    );
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

    // update view frustum
    updateFrustum();

    // draw models
    for (const auto& mdl : models)
    {
        drawModel(mdl);
        drawAABB(mdl.mesh.BoundingBox(mdl.transform.GetMatrix()));
    }
}

void drawScene2D()
{
    // setup projection
    glMatrixMode(GL_PROJECTION);
	auto res = resolution.Cast<Gs::Real>();
    auto proj = Gs::ProjectionMatrix4::Planar(res.x, res.y);
    glLoadMatrix_T(proj.Ptr());

    // setup model-view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // draw scene
    drawSpline(spline, 0.0f, 6.0f, 500);
    drawBezierCurve(bezierCurve, 100);
}

void displayCallback()
{
    // update scene
    updateScene();

    // draw frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    {
        switch (showScene)
        {
            case 0:
                drawScene3D();
                break;
            case 1:
                drawScene2D();
                break;
        }
    }
    glutSwapBuffers();

    // reset states
    rotateModel = 0.0;
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

    displayCallback();
}

void keyboardCallback(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27: // ESC
            exit(0);
            break;

        case '\t': // TAB
            wireframeMode = !wireframeMode;
            break;

        case '\r': // ENTER
            projMorphing = true;
            projMorphingOrtho = !projMorphingOrtho;
            break;

        case '1':
            showScene = 0;
            break;

        case '2':
            showScene = 1;
            break;

        case '3':
            autoRotate = !autoRotate;
            break;

        case '4':
            rotateModel = 0.1f;
            break;

        case '5':
            rotateModel = -0.1f;
            break;

        case '6':
            if (neighborSearchDepth > 0)
                --neighborSearchDepth;
            break;

        case '7':
            if (neighborSearchDepth < 8)
                ++neighborSearchDepth;
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
    std::cout << "Press 1 to show 3D scene" << std::endl;
    std::cout << "Press 2 to show 2D scene" << std::endl;
    std::cout << "Press Enter to switch between perspective and orthographic view" << std::endl;
    std::cout << "Press Tab to switch between solid and wireframe mode" << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

    auto sx = glutGet(GLUT_SCREEN_WIDTH);
    auto sy = glutGet(GLUT_SCREEN_HEIGHT);

    glutInitWindowSize(resolution.x, resolution.y);
    glutInitWindowPosition(sx/2 - resolution.x/2, sy/2 - resolution.y/2);
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

