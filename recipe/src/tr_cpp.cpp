// SPDX-FileCopyrightText: 2026 Angelos Katis (RADECTI)
// SPDX-License-Identifier: MIT
// TRAINING program (pack build set) -- libstdc++/libsupc++/libgcc_eh coverage.
// Never executed. Its job is to pull as much of the static C++ runtime into the
// image as possible; its own functions (and every template instantiated in THIS
// object) are excluded from the pack by linker-map origin.
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bitset>
#include <charconv>
#include <chrono>
#include <codecvt>
#include <complex>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <valarray>
#include <variant>
#include <vector>

namespace tr {

struct Base { virtual ~Base() = default; virtual int f() const { return 1; } };
struct Mid : virtual Base { int f() const override { return 2; } };
struct Leaf : Mid { int f() const override { return 3; } };

template <class E> static int thrower(int k) {
    try {
        if (k > 0) throw E("x");
    } catch (const std::exception &e) {
        return (int)std::strlen(e.what());
    }
    return 0;
}

static int exceptions(int k) {
    int n = 0;
    n += thrower<std::logic_error>(k) + thrower<std::domain_error>(k) + thrower<std::invalid_argument>(k);
    n += thrower<std::length_error>(k) + thrower<std::out_of_range>(k) + thrower<std::runtime_error>(k);
    n += thrower<std::range_error>(k) + thrower<std::overflow_error>(k) + thrower<std::underflow_error>(k);
    try { throw std::system_error(std::make_error_code(std::errc::invalid_argument), "sys"); }
    catch (const std::system_error &e) { n += e.code().value() + (int)e.code().message().size(); }
    try { throw std::system_error(k, std::system_category()); } catch (...) { n++; }
    try { std::vector<int> v; v.at(k + 10); } catch (const std::out_of_range &) { n++; }
    try { std::string s; s.substr(k + 10); } catch (const std::out_of_range &) { n++; }
    try { std::any a = k; (void)std::any_cast<double>(a); } catch (const std::bad_any_cast &) { n++; }
    try { std::optional<int> o; (void)o.value(); } catch (const std::bad_optional_access &) { n++; }
    try { std::variant<int, double> v = 1.0; (void)std::get<int>(v); } catch (const std::bad_variant_access &) { n++; }
    try { std::function<int()> f; f(); } catch (const std::bad_function_call &) { n++; }
    try { std::shared_ptr<int> sp; std::weak_ptr<int> wp(sp); std::shared_ptr<int> x(wp); } catch (const std::bad_weak_ptr &) { n++; }
    try { Base *b = new Base; Leaf &l = dynamic_cast<Leaf &>(*b); n += l.f(); } catch (const std::bad_cast &) { n++; }
    try { Base *b = nullptr; n += (int)typeid(*b).name()[0]; } catch (const std::bad_typeid &) { n++; }
#if defined(_GLIBCXX_HAS_GTHREADS)
    try { std::promise<int> p; auto f = p.get_future(); auto g = p.get_future(); } catch (const std::future_error &e) { n += e.code().value(); }
#endif
    try { throw 42; } catch (int v) { n += v; }
    try {
        try { throw std::runtime_error("inner"); }
        catch (...) { std::throw_with_nested(std::logic_error("outer")); }
    } catch (const std::exception &e) {
        try { std::rethrow_if_nested(e); } catch (...) { n++; }
    }
    std::exception_ptr ep = std::make_exception_ptr(std::runtime_error("ep"));
    try { std::rethrow_exception(ep); } catch (...) { n += std::uncaught_exceptions(); }
    std::set_terminate([] { std::abort(); });
    try { int *p = new int[(size_t)k * 1000000000000ull]; n += p[0]; } catch (const std::bad_alloc &) { n++; }
    int *q = new (std::nothrow) int[16];
    delete[] q;
    auto *al = new (std::align_val_t(64)) double[8];
    ::operator delete[](al, std::align_val_t(64));
    return n;
}

static std::string strings(const std::string &in) {
    std::string s = in + std::to_string(42) + std::to_string(3.25) + std::to_string(-7LL);
    std::wstring w = std::to_wstring(123) + L"wide";
    s.replace(0, 1, "Q");
    s.insert(2, "ins");
    s.erase(1, 1);
    auto pos = s.find("in");
    auto rpos = s.rfind('4');
    int a = std::stoi("123") + (int)std::stol("-5") + (int)std::stoul("9") + (int)std::stoll("11") + (int)std::stoull("12");
    double d = std::stod("1.5") + std::stof("2.5") + (double)std::stold("3.5");
    std::u16string u16 = u"u16";
    std::u32string u32 = U"u32";
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::string back = conv.to_bytes(w);
    char buf[64];
    auto r = std::to_chars(buf, buf + sizeof buf, 3.14159);
    auto r2 = std::to_chars(buf, buf + sizeof buf, 2.5f, std::chars_format::scientific);
    double parsed = 0;
    std::from_chars(buf, r2.ptr, parsed);
    std::string_view sv(s);
    return s.substr(0, 4) + back + std::string(buf, r.ptr) + std::to_string(a + d + parsed + (double)pos + (double)rpos + (double)u16.size() + (double)u32.size() + (double)sv.find('Q'));
}

static std::string streams(const std::string &path) {
    std::ostringstream os;
    os << std::setw(8) << std::setfill('0') << 42 << ' ' << std::hex << 255 << ' ' << std::dec
       << std::setprecision(10) << 3.14159265358979 << ' ' << std::fixed << 2.5 << ' ' << std::scientific
       << 1e-5 << ' ' << std::boolalpha << true << ' ' << std::quoted("q\"t") << ' ' << 1.5f << ' ' << 7LL << ' ' << 7ULL;
    std::time_t t = std::time(nullptr);
    os << std::put_time(std::localtime(&t), "%Y-%m-%d");
    std::istringstream is("12 3.5 word 0x1f");
    int i; double d; std::string w; int h;
    is >> i >> d >> w >> std::hex >> h;
    std::wostringstream wos;
    wos << L"w" << 3.5 << 12;
    std::ofstream of(path, std::ios::binary);
    of << os.str() << '\n';
    of.close();
    std::ifstream in(path);
    std::string line, all;
    while (std::getline(in, line)) all += line;
    std::fstream fs(path, std::ios::in | std::ios::out);
    fs.seekg(0, std::ios::end);
    std::stringstream ss;
    ss << in.rdbuf();
    std::cout << os.str() << std::endl;
    std::cerr << "err " << i << d << w << h << std::endl;
    std::clog << "log" << std::flush;
    std::wcout << wos.str() << std::endl;
    std::cout.imbue(std::locale::classic());
    std::locale loc;
    const auto &ct = std::use_facet<std::ctype<char>>(loc);
    char up = ct.toupper('a');
    const auto &np = std::use_facet<std::numpunct<char>>(loc);
    std::ios_base::sync_with_stdio(false);
    return all + up + np.decimal_point() + ss.str() + (char)(std::cin.peek() & 0x7f);
}

static long containers(int n) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 1);
    std::mt19937 rng(n);
    std::shuffle(v.begin(), v.end(), rng);
    std::sort(v.begin(), v.end());
    std::stable_sort(v.begin(), v.end(), std::greater<int>());
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    std::deque<std::string> dq = {"a", "b"};
    dq.push_front("z");
    std::list<int> li(v.begin(), v.end());
    li.sort();
    li.unique();
    li.reverse();
    std::forward_list<int> fl = {3, 1, 2};
    fl.sort();
    std::map<std::string, int> m;
    std::multimap<int, int> mm;
    std::set<long> st;
    std::multiset<double> ms;
    std::unordered_map<std::string, std::vector<int>> um;
    std::unordered_set<long long> us;
    for (int i = 0; i < n; i++) {
        m[std::to_string(i)] = i;
        mm.emplace(i % 3, i);
        st.insert(i * 7L);
        ms.insert(i * 0.5);
        um["k" + std::to_string(i % 5)].push_back(i);
        us.insert(i * 11LL);
    }
    m.erase("3");
    st.erase(st.begin());
    std::priority_queue<int> pq(v.begin(), v.end());
    std::stack<int> sk;
    sk.push(1);
    std::bitset<128> bs(0xdeadbeefULL);
    std::array<int, 4> ar = {4, 3, 2, 1};
    std::valarray<double> va(1.5, 8);
    std::complex<double> c(1, 2);
    c = std::sqrt(c) * std::exp(c);
    std::uniform_int_distribution<int> ud(0, 100);
    std::normal_distribution<double> nd(0, 1);
    std::random_device rd;
    std::pmr::monotonic_buffer_resource mbr;
    std::pmr::vector<int> pv(&mbr);
    pv.push_back(1);
#if defined(_GLIBCXX_HAS_GTHREADS)
    std::pmr::synchronized_pool_resource spr;
#else
    std::pmr::unsynchronized_pool_resource spr;
#endif
    std::pmr::unsynchronized_pool_resource upr; (void)upr.allocate(8);
    std::pmr::string ps("pmr", &spr);
    auto sp = std::make_shared<std::vector<int>>(v);
    std::weak_ptr<std::vector<int>> wp = sp;
    auto up = std::make_unique<std::map<int, int>>();
    (*up)[1] = 2;
    std::hash<std::string> hs;
    return (long)(m.size() + mm.size() + st.size() + ms.size() + um.size() + us.size() + pq.top() + sk.top() +
                  bs.count() + bs.to_string().size() + ar[0] + va.sum() + c.real() + ud(rng) + nd(rng) + rd() +
                  pv.size() + ps.size() + wp.use_count() + up->size() + hs("x") + dq.size() + li.size());
}

#if defined(_GLIBCXX_HAS_GTHREADS)
static int concurrency(int n) {
    std::atomic<int> a{0};
    std::mutex mu;
    std::recursive_mutex rmu;
    std::timed_mutex tmu;
    std::shared_mutex shm;
    std::condition_variable cv;
    std::condition_variable_any cva;
    bool ready = false;
    std::thread t([&] {
        std::lock_guard<std::mutex> g(mu);
        ready = true;
        a += n;
        cv.notify_all();
    });
    {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait(lk, [&] { return ready; });
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    t.join();
    { std::lock_guard<std::recursive_mutex> g(rmu); }
    if (tmu.try_lock_for(std::chrono::milliseconds(1))) tmu.unlock();
    { std::shared_lock<std::shared_mutex> s(shm); }
    std::once_flag of;
    std::call_once(of, [&] { a++; });
    auto fut = std::async(std::launch::async, [n] { return n * 2; });
    std::promise<int> pr;
    auto pf = pr.get_future();
    pr.set_value(3);
    std::packaged_task<int(int)> pt([](int x) { return x + 1; });
    auto ptf = pt.get_future();
    pt(4);
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    std::this_thread::yield();
    auto now = std::chrono::system_clock::now();
    auto st = std::chrono::steady_clock::now();
    auto hr = std::chrono::high_resolution_clock::now();
    (void)cva;
    return a + fut.get() + pf.get() + ptf.get() + (int)std::thread::hardware_concurrency() +
           (int)(now.time_since_epoch().count() & 1) + (int)(st.time_since_epoch().count() & 1) +
           (int)(hr.time_since_epoch().count() & 1);
}
#else
static int concurrency(int n) { return n; }
#endif

static long filesystem(const std::string &root) {
    namespace fs = std::filesystem;
    fs::path p = fs::path(root) / "sub" / "file.txt";
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    long n = (long)p.filename().string().size() + (long)p.extension().string().size() + (long)p.stem().string().size();
    n += fs::exists(p, ec) + fs::is_directory(p.parent_path(), ec) + fs::is_regular_file(p, ec);
    for (auto &e : fs::directory_iterator(root, ec)) n += (long)e.path().native().size();
    for (auto &e : fs::recursive_directory_iterator(root, ec)) n += (long)e.is_directory();
    n += (long)fs::current_path(ec).native().size() + (long)fs::temp_directory_path(ec).native().size();
    n += (long)fs::absolute(p, ec).native().size() + (long)fs::canonical(root, ec).native().size();
    n += (long)fs::file_size(p, ec) + (long)fs::space(root, ec).free;
    fs::copy_file(p, p.string() + ".bak", fs::copy_options::overwrite_existing, ec);
    fs::rename(p, p.string() + ".2", ec);
    fs::remove(p, ec);
    n += (long)fs::remove_all(fs::path(root) / "sub", ec);
    n += (long)fs::last_write_time(root, ec).time_since_epoch().count();
    n += (long)p.lexically_normal().native().size() + (long)p.relative_path().native().size();
    return n;
}

}  // namespace tr

int main(int argc, char **argv) {
    std::string arg = argc > 1 ? argv[1] : "demo";
    long r = tr::exceptions(argc) + tr::containers(argc * 10) + tr::concurrency(argc);
    std::string s = tr::strings(arg) + tr::streams(arg + ".txt");
    r += tr::filesystem(arg);
    std::vector<std::unique_ptr<tr::Base>> objs;
    objs.push_back(std::make_unique<tr::Leaf>());
    objs.push_back(std::make_unique<tr::Mid>());
    for (auto &o : objs) r += o->f() + (long)typeid(*o).hash_code();
    std::cout << s.size() + r << std::endl;
    return (int)(r & 0x7f);
}
