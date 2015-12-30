#include <iostream>

#include <InitGuid.h>

///some combination of these must be what I want
#include <windows.h>
#include <gdiplus.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <DXGI.h>
#include <dxgiformat.h>
#include <dxgitype.h>
#include <DXGI1_2.h>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>


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
		printf("Failed to get QI for DXGI Device\n");
		return 1;
	}

	status = DxgiDevice->GetParent(IID_IDXGIAdapter, (void**) &DxgiAdapter);
	DxgiDevice->Release();
	DxgiDevice = NULL;

	if (FAILED(status))
	{
		printf("Failed to get parent DXGI Adapter\n");
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
			printf("Failed to get description\n");
			return 1;
		}

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
	dTop = 0; ///???

	status = DxgiAdapter->EnumOutputs(dTop, &DxgiOutput);
	DxgiAdapter->Release();
	DxgiAdapter = NULL;

	if (FAILED(status))
	{
		printf("Failed to get output\n");
		return 1;
	}

	status = DxgiOutput->QueryInterface(IID_IDXGIOutput1, (void**) &DxgiOutput1);
	DxgiOutput->Release();
	DxgiOutput = NULL;

	if (FAILED(status))
	{
		printf("Failed to get IDXGIOutput1\n");
		return 1;
	}

	status = DxgiOutput1->DuplicateOutput((IUnknown*)gDevice, pgOutputDuplication);
	DxgiOutput1->Release();
	DxgiOutput1 = NULL;

	if (FAILED(status))
	{
		if (status == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			printf("Too many cooks on the duplication api\n");
			return 1;
		}

		printf("Failed to get duplicate output with err %x\n", status);
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

	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	if (gAcquiredDesktopImage)
	{
		gAcquiredDesktopImage->Release();
		gAcquiredDesktopImage = NULL;
	}

	status = gOutputDuplication->AcquireNextFrame(timeout, &FrameInfo, &DesktopResource);

	if (status == DXGI_ERROR_WAIT_TIMEOUT)
	{
	    //printf("Timeout\n");
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

			printf("Access lost, regrabbing\n");

			return 1;
		}
		else
		{
            printf("Failed to get next frame %x\n", status);

			//WLog_ERR(TAG, "Failed to acquire next frame with status=%#X", status);
			status = gOutputDuplication->ReleaseFrame();

			if (FAILED(status))
			{
				//WLog_ERR(TAG, "Failed to release frame with status=%d", status);
				printf("Failed to release frame???\n");
			}

			return 1;
		}
	}

	status = DesktopResource->QueryInterface(IID_ID3D11Texture2D, (void**) &gAcquiredDesktopImage);
	DesktopResource->Release();
	DesktopResource = NULL;

	*pgAcquiredDesktopImage = gAcquiredDesktopImage;

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
			printf("Failed to release outputduplication frame %d\n", status);
		}
	}

	return 0;
}


vec2f get_lk(float* block_pxl, float* block_pxl_old, vec2i pos, int block_size, int w)
{
    float m1 = 0, m2 = 0, m3 = 0, m4 = 0;

    float y1 = 0.f, y2 = 0.f;

    int x = pos.v[0];
    int y = pos.v[1];

    for(int jj=0; jj<block_size; jj++)
    {
        for(int ii=0; ii<block_size; ii++)
        {
            int xc = x*block_size + ii;

            int yc = y*block_size + jj;

            float ldx = block_pxl[(yc)*w + xc + 1] - block_pxl[(yc)*w + xc - 1];
            float ldy = block_pxl[(yc + 1)*w + xc] - block_pxl[(yc - 1)*w + xc];

            float ldt = block_pxl[(yc)*w + xc] - block_pxl_old[(yc)*w + xc];

            m1 += ldx*ldx;
            m2 += ldx*ldy;
            m3 += ldx*ldy;
            m4 += ldy*ldy;

            y1 += ldx * ldt;
            y2 += ldy * ldt;
        }
    }

    y1 = -y1;
    y2 = -y2;

    float inv = 1.f / (m1 * m4 - m2 * m3);

    if(m1 * m4 - m2 * m3 < 0.001f)
        inv = 0.f;

    float nm1, nm2, nm3, nm4;

    nm1 = m4 * inv;
    nm2 = -m2 * inv;
    nm3 = -m3 * inv;
    nm4 = m1 * inv;

    float v1 = nm1 * y1 + nm2 * y2;
    float v2 = nm3 * y1 + nm4 * y2;

    return {v1, v2};
}

vec2f get_lk_pix(float* block_pxl, float* block_pxl_old, vec2i pos, int block_size, int w)
{
    float m1 = 0, m2 = 0, m3 = 0, m4 = 0;

    float y1 = 0.f, y2 = 0.f;

    int x = pos.v[0];
    int y = pos.v[1];

    for(int jj=0; jj<block_size; jj++)
    {
        for(int ii=0; ii<block_size; ii++)
        {
            int xc = x + ii;

            int yc = y + jj;

            float ldx = block_pxl[(yc)*w + xc + 1] - block_pxl[(yc)*w + xc - 1];
            float ldy = block_pxl[(yc + 1)*w + xc] - block_pxl[(yc - 1)*w + xc];

            float ldt = block_pxl[(yc)*w + xc] - block_pxl_old[(yc)*w + xc];

            m1 += ldx*ldx;
            m2 += ldx*ldy;
            m3 += ldx*ldy;
            m4 += ldy*ldy;

            y1 += ldx * ldt;
            y2 += ldy * ldt;
        }
    }

    y1 = -y1;
    y2 = -y2;

    float inv = 1.f / (m1 * m4 - m2 * m3);

    if(m1 * m4 - m2 * m3 < 0.001f)
        inv = 0.f;

    float nm1, nm2, nm3, nm4;

    nm1 = m4 * inv;
    nm2 = -m2 * inv;
    nm3 = -m3 * inv;
    nm4 = m1 * inv;

    float v1 = nm1 * y1 + nm2 * y2;
    float v2 = nm3 * y1 + nm4 * y2;

    return {v1, v2};
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    ID3D11Device* pdev = nullptr;
    ID3D11DeviceContext* context = nullptr;

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

    printf("%x\n", r3);

    int w = 1680;
    int h = 1050;

    D3D11_TEXTURE2D_DESC desc = {0};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ID3D11Texture2D *pTexture = nullptr;
    HRESULT r2 = pdev->CreateTexture2D( &desc, nullptr, &pTexture );

    printf("WHAT %x\n", r2);

    IDXGIOutputDuplication* dup = nullptr;

    dxgi_getDuplication(pdev, &dup);

    ID3D11Texture2D* gAcquiredDesktopImage = NULL;

    IDXGIResource* dres = NULL;

    dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);
    dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);
    dxgi_nextFrame(2, pdev, &gAcquiredDesktopImage, &dup, &dres);

    sf::Clock clk;

    const int block_size = 20;

    int cw, ch;
    cw = ceilf((float)w/block_size);
    ch = ceilf((float)h/block_size);

    float* block_pxl = new float[w*h]();
    float* block_pxl_old = new float[w*h]();

    while(1)
    {
        int res = dxgi_nextFrame(0, pdev, &gAcquiredDesktopImage, &dup, &dres);

        if(res == 0)
        {
            std::cout << clk.getElapsedTime().asMicroseconds() / 1000.f << std::endl;
            clk.restart();

            context->CopyResource(pTexture, gAcquiredDesktopImage);

            D3D11_MAPPED_SUBRESOURCE mapped;

            HRESULT res = context->Map(pTexture, 0, D3D11_MAP_READ, 0, &mapped);

            //printf("MAP %x\n", res);

            uint32_t* dat = (uint32_t*)mapped.pData;

            for(int y=0; y<h; y++)
            {
                for(int x=0; x<w; x++)
                {
                    uint32_t val = dat[y*mapped.RowPitch/4 + x];

                    uint8_t r = (val >> 16) & 0xFF;
                    uint8_t g = (val >> 8) & 0xFF;
                    uint8_t b = (val >> 0) & 0xFF;

                    ///eh fuckit
                    float intensity = r * 0.3f + g * 0.3f + b * 0.3f;

                    block_pxl[y*w + x] = intensity;
                }
            }

            context->Unmap(pTexture, 0);

            const float high_magnitude_threshold = 1.25f;

            float mag_accum = 0.f;

            float pixel_cx = w/2.f;
            float pixel_cy = h/2.f;

            int bnum = 0;

            for(int y=1; y<ch-1; y++)
            {
                for(int x=1; x<cw-1; x++)
                {
                    ///lazy
                    bnum++;

                    vec2f dir = get_lk(block_pxl, block_pxl_old, {x, y}, block_size, w);

                    float v1 = dir.v[0];
                    float v2 = dir.v[1];

                    float len = sqrtf(v1 * v1 + v2 * v2) * 10.f;

                    len = std::min(len, 30.f);

                    mag_accum += len;

                    #ifdef MOTION
                    vec2f ls = {x * (float)block_size, y * (float)block_size};

                    vec2f fin = ls + dir.norm() * len;//dir.norm() * len + ls;

                    if(x >= cbx - check_size && x <= cbx + check_size &&
                       y >= cby - check_size && y <= cby + check_size)
                    {
                        SDL_SetRenderDrawColor( renderer, 0xFF, 0x00, 0x00, 0xFF );
                    }
                    else
                        SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0xFF, 0xFF );

                    ls = ls / (vec2f){w, h};
                    ls = ls * (vec2f){winw, winh};

                    fin = fin / (vec2f){w, h};
                    fin = fin * (vec2f){winw, winh};

                    SDL_RenderDrawLine(renderer, ls.v[0], ls.v[1], fin.v[0], fin.v[1]);
                    #endif
                }
            }

            #define DO_MOUSE
            #ifdef DO_MOUSE

            bool fire = false;

            int trigger_block = 6;

            vec2f dir = get_lk_pix(block_pxl, block_pxl_old, (vec2i){pixel_cx, pixel_cy} - trigger_block/2, trigger_block, w);

            const float thresh = 0.15;

            if(dir.length() > thresh)
                fire = true;

            mag_accum /= bnum;

            if(mag_accum >= high_magnitude_threshold)
            {
                printf("high %f ", mag_accum);
                fire = false;
            }

            if(fire)
            {
                printf("FIRE! %f ", dir.length());
                mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_MOVE, 0, 0, 0, GetMessageExtraInfo());

                mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_MOVE, 0, 0, 0, GetMessageExtraInfo());
            }

            #endif

            std::swap(block_pxl, block_pxl_old);
        }
    }

    return 0;
}
