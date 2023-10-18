#include <QMouseEvent>
#include "NGLScene.h"
#include <iostream>
#include <ngl/Transformation.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/VAOFactory.h>
#include <ngl/SimpleVAO.h>
#include <ngl/ShaderLib.h>

//----------------------------------------------------------------------------------------------------------------------
NGLScene::NGLScene(int _timer, QWidget *_parent) : QOpenGLWidget(_parent)
{
  // set this widget to have the initial keyboard focus
  setFocus();
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  this->resize(_parent->size());
  m_timerValue = _timer;
  startSimTimer();
  m_timeractive = false;
}

// This virtual function is called once before the first call to paintGL() or resizeGL(),
// and then once whenever the widget has been assigned a new QGLContext.
// This function should set up any required OpenGL context rendering flags, defining display lists, etc.

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::initializeGL()
{
  ngl::NGLInit::initialize();
  glClearColor(0.27f, 0.31f, 0.38f, 1.0f); // blue-grey background 
                                        // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0, 0, 7);
  ngl::Vec3 to(0, 0, 0);
  ngl::Vec3 up(0, 1, 0);

  m_view = ngl::lookAt(from, to, up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project = ngl::perspective(45.0f, 720.0f / 576.0f, 0.5f, 150.0f);

  ngl::VAOPrimitives::createSphere("sphere", 1.0, 20);
  ngl::ShaderLib::use(ngl::nglDiffuseShader);
  ngl::ShaderLib::setUniform("Colour", 1.0f, 1.0f, 0.0f, 1.0f);
  ngl::ShaderLib::setUniform("lightPos", 1.0f, 1.0f, 1.0f);
  ngl::ShaderLib::setUniform("lightDiffuse", 1.0f, 1.0f, 1.0f, 1.0f);
}

//----------------------------------------------------------------------------------------------------------------------
// This virtual function is called whenever the widget has been resized.
// The new size is passed in width and height.
void NGLScene::resizeGL(int _w, int _h)
{
  m_project = ngl::perspective(45.0f, static_cast<float>(_w) / _h, 0.05f, 350.0f);
  m_win.width = static_cast<int>(_w * devicePixelRatio());
  m_win.height = static_cast<int>(_h * devicePixelRatio());
}

void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib::use(ngl::nglDiffuseShader);
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M = m_transform.getMatrix();
  MV = m_view * m_mouseGlobalTX * M;
  MVP = m_project * MV;
  normalMatrix = MV;
  normalMatrix.inverse().transpose();
  ngl::ShaderLib::setUniform("MVP", MVP);
  ngl::ShaderLib::setUniform("normalMatrix", normalMatrix);
}

void NGLScene::loadMatricesToColourShader()
{
  ngl::ShaderLib::use(ngl::nglColourShader);
  ngl::Mat4 MVP;
  MVP = m_project * m_view *
        m_mouseGlobalTX *
        m_transform.getMatrix();
  ngl::ShaderLib::setUniform("MVP", MVP);
}

//----------------------------------------------------------------------------------------------------------------------
// This virtual function is called whenever the widget needs to be painted.
// this is our main drawing routine
void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Rotation based on the mouse position for our global
  // transform
  // Rotation based on the mouse position for our global transform
  auto rotX = ngl::Mat4::rotateX(m_win.spinXFace);
  auto rotY = ngl::Mat4::rotateY(m_win.spinYFace);
  // multiply the rotations
  m_mouseGlobalTX = rotY * rotX;

  if (m_mesh->getCollisionObj().m_activeObj[0])
  {
      ngl::ShaderLib::use(ngl::nglDiffuseShader);
      ngl::VAOPrimitives::createTrianglePlane("plane", 14, 14, 80, 80, ngl::Vec3(0, 1, 0));
      ngl::ShaderLib::setUniform("Colour", 0.18f, 0.20f, 0.25f, 1.0f); //dark grey-blue 
      m_transform.setPosition(0.0f, m_mesh->getCollisionObj().m_planeY, 0.0f);
      loadMatricesToShader();
      ngl::VAOPrimitives::draw("plane");
  }

  if (m_mesh->getCollisionObj().m_activeObj[1])
  {
      ngl::ShaderLib::use(ngl::nglDiffuseShader);
      ngl::ShaderLib::setUniform("Colour", 0.45f, 0.45f, 0.38f, 1.0f); //muddy yellow
      m_transform.setScale(m_mesh->getCollisionObj().m_sphereR, m_mesh->getCollisionObj().m_sphereR, m_mesh->getCollisionObj().m_sphereR);
      m_transform.setPosition(m_mesh->getCollisionObj().m_spherePos);
      loadMatricesToShader();
      ngl::VAOPrimitives::draw("sphere");
  }

  if (m_mesh->getCollisionObj().m_activeObj[2])
  {
      ngl::ShaderLib::use(ngl::nglDiffuseShader);
      ngl::ShaderLib::setUniform("Colour", 0.45f, 0.45f, 0.38f, 1.0f); //muddy yellow
      m_transform.setScale(m_mesh->getCollisionObj().m_cubeA, m_mesh->getCollisionObj().m_cubeA, m_mesh->getCollisionObj().m_cubeA);
      m_transform.setPosition(m_mesh->getCollisionObj().m_cubePos);
      loadMatricesToShader();
      ngl::VAOPrimitives::draw("cube");
  }

  for (int i = 0; i < m_mesh->getHeight(); i++)
  {
      for (int j = 0; j < m_mesh->getWidth(); j++)
      {
          ngl::ShaderLib::use(ngl::nglColourShader);
          ngl::ShaderLib::setUniform("Colour", 0.73f, 0.75f, 0.81f, 1.0f); //cool white
          m_transform.setScale(0.05f, 0.05f, 0.05f);
          m_transform.setPosition(m_mesh->getMesh()[i][j].m_currentPos);
          loadMatricesToColourShader();
          ngl::VAOPrimitives::draw("sphere");
      }
  }

  for (int i=0; i< m_mesh->getStretchSprings().size(); i++)
  {
      ngl::ShaderLib::use(ngl::nglColourShader);
      // get our position values and put in a vector
      std::vector<ngl::Vec3> points(2);
      points[0] = m_mesh->getStretchSprings()[i].getAPosition();
      points[1] = m_mesh->getStretchSprings()[i].getBPosition();
      ngl::ShaderLib::setUniform("Colour", 0.62f, 0.75f, 0.54f, 1.0f); //light green
      m_transform.reset();
      loadMatricesToColourShader();
      // load transform stack
      std::unique_ptr<ngl::AbstractVAO> vao(ngl::VAOFactory::createVAO("simpleVAO", GL_LINES));
      vao->bind();
      vao->setData(ngl::AbstractVAO::VertexData(2 * sizeof(ngl::Vec3), points[0].m_x));
      vao->setNumIndices(2);
      vao->setVertexAttributePointer(0, 3, GL_FLOAT, 0, 0);
      vao->draw();
      vao->unbind();
  }

  for (int i = 0; i < m_mesh->getShearSprings().size(); i++)
  {
      ngl::ShaderLib::use(ngl::nglColourShader);
      // get our position values and put in a vector
      std::vector<ngl::Vec3> points(2);
      points[0] = m_mesh->getShearSprings()[i].getAPosition();
      points[1] = m_mesh->getShearSprings()[i].getBPosition();
      ngl::ShaderLib::setUniform("Colour", 0.67f, 0.54f, 0.68f, 1.0f); //light red
      m_transform.reset();
      loadMatricesToColourShader();
      // load transform stack
      std::unique_ptr<ngl::AbstractVAO> vao(ngl::VAOFactory::createVAO("simpleVAO", GL_LINES));
      vao->bind();
      vao->setData(ngl::AbstractVAO::VertexData(2 * sizeof(ngl::Vec3), points[0].m_x));
      vao->setNumIndices(2);
      vao->setVertexAttributePointer(0, 3, GL_FLOAT, 0, 0);
      vao->draw();
      vao->unbind();
  }
}


void NGLScene::timerEvent(QTimerEvent *)
{
  m_mesh->clothUpdate();
  update();
}

NGLScene::~NGLScene()
{
}

void NGLScene::startSimTimer()
{
    if (!m_timeractive) { m_timer = startTimer(m_timerValue); }
    m_timeractive = true;
}

void NGLScene::stopSimTimer()
{
    if (m_timeractive) { killTimer(m_timer); }
    m_timeractive = false;

}
