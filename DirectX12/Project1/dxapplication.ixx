module;
#include <Windows.h>
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

export module dxapplication;
import std;

using Microsoft::WRL::ComPtr;

export namespace frame
{
	constexpr unsigned int FRAME_COUNT = 2;
}

export class DXApplication
{
public:
	DXApplication(unsigned int w, unsigned int h, std::wstring title);
	void Initialize(HWND hwnd);
	void Terminate();
	void Process();
	void Render();

	[[nodiscard]] const WCHAR* GetTitle() const { return _title.c_str(); }
	[[nodiscard]] unsigned int GetWidth() const { return _width; }
	[[nodiscard]] unsigned int GetHeight() const { return _height; }

private:
	std::wstring _title;// ウィンドウのタイトル
	unsigned int _width;// ウィンドウの幅
	unsigned int _height;// ウィンドウの高さ

	// ポリゴン描画
	CD3DX12_VIEWPORT _viewport;// ビューポート
	CD3DX12_RECT _scissorrect;// シザリング矩形
	ComPtr<ID3D12PipelineState> _pipelineState;// パイプラインステート
	ComPtr<ID3D12RootSignature> _rootSignature;// ルートシグネチャ
	// リソース
	ComPtr<ID3D12Resource> _vertexBuffer;// 頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;// 頂点バッファビュー
	ComPtr<ID3D12Resource> _indexBuffer;// インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;// インデックスバッファビュー

	// パイプラインオブジェクト
	ComPtr<ID3D12Device> _device;// GPUを使って描画を始める
	ComPtr<ID3D12CommandAllocator> _commandAllocator;// 命令データを保存しておくためのメモリ領域
	ComPtr<ID3D12GraphicsCommandList> _commandList;// 描画命令を記録するためのオブジェクト
	ComPtr<ID3D12CommandQueue> _commandQueue;// 描画命令を実行するためのオブジェクト
	ComPtr<IDXGISwapChain4> _swapChain;// 描画結果を表示するためのオブジェクト
	ComPtr<ID3D12DescriptorHeap> _rtvHeap;// 描画先のメモリの管理名簿
	ComPtr<ID3D12Resource> _renderTargets[frame::FRAME_COUNT];// 描画先のメモリ
	// フェンス
	ComPtr<ID3D12Fence> _fence;// GPUとCPUの処理の同期を取るためのオブジェクト
	UINT64 _fenceValue;// フェンスの値
	HANDLE _fenceEvent;// CPUを待機させる
	void LoadPipline(HWND hwnd);// パイプラインの初期化
	void LoadAssets();// 頂点バッファの初期化
	void CreateD3D12Device(IDXGIFactory7* dxgiFactory, ID3D12Device** d3d12device);
	void ThrowIfFailed(HRESULT hr);// エラーコードをチェックして、失敗していたら例外を投げる

	void CommandInitialize();// コマンドオブジェクトの初期化(CPUをフルに使って、GPUに休みなく仕事をさせる)
	void SwapchainInitialize(ComPtr<IDXGIFactory7> dxgiFactory, HWND hwnd);// スワップチェーンの初期化(描画結果を表示するためのオブジェクト)
	void DescriptorHeapInitialize();// デスクリプタヒープの初期化(GPUリソースの管理場所)
	void RenderTargetViewInitialize();// レンダーターゲットビューの初期化(描画先のメモリを描画できるようにする)
	void FenceInitialize();// フェンスの初期化(GPUとCPUの処理の同期を取るためのオブジェクト)

	void CommandListReset();// コマンドリストのリセット(描画命令を記録するためのオブジェクトを初期状態に戻す)
	void CommandListGenerate();// コマンドリストの生成(描画命令を記録するためのオブジェクトに描画命令を記録する
	void CommandListExecute();// コマンドリストの実行(描画命令を記録するためのオブジェクトに記録された命令を実行する)
	void GPUWait();// GPUの処理が終わるまで待機する
};