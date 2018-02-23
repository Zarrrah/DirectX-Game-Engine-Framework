#include "Framework.h"

Framework * _thisFramework = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

Framework::Framework(unsigned int width, unsigned int height)
{
	_thisFramework = this;
	_width = width;
	_height = height;
	_eyePosition = XMFLOAT4(0.0f, 1.0f, -15.0f, 0.0f);
	_focalPointPosition = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	_upVector = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	//Only run after the constructor has been called
	if (_thisFramework)
	{
		return _thisFramework->Run(hInstance, nCmdShow);
	}
	return -1;
}

int Framework::Run(HINSTANCE hInstance, int nCmdShow)
{
	int returnValue;

	_hInstance = hInstance;
	if (!InitialiseMainWindow(nCmdShow))
	{
		return -1;
	}
	returnValue = MainLoop();
	Shutdown();
	return returnValue;
}

bool Framework::InitialiseMainWindow(int nCmdShow)
{
#define MAX_LOADSTRING 100

	WCHAR windowTitle[MAX_LOADSTRING];
	WCHAR windowClass[MAX_LOADSTRING];

	LoadStringW(_hInstance, IDS_APP_TITLE, windowTitle, MAX_LOADSTRING);
	LoadStringW(_hInstance, IDC_GRAPHICS2, windowClass, MAX_LOADSTRING);

	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = _hInstance;
	wcex.hIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_GRAPHICS2));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = windowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	if (!RegisterClassExW(&wcex))
	{
		MessageBox(0, L"Unable to register window class", 0, 0);
		return false;
	}

	// Now work out how large the window needs to be for our required client window size
	RECT windowRect = { 0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	_width = windowRect.right - windowRect.left;
	_height = windowRect.bottom - windowRect.top;

	_hWnd = CreateWindowW(windowClass,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, _width, _height,
		nullptr, nullptr, _hInstance, nullptr);
	if (!_hWnd)
	{
		MessageBox(0, L"Unable to create window", 0, 0);
		return false;
	}
	if (!Initialise())
	{
		return false;
	}
	ShowWindow(_hWnd, nCmdShow);
	UpdateWindow(_hWnd);
	return true;
}

bool Framework::Initialise() {
	// The call to CoInitializeEx is needed if we are using textures since the WIC library used requires it, so we take care of initialising it here
	if FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
		return false;
	if (!CreateDeviceAndSwapChain())
		return false;

	//Calls the resize method to render the window
	OnResize(WM_EXITSIZEMOVE);

	//Creates scenegraph
	_sceneGraph = make_shared<SceneGraph>();
	CreateSceneGraph();
	_sceneGraph->Initialise();

	return true;
} 


void Framework::Update()
{
	UpdateSceneGraph();
	_sceneGraph->Update(XMMatrixIdentity());
}

void Framework::Render()
{
	const float clearColour[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	_deviceContext->ClearRenderTargetView(_renderTargetView.Get(), clearColour);
	_deviceContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	_sceneGraph->Render();
	ThrowIfFailed(_swapChain->Present(0, 0));
}

void Framework::Shutdown()
{
	_sceneGraph->Shutdown();
	CoUninitialize();
}


int Framework::MainLoop()
{
	MSG msg;
	HACCEL hAccelTable = LoadAccelerators(_hInstance, MAKEINTRESOURCE(IDC_GRAPHICS2));
	LARGE_INTEGER counterFrequency;
	LARGE_INTEGER nextTime;
	LARGE_INTEGER currentTime;
	LARGE_INTEGER lastTime;
	bool updateFlag = true;

	// Initialise timer
	QueryPerformanceFrequency(&counterFrequency);
	DWORD msPerFrame = (DWORD)(counterFrequency.QuadPart / DEFAULT_FRAMERATE);
	double timeFactor = 1.0 / counterFrequency.QuadPart;
	QueryPerformanceCounter(&nextTime);
	lastTime = nextTime;

	// Main message loop:
	msg.message = WM_NULL;
	while (msg.message != WM_QUIT)
	{
		if (updateFlag)
		{
			QueryPerformanceCounter(&currentTime);
			_timeSpan = (currentTime.QuadPart - lastTime.QuadPart) * timeFactor;
			lastTime = currentTime;
			Update();
			updateFlag = false;
		}
		QueryPerformanceCounter(&currentTime);
		// Is it time to render the frame?
		if (currentTime.QuadPart > nextTime.QuadPart)
		{
			Render();
			// Set time for next frame
			nextTime.QuadPart += msPerFrame;
			// If we get more than a frame ahead, allow one to be dropped
			// Otherwise, we will never catch up if we let the error accumulate
			// and message handling will suffer
			if (nextTime.QuadPart < currentTime.QuadPart)
			{
				nextTime.QuadPart = currentTime.QuadPart + msPerFrame;
			}
			updateFlag = true;
		}
		else
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (_thisFramework != nullptr)
	{
		// If framework is started, then we can call our own message proc
		return _thisFramework->MsgProc(hWnd, message, wParam, lParam);
	}
	else
	{
		// otherwise, we just pass control to the default message proc
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

LRESULT Framework::MsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
			break;

		case WM_KEYDOWN:
			OnKeyDown(wParam);
			return 0;

		case WM_KEYUP:
			OnKeyUp(wParam);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_SIZE:
			_width = LOWORD(lParam);
			_height = HIWORD(lParam);
			OnResize(wParam);
			Render();
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


Framework * Framework::GetFramework()
{
	return nullptr;
}

XMMATRIX Framework::GetViewTransformation()
{
	return XMLoadFloat4x4(&_viewTransformation);
}

XMMATRIX Framework::GetProjectionTransformation()
{
	return XMLoadFloat4x4(&_projectionTransformation);
}


void Framework::OnResize(WPARAM wParam)
{
	//Update view and projection matrices to allow for the window size change
	XMStoreFloat4x4(&_viewTransformation, XMMatrixLookAtLH(XMLoadFloat4(&_eyePosition), XMLoadFloat4(&_focalPointPosition), XMLoadFloat4(&_upVector)));
	XMStoreFloat4x4(&_projectionTransformation, XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)GetWindowWidth() / GetWindowHeight(), 1.0f, 100.0f));

	//We only want to resize the buffers when the user has finished dragging the window 
	if (wParam != WM_EXITSIZEMOVE)
	{
		return;
	}
	//Free any existing render and depth views
	_renderTargetView = nullptr;
	_depthStencilView = nullptr;
	_depthStencilBuffer = nullptr;

	ThrowIfFailed(_swapChain->ResizeBuffers(1, GetWindowWidth(), GetWindowHeight(), DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	// Create a drawing surface for DirectX to render to
	ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
	ThrowIfFailed(_device->CreateRenderTargetView(backBuffer.Get(), NULL, _renderTargetView.GetAddressOf()));

	//Create the depth buffer
	D3D11_TEXTURE2D_DESC depthBufferTexture = CreateDepthBuggerTexture();
	ComPtr<ID3D11Texture2D> depthBuffer;
	ThrowIfFailed(_device->CreateTexture2D(&depthBufferTexture, NULL, depthBuffer.GetAddressOf()));
	ThrowIfFailed(_device->CreateDepthStencilView(depthBuffer.Get(), 0, _depthStencilView.GetAddressOf()));

	// Set the render target view buffer and the depth stencil view buffer to the output-merger
	_deviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilView.Get());

	// Specify a viewport of the required size
	D3D11_VIEWPORT viewPort = CreateViewPort();
	_deviceContext->RSSetViewports(1, &viewPort);
}


bool Framework::CreateDeviceAndSwapChain()
{
	UINT createDeviceFlags = 0;

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	unsigned int totalFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP
	};
	unsigned int totalDriverTypes = ARRAYSIZE(driverTypes);

	DXGI_SWAP_CHAIN_DESC swapChainDesc = CreateSwapChainDesc();

	D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;
	for (unsigned int driver = 0; driver < totalDriverTypes && driverType == D3D_DRIVER_TYPE_UNKNOWN; driver++)
		if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(0, driverTypes[driver], 0, createDeviceFlags, featureLevels, totalFeatureLevels, D3D11_SDK_VERSION, &swapChainDesc, _swapChain.GetAddressOf(), _device.GetAddressOf(), 0, _deviceContext.GetAddressOf())))
			driverType = driverTypes[driver];
	if (driverType == D3D_DRIVER_TYPE_UNKNOWN)
		return false;
	return true;
}

D3D11_TEXTURE2D_DESC Framework::CreateDepthBuggerTexture() 
{
	D3D11_TEXTURE2D_DESC depthBufferTexture = { 0 };
	depthBufferTexture.Width = GetWindowWidth();
	depthBufferTexture.Height = GetWindowHeight();
	depthBufferTexture.ArraySize = 1;
	depthBufferTexture.MipLevels = 1;
	depthBufferTexture.SampleDesc.Count = 4;
	depthBufferTexture.Format = DXGI_FORMAT_D32_FLOAT;
	depthBufferTexture.Usage = D3D11_USAGE_DEFAULT;
	depthBufferTexture.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	return depthBufferTexture;
}

D3D11_VIEWPORT Framework::CreateViewPort() {
	D3D11_VIEWPORT viewPort;
	viewPort.Width = static_cast<float>(GetWindowWidth());
	viewPort.Height = static_cast<float>(GetWindowHeight());
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	return viewPort;
}

DXGI_SWAP_CHAIN_DESC Framework::CreateSwapChainDesc() 
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = GetWindowWidth();
	swapChainDesc.BufferDesc.Height = GetWindowHeight();
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetHWnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SampleDesc.Count = 4;
	swapChainDesc.SampleDesc.Quality = 0;
	return swapChainDesc;
}