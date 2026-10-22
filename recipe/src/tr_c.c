// SPDX-FileCopyrightText: 2026 Angelos Katis (RADECTI)
// SPDX-License-Identifier: MIT
/* TRAINING program (pack build set) -- C runtime coverage for mingw-w64.
 * Never executed. Its job is to make the linker pull in as much of the static
 * runtime as possible: crt2/crtbegin/crtend, libmingw32, libmingwex (C99 stdio
 * engine, math, getopt, dirent, ...), libgcc, libssp, libwinpthread.
 * Author functions here are EXCLUDED from the pack by linker-map origin. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <math.h>
#include <complex.h>
#include <fenv.h>
#include <inttypes.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <setjmp.h>
#include <stdarg.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <direct.h>
#include <process.h>
#include <getopt.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>
#include <windows.h>

typedef void (*anyfn)(void);
#define P(f) (anyfn)(f)

/* Referenced, never called: forces every one of these into the image. */
volatile anyfn TABLE[] = {
    /* stdio */
    P(printf), P(fprintf), P(sprintf), P(snprintf), P(vprintf), P(vfprintf), P(vsprintf),
    P(vsnprintf), P(scanf), P(fscanf), P(sscanf), P(vsscanf), P(fopen), P(freopen), P(fclose),
    P(fread), P(fwrite), P(fseek), P(ftell), P(rewind), P(fgetpos), P(fsetpos), P(fflush),
    P(fgets), P(fputs), P(puts), P(putchar), P(getchar), P(fgetc), P(fputc), P(ungetc),
    P(perror), P(tmpfile), P(tmpnam), P(remove), P(rename), P(setvbuf), P(setbuf),
    P(clearerr), P(feof), P(ferror), P(fileno), P(_fdopen), P(_popen), P(_pclose),
    P(fwprintf), P(swprintf), P(vswprintf), P(wprintf), P(fwscanf), P(swscanf),
    P(fgetws), P(fputws), P(fgetwc), P(fputwc), P(_wfopen),
    P(__mingw_printf), P(__mingw_fprintf), P(__mingw_sprintf), P(__mingw_snprintf),
    P(__mingw_vsnprintf), P(__mingw_vfprintf), P(__mingw_sscanf), P(__mingw_vsscanf),
    P(__mingw_fscanf), P(__mingw_swprintf), P(__mingw_vswprintf), P(__mingw_wprintf),
    P(__mingw_asprintf), P(__mingw_vasprintf),
    /* stdlib */
    P(malloc), P(calloc), P(realloc), P(free), P(qsort), P(bsearch), P(atoi), P(atol),
    P(atoll), P(atof), P(strtol), P(strtoul), P(strtoll), P(strtoull), P(strtod), P(strtof),
    P(strtold), P(strtoimax), P(strtoumax), P(wcstol), P(wcstoul), P(wcstod), P(wcstof),
    P(wcstold), P(wcstoll), P(wcstoull), P(rand), P(srand), P(getenv), P(_putenv), P(abs), P(labs), P(llabs), P(imaxabs), P(div), P(ldiv),
    P(lldiv), P(imaxdiv), P(system), P(atexit), P(mbstowcs), P(wcstombs),
    P(mbtowc), P(wctomb), P(mblen), P(mbrtowc), P(wcrtomb), P(mbrlen), P(mbsrtowcs),
    P(wcsrtombs), P(btowc), P(wctob), P(_aligned_malloc), P(_aligned_free),
    P(mkstemp), P(_itoa), P(_ltoa), P(_ultoa), P(_i64toa), P(_ecvt), P(_fcvt), P(_gcvt),
    /* string */
    P(strlen), P(strnlen), P(strcpy), P(strncpy), P(strcat), P(strncat), P(strcmp),
    P(strncmp), P(strchr), P(strrchr), P(strstr), P(strtok), P(strtok_r), P(strspn),
    P(strcspn), P(strpbrk), P(strerror), P(strcoll), P(strxfrm), P(memcpy), P(memmove),
    P(memset), P(memcmp), P(memchr), P(_strdup), P(strdup), P(_stricmp), P(_strnicmp),
    P(strcasecmp), P(strncasecmp), P(_strlwr), P(_strupr), P(_strrev), P(mempcpy),
    P(wcslen), P(wcsnlen), P(wcscpy), P(wcsncpy), P(wcscat), P(wcscmp), P(wcsncmp),
    P(wcschr), P(wcsrchr), P(wcsstr), P(wcstok), P(wcsspn), P(wcscspn), P(wmemcpy),
    P(wmemmove), P(wmemset), P(wmemcmp), P(wmemchr), P(_wcsdup), P(_wcsicmp), P(wcscoll),
    P(wcsftime),
    /* ctype / wctype */
    P(isalpha), P(isdigit), P(isalnum), P(isspace), P(isupper), P(islower), P(isxdigit),
    P(ispunct), P(isprint), P(iscntrl), P(isgraph), P(isblank), P(toupper), P(tolower),
    P(iswalpha), P(iswdigit), P(iswspace), P(towupper), P(towlower), P(iswctype), P(wctype),
    /* math (double, float, long double) */
    P(sin), P(cos), P(tan), P(asin), P(acos), P(atan), P(atan2), P(sinh), P(cosh), P(tanh),
    P(asinh), P(acosh), P(atanh), P(exp), P(exp2), P(expm1), P(log), P(log2), P(log10),
    P(log1p), P(logb), P(ilogb), P(pow), P(sqrt), P(cbrt), P(hypot), P(fmod), P(remainder),
    P(remquo), P(floor), P(ceil), P(round), P(trunc), P(lround), P(llround), P(lrint),
    P(llrint), P(rint), P(nearbyint), P(frexp), P(ldexp), P(scalbn), P(scalbln), P(modf),
    P(fabs), P(copysign), P(nextafter), P(nexttoward), P(fdim), P(fmax), P(fmin), P(fma),
    P(erf), P(erfc), P(lgamma), P(tgamma), P(nan),
    P(sinf), P(cosf), P(tanf), P(asinf), P(acosf), P(atanf), P(atan2f), P(expf), P(exp2f),
    P(logf), P(log2f), P(log10f), P(powf), P(sqrtf), P(cbrtf), P(hypotf), P(fmodf),
    P(floorf), P(ceilf), P(roundf), P(truncf), P(lroundf), P(fabsf), P(frexpf), P(ldexpf),
    P(modff), P(fmaf), P(fmaxf), P(fminf), P(erff), P(lgammaf), P(tgammaf), P(sinhf),
    P(sinl), P(cosl), P(tanl), P(asinl), P(acosl), P(atanl), P(atan2l), P(expl), P(exp2l),
    P(logl), P(log2l), P(log10l), P(powl), P(sqrtl), P(cbrtl), P(hypotl), P(fmodl),
    P(floorl), P(ceill), P(roundl), P(truncl), P(lroundl), P(fabsl), P(frexpl), P(ldexpl),
    P(modfl), P(fmal), P(erfl), P(lgammal), P(tgammal), P(sinhl), P(coshl), P(tanhl),
    P(csqrt), P(cexp), P(clog), P(cpow), P(cabs), P(carg), P(csin), P(ccos),
    P(feclearexcept), P(fetestexcept), P(fegetround), P(fesetround), P(fegetenv),
    P(fesetenv),
    /* time */
    P(time), P(clock), P(difftime), P(mktime), P(localtime), P(gmtime), P(strftime),
    P(asctime), P(ctime), P(_mkgmtime), P(gettimeofday), P(nanosleep), P(usleep),
    P(clock_gettime), P(localtime_s), P(gmtime_s),
    /* locale, signal, setjmp */
    P(setlocale), P(localeconv), P(signal), P(raise), P(longjmp), P(_setjmp),
    /* io / posix-ish (mingwex, msvcrt) */
    P(_open), P(_close), P(_read), P(_write), P(_lseek), P(_access), P(_stat), P(_fstat),
    P(_mkdir), P(_rmdir), P(_getcwd), P(_chdir), P(_unlink), P(_findfirst), P(_findnext),
    P(_findclose), P(_beginthreadex), P(_endthreadex), P(_getpid), P(_spawnvp),
    P(getopt), P(getopt_long), P(opendir), P(readdir), P(closedir), P(rewinddir),
    P(basename), P(dirname), P(ftruncate), P(truncate), P(sleep),
    /* winpthreads */
    P(pthread_create), P(pthread_join), P(pthread_mutex_init), P(pthread_mutex_lock),
    P(pthread_mutex_unlock), P(pthread_mutex_destroy), P(pthread_cond_init),
    P(pthread_cond_wait), P(pthread_cond_signal), P(pthread_cond_broadcast),
    P(pthread_cond_timedwait), P(pthread_once), P(pthread_key_create), P(pthread_getspecific),
    P(pthread_setspecific), P(pthread_self), P(pthread_detach), P(pthread_rwlock_rdlock),
    P(pthread_rwlock_wrlock), P(pthread_rwlock_unlock), P(pthread_spin_lock), P(pthread_barrier_wait),
    P(sched_yield),
};

/* A handful of real uses so the compiler emits libgcc helper calls too:
 * 64-bit division on i386 (__udivdi3/__divdi3/__moddi3/__umoddi3), popcount,
 * clz/ctz, float<->int conversions, 128-bit math on x64, and __chkstk for a big frame. */
unsigned long long tr_div(unsigned long long a, unsigned long long b) { return a / b + a % b; }
long long tr_sdiv(long long a, long long b) { return a / b - a % b; }
int tr_bits(unsigned long long v) { return __builtin_popcountll(v) + __builtin_clzll(v | 1) + __builtin_ctzll(v | 1) + __builtin_parityll(v); }
double tr_conv(unsigned long long v) { return (double)v + (float)(v >> 3); }
unsigned long long tr_back(double d) { return (unsigned long long)d; }
long double tr_ld(long double x) { return x * 3.0L / 7.0L; }
float _Complex tr_cplx(float _Complex a, float _Complex b) { return a * b / (a + b); }
double _Complex tr_cplxd(double _Complex a, double _Complex b) { return a * b / (a - b); }
#ifdef __x86_64__
__int128 tr_i128(__int128 a, __int128 b) { return a / b * (a % b); }
#endif
int tr_bigframe(int n) { volatile char big[70000]; big[n % 70000] = (char)n; return big[(n * 7) % 70000]; }
int tr_atomic(volatile long *p) { return (int)__sync_fetch_and_add(p, 1) + (int)__atomic_exchange_n(p, 5, __ATOMIC_SEQ_CST); }

static jmp_buf jb;
static void *thr(void *a) { return (void *)((intptr_t)a + 1); }

int main(int argc, char **argv) {
    char buf[256];
    int c, n = 0;
    while ((c = getopt(argc, argv, "ab:")) != -1) n += c;
    snprintf(buf, sizeof buf, "%d %s %.3f %Lf %" PRId64, n, argc > 1 ? argv[1] : "x", sqrt((double)argc),
             tr_ld(argc), (int64_t)tr_div((unsigned)argc + 10u, 3u));
    puts(buf);
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", tm);
    printf("%s %lld %d %g\n", buf, tr_sdiv(argc * 1000LL, 7), tr_bits((unsigned long long)t), tr_conv(t));
    pthread_t th;
    void *res = NULL;
    if (pthread_create(&th, NULL, thr, (void *)(intptr_t)argc) == 0) pthread_join(th, &res);
    if (setjmp(jb) == 0 && argc > 5) longjmp(jb, 1);
    DIR *d = opendir(".");
    if (d) { struct dirent *e; while ((e = readdir(d))) n += (int)strlen(e->d_name); closedir(d); }
    qsort(buf, strlen(buf), 1, (int (*)(const void *, const void *))strcmp);
    return (int)(intptr_t)res + n + (int)tr_back(1.5) + (TABLE[argc & 7] != 0);
}
