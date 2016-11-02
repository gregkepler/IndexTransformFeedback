#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int POINT_NUM			= 100;
const uint32_t POSITION_INDEX		= 1;
const uint32_t VELOCITY_INDEX			= 2;
const uint32_t LINE_INDEX			= 3;

class IndexTransformFeedbackApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
	void initGlsl();
	void initBuffers();
	
	gl::GlslProgRef	mLineRenderGlsl, mLineUpdateGlsl, mPointGlsl, mPointUpdateGlsl;
	gl::VboRef		mPositions[2], mVelocities[2], mLineIndices[2];
	gl::BatchRef	mPointBatch[2], mLineBatch[2];
	gl::VboMeshRef	mLineVboMesh[2];
	gl::VaoRef		mVao[2], mLineVao[2];
	
	std::uint32_t	mSourceIndex		= 0;
	std::uint32_t	mDestinationIndex	= 1;
	
	CameraPersp			mCam;
	CameraUi			mCamUi;
};

void IndexTransformFeedbackApp::setup()
{
	initGlsl();
	initBuffers();
	
	// create a camera
	mCam = CameraPersp( getWindowWidth(), getWindowHeight(), 60, 0.1f, 10000.0f );
	mCam.lookAt( vec3( 0, 0, 1000.0f ), vec3() );
	mCamUi = CameraUi( &mCam );
}

void IndexTransformFeedbackApp::initGlsl()
{
	{
		// line render shader
		gl::GlslProg::Format renderFormat;
		renderFormat.vertex( loadAsset( "lineRender.vert" ) );
		renderFormat.fragment( loadAsset( "lineRender.frag" ) );
//		renderFormat.attribLocation( "vPosition", POSITION_INDEX );
		mLineRenderGlsl = gl::GlslProg::create( renderFormat );
	}
	
	
	{
		// line update shader
		std::vector<std::string> feedbackVaryings = {
			"tf_index"
		};
		
		gl::GlslProg::Format updateFmt;
		updateFmt.vertex( loadAsset( "lineUpdate.vert" ) )
		.feedbackFormat( GL_SEPARATE_ATTRIBS )
		.feedbackVaryings( feedbackVaryings )
		.attribLocation( "vIndex",	0 );
		
		mLineUpdateGlsl = gl::GlslProg::create( updateFmt );
		mLineUpdateGlsl->uniform( "uParticleCount", POINT_NUM );
		mLineUpdateGlsl->uniform( "uStride", POINT_NUM );
		mLineUpdateGlsl->uniform( "uMaxDist", 200.0f );
		
	}
	
	// point update shader
	{
		std::vector<std::string> feedbackVaryings = {
			"tf_position"//, "tf_boxindex"
		};
		
		gl::GlslProg::Format updateFmt;
		updateFmt.vertex( loadAsset( "update.vert" ) )
		// This option will be either GL_SEPARATE_ATTRIBS or GL_INTERLEAVED_ATTRIBS,
		// depending on the structure of our data, below. We're using multiple
		// buffers. Therefore, we're using GL_SEPERATE_ATTRIBS
		.feedbackFormat( GL_SEPARATE_ATTRIBS )
		.feedbackVaryings( feedbackVaryings )
		.attribLocation( "vPosition",	POSITION_INDEX );
//		.attribLocation( "vVelocity",	VELOCITY_INDEX );
		//	.attribLocation( "vBoxIndex",	GRID_POINT_INDEX );
		
		mPointUpdateGlsl = gl::GlslProg::create( updateFmt );
	}
	
	{
		// point shader
		gl::GlslProg::Format format;
		format.vertex( loadAsset( "pointRender.vert" ) );
		format.fragment( loadAsset( "pointRender.frag" ) );
		format.attribLocation( "vPosition", POSITION_INDEX );
		mPointGlsl = gl::GlslProg::create( format );
	}
}

void IndexTransformFeedbackApp::initBuffers()
{
	vector<vec4> positions;
	for( int i = 0; i < POINT_NUM; i++ ){
		positions.push_back( vec4( randFloat( -0.5, 0.5 ) * getWindowWidth(), randFloat( -0.5, 0.5) * getWindowHeight(), randFloat( -0.5, 0.5) * 500, 1 ) );
	}
	
	int lineCount = POINT_NUM * POINT_NUM * 2;
	vector<int> lineIndices( lineCount, -1 );
	int index = 0;
	for( int i = 0; i < POINT_NUM; i++ ){
		for( int j = 0; j < POINT_NUM; j++){
			vec4 pt1 = positions[i];
			vec4 pt2 = positions[j];
			
			if( pt1 != pt2 && distance( pt1, pt2 ) < 100.0f ){
				lineIndices[index] = i;
				lineIndices[index+1] = j;
				CI_LOG_V( "connect: " << i << " TO " << j );
			}
			index += 2;
		}
	}
	
	
	// make velocities ---------
	vector<ci::vec3> velocities;
	for( int i = 0; i < positions.size(); i++ ){
		velocities.push_back( vec3( randFloat( -0.5f, 0.5f ), randFloat( -0.5f, 0.5f ), 0.0f ) );
		//		velocities.push_back( vec3() );
	}
	
	
	// Create Position Vbo with the initial position data
	mPositions[mSourceIndex] = ci::gl::Vbo::create( GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_STATIC_DRAW );
	// Create another Position Buffer that is null, for ping-ponging
	mPositions[mDestinationIndex] = ci::gl::Vbo::create( GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), nullptr, GL_STATIC_DRAW );
	
	mVelocities[mSourceIndex] = ci::gl::Vbo::create( GL_ARRAY_BUFFER, velocities.size() * sizeof(vec3), velocities.data(), GL_STATIC_DRAW );
	mVelocities[mDestinationIndex] = ci::gl::Vbo::create( GL_ARRAY_BUFFER, velocities.size() * sizeof(vec3), nullptr, GL_STATIC_DRAW );
	
	// VBO for storing line indices
	mLineIndices[mSourceIndex] = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, lineCount * sizeof(int), lineIndices.data(), GL_STATIC_DRAW );
	mLineIndices[mDestinationIndex] = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, lineCount * sizeof(int), nullptr, GL_STATIC_DRAW );
	
	
	for( int i = 0; i < 2; i++ ) {
		{
			// Initialize the Vao's holding the info for each buffer
			mVao[i] = ci::gl::Vao::create();
			// Bind the vao to capture index data for the glsl
			gl::ScopedVao vao( mVao[i] );
			
			
			{
				gl::ScopedBuffer sccopeBuffer( mPositions[i] );
				ci::gl::enableVertexAttribArray( POSITION_INDEX );
				ci::gl::vertexAttribPointer( POSITION_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(vec4),  (const GLvoid*) 0 );
			}
			
			{
				gl::ScopedBuffer sccopeBuffer( mVelocities[i] );
				ci::gl::vertexAttribPointer( VELOCITY_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (const GLvoid*) 0 );
				ci::gl::enableVertexAttribArray( VELOCITY_INDEX );
			}
		}
	}
	
	
	// Describe the shader attributes.
	geom::BufferLayout instanceDataLayout;
	instanceDataLayout.append( geom::Attrib::CUSTOM_1, 4, sizeof( vec4 ), 0, 1 /* per instance */ );
	
	{
		auto mesh = gl::VboMesh::create( geom::Sphere().radius( 5.0 ) );
		mesh->appendVbo( instanceDataLayout, mPositions[0] );
		mPointBatch[0] = gl::Batch::create( mesh, mPointGlsl, {
			{ geom::Attrib::CUSTOM_1, "vPosition" } } );
	}
	{
		auto mesh = gl::VboMesh::create( geom::Sphere().radius( 5.0 ) );
		mesh->appendVbo( instanceDataLayout, mPositions[1] );
		mPointBatch[1] = gl::Batch::create( mesh, mPointGlsl, {
			{ geom::Attrib::CUSTOM_1, "vPosition" } } );
	}
	
	
	geom::BufferLayout lineDataLayout;
	lineDataLayout.append( geom::Attrib::POSITION, 4, sizeof( vec4 ), 0 );
//	lineDataLayout.append( geom::Attrib::CUSTOM_1, 1, sizeof( int32_t ), 0, 1 /* per instance */ );
	
//	auto mIndexData = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, lineIndices, GL_STATIC_DRAW );
	mLineVboMesh[mSourceIndex]		= gl::VboMesh::create( lineIndices.size(), GL_LINES, { { lineDataLayout, mPositions[mSourceIndex] } }, lineIndices.size(), GL_UNSIGNED_INT, mLineIndices[0] );
	mLineVboMesh[mDestinationIndex] = gl::VboMesh::create( lineIndices.size(), GL_LINES, { { lineDataLayout, mPositions[mDestinationIndex] } }, lineIndices.size(), GL_UNSIGNED_INT, mLineIndices[1] );
	
	mLineBatch[0] = gl::Batch::create( mLineVboMesh[0], mLineRenderGlsl );
	mLineBatch[1] = gl::Batch::create( mLineVboMesh[1], mLineRenderGlsl );
	
	for( int i = 0; i < 2; i++ ) {
	{
		mLineVao[i] = ci::gl::Vao::create();
		gl::ScopedVao vao( mLineVao[i] );
		
		{
		 gl::ScopedBuffer sccopeBuffer( mLineIndices[i] );
		 ci::gl::vertexAttribPointer( LINE_INDEX, 1, GL_INT, GL_FALSE, 0,  (const GLvoid*) 0 );
		 ci::gl::enableVertexAttribArray( LINE_INDEX );
		}
	}
	}

}

void IndexTransformFeedbackApp::mouseDown( MouseEvent event )
{
	mCamUi.mouseDown( event );
}

void IndexTransformFeedbackApp::mouseDrag( MouseEvent event )
{
	mCamUi.mouseDrag( event );
}

void IndexTransformFeedbackApp::update()
{
	gl::ScopedState		stateScope( GL_RASTERIZER_DISCARD, true );
	
	
	// UPDATE POSITIONS --------------------------------------------------------
	{
		gl::ScopedGlslProg	glslScope( mPointUpdateGlsl );
		gl::ScopedVao		vaoScope( mVao[mSourceIndex] );
		
		gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mPositions[mDestinationIndex] );
		gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 1, mVelocities[mDestinationIndex] );
		
		// We begin Transform Feedback, using the same primitive that
		// we're "drawing". Using points for the particle system.
		gl::beginTransformFeedback( GL_POINTS );
		gl::drawArrays( GL_POINTS, 0, POINT_NUM );
		gl::endTransformFeedback();
		//		mPositionTexture = gl::BufferTexture::create( mPositions[mDestinationIndex], GL_RGBA32F );
	}
	
	
	
	
	// UPDATE INDICES ----------------------------------------------------------
	if( mLineUpdateGlsl ){
		
		gl::ScopedGlslProg	glslScope( mLineUpdateGlsl );
		gl::ScopedVao		vaoScope( mLineVao[mSourceIndex] );
//		gl::ScopedBuffer	vboScope( mLineIndices[mSourceIndex] );
		
		gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mLineIndices[mDestinationIndex] );
		
		gl::beginTransformFeedback( GL_POINTS );
		gl::drawArrays( GL_POINTS, 0, POINT_NUM * POINT_NUM );
		
		gl::endTransformFeedback();
	}
	
	std::swap( mSourceIndex, mDestinationIndex );
	
}



void IndexTransformFeedbackApp::draw()
{
	gl::ScopedMatrices scpMatrx;
	gl::setMatrices( mCam );
// return;
	
	gl::clear( Color( 0, 0, 0 ) );
	{
		
		gl::ScopedBlendAdditive scpAdd;
		gl::enableAdditiveBlending();
		gl::setDefaultShaderVars();
		
		// draw points
		{
			mPointBatch[mSourceIndex]->drawInstanced( POINT_NUM );
		}
		
		// draw lines
		{
			mLineBatch[mSourceIndex]->draw();
		}
	}
}

CINDER_APP( IndexTransformFeedbackApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
