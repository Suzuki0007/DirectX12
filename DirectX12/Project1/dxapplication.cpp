module;
#include <Windows.h>
#include <ranges>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "directx/d3d12.h"
#include "directx/d3dx12.h"
#include <stdexcept>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
module dxapplication;
import std;

using Microsoft::WRL::ComPtr;

namespace
{
	// デバイスの初期化
	constexpr D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
}


DXApplication::DXApplication(unsigned int w, unsigned int h, std::wstring title)
{
	this->_title = title;
	this->_width = w;
	this->_height = h;
	this->_fenceValue = 0;
	this->_fenceEvent = nullptr;
}

// 初期化
void DXApplication::Initialize(HWND hwnd)
{
	LoadPipline(hwnd);
}

// 終了
void DXApplication::Terminate()
{
	CloseHandle(_fenceEvent);
}

void DXApplication::LoadPipline(HWND hwnd)
{
	UINT dxgiFactoryFlogs = 0;

	#ifdef _DEBUG
		{
			// デバッグレイヤーを有効にする
			ComPtr<ID3D12Debug> debugLayer;
			//　デバッグ機能を無事に呼び出せたかどうか？
			if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
			{
				debugLayer->EnableDebugLayer();					// デバッグレイヤーを有効にする
				dxgiFactoryFlogs |= DXGI_CREATE_FACTORY_DEBUG;	// デバッグモードで動かす
			}
		}
	#endif

	// DXGIFactoryの初期化
	ComPtr<IDXGIFactory7> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlogs, IID_PPV_ARGS(&dxgiFactory)));// DXGIファクトリーを作成する

	// デバイスの初期化
	CreateD3D12Device(dxgiFactory.Get(), _device.ReleaseAndGetAddressOf());
	// コマンドの初期化
	CommandInitialize();
	// スワップチェーンの初期化
	SwapchainInitialize(dxgiFactory, hwnd);
	// デスクリプタヒープの初期化
	DescriptorHeapInitialize();
	// レンダーターゲットビューの初期化
	RenderTargetViewInitialize();
	// フェンスの初期化
	FenceInitialize();
}

void DXApplication::ThrowIfFailed(HRESULT hr)
{
	if(FAILED(hr))
	{
		// 失敗していたら例外を投げる
		// std::runtime_error : 標準の例外クラス。エラーメッセージを文字列で渡すことができる
		// std::format : 書式化された文字列を生成する。{}の中に引数を埋め込む
		// 0x{:08X} : 16進数で8桁の整数を表示する
		// static_cast<UINT>(hr) : hrをUINT型に変換する
		throw std::runtime_error(std::format("HRESULT of 0x{:08X}", hr));
	}
}

void DXApplication::CreateD3D12Device(IDXGIFactory7* dxgiFactory, ID3D12Device** d3d12device)
{
	IDXGIAdapter* targetAdapter = nullptr;
	IDXGIAdapter* tmpAdapter = nullptr;

	// アダプターを列挙しながら、その場でAMDを探す
	for(int i = 0; SUCCEEDED(dxgiFactory->EnumAdapters(i, &tmpAdapter)); ++i)
	{
		DXGI_ADAPTER_DESC desc;
		tmpAdapter->GetDesc(&desc);

		// AMDのアダプターを見つけたら、targetAdapterに保存する。2つ目以降のAMDは無視する。
		if(targetAdapter == nullptr && std::wstring_view(desc.Description).contains(L"AMD"))
		{
			targetAdapter = tmpAdapter; // 最初に見つかったAMDをキープする
		}
		else
		{
			tmpAdapter->Release(); // 使わない、または2つ目以降のAMDは即解放
		}
	}

	// デバイスの初期化
	ID3D12Device* tmpDevice = nullptr;
	for(auto level : levels)
	{
		if(SUCCEEDED(D3D12CreateDevice(targetAdapter, level, IID_PPV_ARGS(&tmpDevice))))
		{
			break;
		}
	}

	// targetAdapter も使い終わったら解放
	if(targetAdapter != nullptr)
	{
		targetAdapter->Release();
	}

	*d3d12device = tmpDevice;
}

void DXApplication::CommandInitialize()
{
	// コマンドキュー(GPUへと続くための送信ルート)
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // タイムアウト無しでコマンドを実行する
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // コマンドリストと合わせる
	ThrowIfFailed
	(
		// コマンドキューを作成する
		_device->CreateCommandQueue
		(
			&commandQueueDesc,// コマンドキューの設定を渡す
			IID_PPV_ARGS(_commandQueue.ReleaseAndGetAddressOf())// コマンドキューのアドレスを渡す
		)
	);

	// コマンドアロケータ(命令を書き込むためのメモリ領域)
	ThrowIfFailed
	(
		//コマンドアロケータを作成する
		_device->CreateCommandAllocator
		(
			D3D12_COMMAND_LIST_TYPE_DIRECT,// コマンドリストと合わせる
			IID_PPV_ARGS(_commandAllocator.ReleaseAndGetAddressOf())// コマンドアロケータのアドレスを渡す
		)
	);
	// コマンドリスト(GPUに指示を送るためのオブジェクト)
	ThrowIfFailed
	(
		// コマンドリストを作成する
		_device->CreateCommandList
		(
			0,// シングルのGPU環境(マルチGPU環境では1以上の値を指定する)
			D3D12_COMMAND_LIST_TYPE_DIRECT,// リストコマンドの種類を指定する
			_commandAllocator.Get(),// メモリ領域を指定する
			nullptr,//シェーダーや描画設定をまとめたオブジェクトを指定する
			IID_PPV_ARGS(_commandList.ReleaseAndGetAddressOf())// コマンドリストのアドレスを渡す
		)
	);
	ThrowIfFailed(_commandList->Close());
}

void DXApplication::SwapchainInitialize(ComPtr<IDXGIFactory7> dxgiFactory, HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = frame::FRAME_COUNT; // バッファの数
	swapChainDesc.Width = _width; // ウィンドウの幅
	swapChainDesc.Height = _height; // ウィンドウの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // R(赤)G(緑)B(青)A(透明度)をそれぞれ8ビット
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // レンダリングの出力先として使う
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 画面の更新
	swapChainDesc.SampleDesc.Count = 1; // ギザギザを目立たなくし、滑らかに見せる
	ThrowIfFailed
	(
		dxgiFactory->CreateSwapChainForHwnd
		(
			_commandQueue.Get(), // コマンドキューが画面を更新する
			hwnd, // どのウィンドウに描画するかを指定する
			&swapChainDesc, // スワップチェーンの設定を渡す
			nullptr, // フルスクリーンの設定を渡す
			nullptr, // 出力モニタの制限
			(IDXGISwapChain1**)_swapChain.ReleaseAndGetAddressOf()// スワップチェーンのアドレスを渡す
		)
	);
}

void DXApplication::DescriptorHeapInitialize()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frame::FRAME_COUNT; // 描画の数
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 指定なし
	ThrowIfFailed
	(
		_device->CreateDescriptorHeap
		(
			&rtvHeapDesc,// デスクリプタヒープの設定を渡す
			IID_PPV_ARGS(_rtvHeap.ReleaseAndGetAddressOf())// デスクリプタヒープのアドレスを渡す
		)
	);
}

void DXApplication::RenderTargetViewInitialize()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());// デスクリプタヒープの先頭のアドレスを取得する
	
	
	for(auto i : std::views::iota(0u, frame::FRAME_COUNT))// std::views::iotaは0からframe::FRAME_COUNT-1までの整数を生成する
	{
		ThrowIfFailed
		(
			_swapChain->GetBuffer
			(
				static_cast<UINT>(i), // 何番目のバッファを取得するかを指定する
				IID_PPV_ARGS(_renderTargets[i].ReleaseAndGetAddressOf())// バッファのアドレスを渡す
			)
		);

		_device->CreateRenderTargetView
		(
			_renderTargets[i].Get(),	// 描画先のバッファを指定する
			nullptr,				// レンダーターゲットビューの設定
			rtvHandle				// レンダーターゲットビューのアドレスを渡す
		);

		rtvHandle.Offset
		(
			1,// 次のアドレスに移動する
			// デスクリプタヒープの種類ごとに、次のアドレスに移動するための値を取得する
			_device->GetDescriptorHandleIncrementSize
			(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV// レンダーターゲットビューの値を取得する
			)
		);
	}
}

void DXApplication::FenceInitialize()
{
	ThrowIfFailed
	(
		// フェンスを作成する
		_device->CreateFence
		(
			_fenceValue, // フェンスの初期値を指定する
			D3D12_FENCE_FLAG_NONE, // フェンスの動作を指定する
			IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()) // フェンスのアドレスを渡す
		)
	);

	_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);// フェンスイベントを作成する
}

// 処理
void DXApplication::Process()
{
	
}

void DXApplication::CommandListReset()
{
	// コマンドアロケータをリセットする
	ThrowIfFailed
	(
		_commandAllocator->Reset()
	);
	// コマンドリストをリセットする
	ThrowIfFailed
	(
		_commandList->Reset
		(
			_commandAllocator.Get(),
			nullptr
		)
	);
}

void DXApplication::CommandListGenerate()
{
	// バックバッファのインデックスを取得する
	auto frameIndex = _swapChain->GetCurrentBackBufferIndex();// 現在のフレームのインデックスを取得する

	// リソースバリアの設定
	// バリア：コマンド実行時にリソースが何の用途に使用されるのかをGPUに伝えるもの
	auto startResorceBarrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		_renderTargets[frameIndex].Get(),// 描画先のバッファを指定する
		D3D12_RESOURCE_STATE_PRESENT, // 描画前の状態
		D3D12_RESOURCE_STATE_RENDER_TARGET // 描画後の状態
	);
	_commandList->ResourceBarrier(1, &startResorceBarrier);// 描画前の状態から描画後の状態に遷移するためのバリアをコマンドリストに追加する

	// レンダーターゲットの設定
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle
	(
		_rtvHeap->GetCPUDescriptorHandleForHeapStart(),// デスクリプタヒープの先頭のアドレスを取得する
		frameIndex,// 現在のフレームのインデックスを指定する
		// デスクリプタヒープの種類ごとに、次のアドレスに移動するための値を取得する
		_device->GetDescriptorHandleIncrementSize
		(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV// レンダーターゲットビューの値を取得する
		)
	);
	_commandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);// 描画先のバッファをコマンドリストに設定する

	// 画面をクリアする
	float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };// 画面を黒色でクリア
	_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);// 描画先のバッファをクリアする

	// リソースバリアの設定
	auto endResorceBarrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		_renderTargets[frameIndex].Get(),// 描画先のバッファを指定する
		D3D12_RESOURCE_STATE_RENDER_TARGET, // 描画後の状態
		D3D12_RESOURCE_STATE_PRESENT // 描画前の状態
	);
	_commandList->ResourceBarrier(1, &endResorceBarrier);// 描画後の状態から描画前の状態に遷移するためのバリアをコマンドリストに追加する

	// 命令を閉じる
	_commandList->Close();
}

void DXApplication::CommandListExecute()
{
	ID3D12CommandList* commandLists[] = { _commandList.Get() };// コマンドリストの配列を作成する
	_commandQueue->ExecuteCommandLists(1, commandLists);// コマンドリストを実行する
	// 画面のスワップ
	ThrowIfFailed
	(
		_swapChain->Present(1, 0)// 画面を更新する
	);
}

void DXApplication::GPUWait()
{
	ThrowIfFailed
	(
		_commandQueue->Signal(_fence.Get(), ++_fenceValue)// フェンスの値を更新して、GPUに通知する
	);

	if(_fence->GetCompletedValue() < _fenceValue)
	{
		ThrowIfFailed
		(
			_fence->SetEventOnCompletion(_fenceValue, _fenceEvent)// フェンスの値が更新されるまで待機する
		);
		WaitForSingleObject(_fenceEvent, INFINITE);// フェンスイベントがシグナル状態になるまで待機する
	}
}

// 描画
void DXApplication::Render()
{
	CommandListReset();// コマンドリストをリセットする
	CommandListGenerate();// コマンドリストを生成する
	CommandListExecute();// コマンドリストを実行する
	GPUWait();// GPUの処理が終わるまで待機する
}