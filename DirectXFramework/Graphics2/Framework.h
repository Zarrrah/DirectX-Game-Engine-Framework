#pragma once
#include "Core.h"
#include <vector>
#include "Framework.h"
#include "DirectXCore.h"
#include "SceneGraph.h"

// DirectX libraries that are needed
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define DEFAULT_FRAMERATE	60
#define DEFAULT_WIDTH		1920
#define DEFAULT_HEIGHT		1080

using namespace std;

class Framework
{
public:

	inline Framework() : Framework(DEFAULT_WIDTH, DEFAULT_HEIGHT) {}
	Framework(unsigned int width, unsigned int height);
	inline virtual ~Framework() {}

	int Run(HINSTANCE hInstance, int nCmdShow);

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	inline unsigned int GetWindowWidth() { return _width; }
	inline unsigned int GetWindowHeight() { return _height; }
	inline HWND GetHWnd() {	return _hWnd; }

	bool Initialise();
	void Update();
	void Render();
	void Shutdown();

	static Framework * GetFramework();

	inline SceneGraphPointer			GetSceneGraph() { return _sceneGraph; }
	inline ComPtr<ID3D11Device>			GetDevice() { return _device; }
	inline ComPtr<ID3D11DeviceContext>	GetDeviceContext() { return _deviceContext; }

	XMMATRIX							GetViewTransformation();
	XMMATRIX							GetProjectionTransformation();

	inline virtual void OnKeyDown(WPARAM wParam) {}
	inline virtual void OnKeyUp(WPARAM wParam) {}
	virtual void OnResize(WPARAM wParam);


private:
	HINSTANCE		_hInstance;
	HWND			_hWnd;
	unsigned int	_width;
	unsigned int	_height;

	double			_timeSpan;

	bool InitialiseMainWindow(int nCmdShow);
	int MainLoop();

	D3D11_TEXTURE2D_DESC			CreateDepthBuggerTexture();
	D3D11_VIEWPORT					CreateViewPort();
	DXGI_SWAP_CHAIN_DESC			CreateSwapChainDesc();
	bool							CreateDeviceAndSwapChain();

	ComPtr<ID3D11Device>			_device;
	ComPtr<ID3D11DeviceContext>		_deviceContext;
	ComPtr<IDXGISwapChain>			_swapChain;
	ComPtr<ID3D11Texture2D>			_depthStencilBuffer;
	ComPtr<ID3D11RenderTargetView>	_renderTargetView;
	ComPtr<ID3D11DepthStencilView>	_depthStencilView;

	D3D11_VIEWPORT					_screenViewport;

	XMFLOAT4						_eyePosition;
	XMFLOAT4						_focalPointPosition;
	XMFLOAT4						_upVector;

	XMFLOAT4X4						_viewTransformation;
	XMFLOAT4X4						_projectionTransformation;

	SceneGraphPointer				_sceneGraph;
protected:
	virtual void					UpdateSceneGraph() = 0;
	virtual void					CreateSceneGraph() = 0;
};

