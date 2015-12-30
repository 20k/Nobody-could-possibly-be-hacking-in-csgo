#include <iostream>

#include <InitGuid.h>

#include <windows.h>
#include <gdiplus.h>
/*#include <d3d9.h>
#include <D3dx9tex.h>*/
#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI.h>
#include <dxgiformat.h>
#include <dxgitype.h>
#include <DXGI1_2.h>
#include <SFML/Graphics.hpp>
//#include <vec/vec.hpp>


#if 1
int dxgi_getDuplication(ID3D11Device* gDevice, IDXGIOutputDuplication** pgOutputDuplication)
{
	HRESULT status;
	UINT dTop = 1, i = 0;
	DXGI_OUTPUT_DESC desc;
	IDXGIOutput * pOutput;
	IDXGIDevice* DxgiDevice = NULL;
	IDXGIAdapter* DxgiAdapter = NULL;
	IDXGIOutput* DxgiOutput = NULL;
	IDXGIOutput1* DxgiOutput1 = NULL;

	status = gDevice->QueryInterface(IID_IDXGIDevice, (void**) &DxgiDevice);

	if (FAILED(status))
	{
		//WLog_ERR(TAG, "Failed to get QI for DXGI Device");
		printf("bum1\n");
		return 1;
	}

	status = DxgiDevice->GetParent(IID_IDXGIAdapter, (void**) &DxgiAdapter);
	DxgiDevice->Release();
	DxgiDevice = NULL;

	if (FAILED(status))
	{
		//WLog_ERR(TAG, "Failed to get parent DXGI Adapter");
		printf("bum2\n");
		return 1;
	}

	ZeroMemory(&desc, sizeof(desc));
	pOutput = NULL;

	while (DxgiAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC* pDesc = &desc;

		status = pOutput->GetDesc(pDesc);

		if (FAILED(status))
		{
			//WLog_ERR(TAG, "Failed to get description");
			printf("bum3\n");
			return 1;
		}

		//WLog_INFO(TAG, "Output %d: [%s] [%d]", i, pDesc->DeviceName, pDesc->AttachedToDesktop);
		printf("Output %d %s %d\n", i, pDesc->DeviceName, pDesc->AttachedToDesktop);

		if (pDesc->AttachedToDesktop)
        {
            printf("DTop %i\n", i);

            dTop = i;
        }

		pOutput->Release();
		++i;
	}

	//dTop = wfi->screenID;
	//dTop = 0; ///???

	status = DxgiAdapter->EnumOutputs(dTop, &DxgiOutput);
	DxgiAdapter->Release();
	DxgiAdapter = NULL;

	if (FAILED(status))
	{
		//WLog_ERR(TAG, "Failed to get output");
		printf("bum4\n");
		return 1;
	}

	status = DxgiOutput->QueryInterface(IID_IDXGIOutput1, (void**) &DxgiOutput1);
	DxgiOutput->Release();
	DxgiOutput = NULL;

	if (FAILED(status))
	{
		//WLog_ERR(TAG, "Failed to get IDXGIOutput1");
		printf("bum5\n");
		return 1;
	}

	status = DxgiOutput1->DuplicateOutput((IUnknown*)gDevice, pgOutputDuplication);
	DxgiOutput1->Release();
	DxgiOutput1 = NULL;

	if (FAILED(status))
	{
		if (status == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			//WLog_ERR(TAG, "There is already the maximum number of applications using the Desktop Duplication API running, please close one of those applications and then try again."));
			printf("bum6\n");
			return 1;
		}

		//WLog_ERR(TAG, "Failed to get duplicate output. Status = %#X", status);
		printf("bum7\n");
		return 1;
	}

	return 0;
}



///timeout in ms
int dxgi_nextFrame(UINT timeout, ID3D11Device* gDevice, ID3D11Texture2D** pgAcquiredDesktopImage, IDXGIOutputDuplication** pgOutputDuplication, IDXGIResource** pDesktopResource)
{
	HRESULT status = 0;
	UINT i = 0;
	UINT DataBufferSize = 0;
	BYTE* DataBuffer = NULL;
	IDXGIResource*& DesktopResource = *pDesktopResource;

	IDXGIOutputDuplication*& gOutputDuplication = *pgOutputDuplication;
	ID3D11Texture2D*& gAcquiredDesktopImage = *pgAcquiredDesktopImage;

	/*if (wfi->framesWaiting > 0)
	{
		wf_dxgi_releasePixelData(wfi);
	}*/

	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	if (gAcquiredDesktopImage)
	{
		gAcquiredDesktopImage->Release();
		gAcquiredDesktopImage = NULL;
	}

	status = gOutputDuplication->AcquireNextFrame(timeout, &FrameInfo, &DesktopResource);

	if (status == DXGI_ERROR_WAIT_TIMEOUT)
	{
	    printf("broken5\n");
		return 1;
	}

	if (FAILED(status))
	{
		if (status == DXGI_ERROR_ACCESS_LOST)
		{
			//WLog_ERR(TAG, "Failed to acquire next frame with status=%#X", status);
			//WLog_ERR(TAG, "Trying to reinitialize due to ACCESS LOST...");

			if (gAcquiredDesktopImage)
			{
				gAcquiredDesktopImage->Release();
                gAcquiredDesktopImage = NULL;
			}

			if (gOutputDuplication)
			{
				gOutputDuplication->Release();
				gOutputDuplication = NULL;
			}

			dxgi_getDuplication(gDevice, &gOutputDuplication);

			printf("broken3\n");

			return 1;
		}
		else
		{
			//WLog_ERR(TAG, "Failed to acquire next frame with status=%#X", status);
			status = gOutputDuplication->ReleaseFrame();

			if (FAILED(status))
			{
				//WLog_ERR(TAG, "Failed to release frame with status=%d", status);
				printf("newbroken\n");
			}

			printf("broken4\n");

			return 1;
		}
	}

	status = DesktopResource->QueryInterface(IID_ID3D11Texture2D, (void**) &gAcquiredDesktopImage);
	DesktopResource->Release();
	DesktopResource = NULL;

	*pgAcquiredDesktopImage = gAcquiredDesktopImage;

	//printf("%i\n", gAcquiredDesktopImage);

	if (FAILED(status))
	{
	    printf("broken2\n");
        return 1;
	}

	//wfi->framesWaiting = FrameInfo.AccumulatedFrames;

	if (FrameInfo.AccumulatedFrames == 0)
	{
		status = gOutputDuplication->ReleaseFrame();

		if (FAILED(status))
		{
			//WLog_ERR(TAG, "Failed to release frame with status=%d", status);
			printf("broken\n");
		}
	}

	return 0;
}
#endif

//typedef unsigned __int32 uint32_t;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
/*int WinMain(
  _In_ HINSTANCE hInstance,
  _In_ HINSTANCE hPrevInstance,
  _In_ LPSTR     lpCmdLine,
  _In_ int       nCmdShow
)*/
{
    printf("hi\n");

    ID3D11Device* pdev = nullptr;
    ID3D11DeviceContext* context = nullptr;

    //D3D_FEATURE_LEVEL LEV = D3D_FEATURE_LEVEL_11_1;

    //int ndat[1000] = {0};

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_1
    };

    HRESULT r3 = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pdev, nullptr, &context);

    //ID3D11Debug* dbg = NULL;

    /*pdev->QueryInterface(IID_ID3D11Debug, (void**) &dbg);

    HRESULT r4 = dbg->ValidateContext(context);

    printf("r4 %x\n", r4);*/


    //D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pdev, nullptr, &context);

    //int ndat2[1000] = {0};

    //ndat[999] = ndat2[999];

    //printf("%i %i\n", ndat[998], ndat2[998]);

    printf("%x\n", r3);


    /*D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1680;
    desc.Height = 1050;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;*/



    IDXGIOutputDuplication* dup = nullptr;

    dxgi_getDuplication(pdev, &dup);

    ID3D11Texture2D* gAcquiredDesktopImage = NULL;

    IDXGIResource* dres = NULL;

    ///1 is timeout in ms
    dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);
    dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);
    dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);

    int i = 0;

    while(gAcquiredDesktopImage == nullptr)
    {
        sf::Clock clk;

        dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);

        std::cout << clk.getElapsedTime().asMicroseconds() / 1000.f << std::endl;
    }



    D3D11_TEXTURE2D_DESC desc = {0};
    desc.Width = 1680;
    desc.Height = 1050;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    /*D3D11_SUBRESOURCE_DATA *sSubData = new D3D11_SUBRESOURCE_DATA;
    sSubData->pSysMem = new uint32_t[desc.Width*desc.Height]();
    sSubData->SysMemPitch = desc.Width * sizeof(uint32_t);
    sSubData->SysMemSlicePitch = 1;*/

    ID3D11Texture2D *pTexture = nullptr;
    HRESULT r2 = pdev->CreateTexture2D( &desc, nullptr, &pTexture );

    printf("WHAT %x\n", r2);

    /*
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = 256;
    desc.Height = 256;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    */

    //ID3D11Texture2D* d3dres = nullptr;

    //dres->QueryInterface(IID_ID3D11Texture2D, (void**)&d3dres);

    context->CopyResource(pTexture, gAcquiredDesktopImage);//(ID3D11Texture2D*)d3dres);

    gAcquiredDesktopImage->Release();
    dup->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE mapped;

    HRESULT res = context->Map(pTexture, 0, D3D11_MAP_READ, 0, &mapped);

    printf("MAP %x\n", res);

    uint32_t* dat = (uint32_t*)mapped.pData;

    sf::Image img;
    img.create(1680, 1050);

    for(int y=0; y<1050; y++)
    {
        for(int x=0; x<1680; x++)
        {
            //printf("%i %i\n", x, y);

            uint32_t val = dat[y*mapped.RowPitch/4 + x];

            img.setPixel(x, y, sf::Color(val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF));

            //if(val != 0)
            //    printf("%i\n", val);
        }
    }

    img.saveToFile("test.png");

    //D3DX11SaveTextureToFile(context, gAcquiredDesktopImage, D3DX11_IFF_BMP, "Test.bmp");

    return 0;
}
