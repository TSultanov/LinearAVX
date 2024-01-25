#include <immintrin.h>
#include <stdio.h>

int main() {
  __m256 a = _mm256_set_ps(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
  __m256 b = _mm256_set_ps(8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f);
  __m256 c = _mm256_add_ps(a, b);

  float d[8];
  _mm256_store_ps(d, c);

  printf("d = %f, %f, %f, %f, %f, %f, %f, %f\n", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);

  return 0;
}
