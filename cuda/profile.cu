#include <cuda_runtime.h>
#include <iostream>
#include <stdexcept>
#include <vector>
void ck(cudaError_t e) {
  if (e != cudaSuccess)
    throw std::runtime_error(cudaGetErrorString(e));
}
__global__ void add(const float *a, const float *b, float *c, int n) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n)
    c[i] = a[i] + b[i];
}
int main() {
  try {
    constexpr int n = 1 << 20;
    float *a, *b, *c;
    std::vector<float> h(n, 1), out(n);
    ck(cudaMalloc(&a, n * sizeof(float)));
    ck(cudaMalloc(&b, n * sizeof(float)));
    ck(cudaMalloc(&c, n * sizeof(float)));
    ck(cudaMemcpy(a, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    ck(cudaMemcpy(b, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    for (int i = 0; i < 10; ++i)
      add<<<(n + 255) / 256, 256>>>(a, b, c, n);
    ck(cudaDeviceSynchronize());
    cudaEvent_t start, end;
    ck(cudaEventCreate(&start));
    ck(cudaEventCreate(&end));
    ck(cudaEventRecord(start));
    for (int i = 0; i < 100; ++i)
      add<<<(n + 255) / 256, 256>>>(a, b, c, n);
    ck(cudaGetLastError());
    ck(cudaEventRecord(end));
    ck(cudaEventSynchronize(end));
    float ms;
    ck(cudaEventElapsedTime(&ms, start, end));
    ck(cudaMemcpy(out.data(), c, n * sizeof(float), cudaMemcpyDeviceToHost));
    for (float x : out)
      if (x != 2)
        throw std::runtime_error("wrong result");
    std::cout << "elements,iterations,mean_us\n" << n << ",100," << ms * 10 << '\n';
    ck(cudaEventDestroy(start));
    ck(cudaEventDestroy(end));
    ck(cudaFree(a));
    ck(cudaFree(b));
    ck(cudaFree(c));
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
