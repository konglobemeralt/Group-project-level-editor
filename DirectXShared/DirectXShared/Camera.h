#ifndef CAMERA_H
#define CAMERA_H

#include "Includes.h"

class Camera
{
public:

	Camera();
	~Camera();

	void Update();

	XMFLOAT4X4 viewMatrix;
	XMFLOAT4X4 projectionMatrix;

private:

	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
};

#endif