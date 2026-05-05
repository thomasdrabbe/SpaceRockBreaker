#include <SFML/Graphics.hpp>
#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace fs = std::filesystem;

static std::wstring exeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf);
    const auto pos = p.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return L".";
    return p.substr(0, pos);
}

static std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
        s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t'))
        ++i;
    return s.substr(i);
}

static std::string readVersionFile(const fs::path& filePath) {
    std::ifstream in(filePath);
    std::string   line;
    if (!std::getline(in, line))
        return "0.0.0";
    return trim(line);
}

static std::string readLocalVersion(const fs::path& dir) {
    return readVersionFile(dir / "version.txt");
}

static std::tuple<int, int, int> semverTuple(const std::string& raw) {
    std::string s = trim(raw);
    const auto cut = std::min({ s.find(' '), s.find('\t'),
                                s.find('+'), s.find('-') });
    if (cut != std::string::npos)
        s.resize(cut);

    int major = -1;
    int minor = -1;
    int patch = -1;

    auto readInt = [&](const std::string& t, size_t i0,
                       size_t& outEnd) -> int {
        if (i0 >= t.size() || !std::isdigit(static_cast<unsigned char>(t[i0])))
            return -1;
        size_t i = i0;
        int    v = 0;
        for (; i < t.size(); ++i) {
            const unsigned char c =
                static_cast<unsigned char>(t[i]);
            if (!std::isdigit(c))
                break;
            v = v * 10 + static_cast<int>(c - '0');
            if (v > 1'000'000)
                return -1;
        }
        outEnd = i;
        return v;
    };

    size_t i       = 0;
    major          = readInt(s, i, i);
    if (major < 0 || i >= s.size() || s[i++] != '.')
        return { -1, -1, -1 };
    minor        = readInt(s, i, i);
    if (minor < 0 || i >= s.size() || s[i++] != '.')
        return { -1, -1, -1 };
    patch       = readInt(s, i, i);
    if (patch < 0)
        return { -1, -1, -1 };
    if (i != s.size())
        return { -1, -1, -1 };

    return { major, minor, patch };
}

static bool semverParsed(const std::tuple<int, int, int>& t) {
    return std::get<0>(t) >= 0;
}

static int compareSemver(const std::tuple<int, int, int>& a,
                         const std::tuple<int, int, int>& b) {
    if (!semverParsed(a) || !semverParsed(b))
        return 0;

    auto ta = std::tie(std::get<0>(a), std::get<1>(a), std::get<2>(a));
    auto tb = std::tie(std::get<0>(b), std::get<1>(b), std::get<2>(b));
    if (ta < tb)
        return -1;
    if (ta > tb)
        return 1;
    return 0;
}

static void logLine(const std::string& msg) {
    const fs::path log = fs::path(exeDir()) / "launcher_log.txt";
    std::ofstream  out(log, std::ios::app);
    out << msg << '\n';
}

static bool runPowerShellScript(const std::wstring& script) {
    const fs::path scriptFile =
        fs::temp_directory_path()
        / ("srb_ps_" + std::to_string(GetCurrentProcessId())
           + "_" + std::to_string(GetTickCount64()) + ".ps1");
    {
        std::wofstream out(scriptFile);
        if (!out)
            return false;
        out << script;
    }

    std::wstring cmd =
        L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"";
    cmd += scriptFile.wstring();
    cmd += L"\"";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(L'\0');
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &si, &pi))
        return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    std::error_code ec;
    fs::remove(scriptFile, ec);
    return code == 0;
}

static bool psDownloadToFile(const std::wstring& url, const fs::path& destFile) {
    auto quoteSingle = [](const std::wstring& s) {
        std::wstring out;
        out.reserve(s.size() + 8);
        out.push_back(L'\'');
        for (wchar_t c : s) {
            if (c == L'\'')
                out += L"''";
            else
                out.push_back(c);
        }
        out.push_back(L'\'');
        return out;
    };
    const std::wstring dst = destFile.wstring();
    std::wstring script =
        L"$ErrorActionPreference='Stop'; "
        L"$ProgressPreference='SilentlyContinue'; "
        L"Invoke-WebRequest -UseBasicParsing -Uri ";
    script += quoteSingle(url);
    script += L" -OutFile ";
    script += quoteSingle(dst);
    script += L";";
    return runPowerShellScript(script);
}

static bool winHttpDownload(const std::wstring& host,
                            const std::wstring& urlPath,
                            INTERNET_PORT      port,
                            bool               https,
                            const fs::path&    destFile,
                            std::atomic<std::uint64_t>* downloadedBytes,
                            std::atomic<std::uint64_t>* totalBytes) {
    HINTERNET ses = WinHttpOpen(L"SpaceRockLauncher/1.0",
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME,
                                WINHTTP_NO_PROXY_BYPASS,
                                0);
    if (!ses)
        return false;
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(ses, WINHTTP_OPTION_REDIRECT_POLICY, &redirect,
                       sizeof(redirect));

    HINTERNET conn = WinHttpConnect(ses, host.c_str(), port, 0);
    if (!conn) {
        WinHttpCloseHandle(ses);
        return false;
    }

    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET req = WinHttpOpenRequest(
        conn, L"GET", urlPath.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!req) {
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(req, nullptr)) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    DWORD status = 0;
    DWORD sz     = sizeof(status);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz,
                        WINHTTP_NO_HEADER_INDEX);
    if (status != 200) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    DWORD contentLen = 0;
    DWORD clSize     = sizeof(contentLen);
    if (WinHttpQueryHeaders(req,
                            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &contentLen, &clSize,
                            WINHTTP_NO_HEADER_INDEX))
        totalBytes->store(contentLen);
    else
        totalBytes->store(0);

    std::ofstream out(destFile, std::ios::binary);
    if (!out) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    std::uint64_t got = 0;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(req, &avail))
            break;
        if (avail == 0)
            break;
        std::vector<char> chunk(avail);
        DWORD read = 0;
        if (!WinHttpReadData(req, chunk.data(), avail, &read) || read == 0)
            break;
        out.write(chunk.data(), static_cast<std::streamsize>(read));
        got += read;
        downloadedBytes->store(got);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(conn);
    WinHttpCloseHandle(ses);
    return out.good();
}

static std::string httpGetText(const std::wstring& host,
                               const std::wstring& urlPath,
                               bool                https) {
    const fs::path tmp =
        fs::temp_directory_path()
        / ("srb_ver_" + std::to_string(GetCurrentProcessId()) + ".txt");
    std::atomic<std::uint64_t> down{};
    std::atomic<std::uint64_t> tot{};
    if (!winHttpDownload(host, urlPath, https ? INTERNET_DEFAULT_HTTPS_PORT
                                             : INTERNET_DEFAULT_HTTP_PORT,
                         https, tmp, &down, &tot)) {
        const std::wstring url = (https ? L"https://" : L"http://") + host + urlPath;
        if (!psDownloadToFile(url, tmp))
            return {};
    }
    std::ifstream in(tmp, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    fs::remove(tmp);
    return trim(ss.str());
}

static std::string fetchRemoteVersionText() {
    const std::wstring cacheBust = L"?ts=" + std::to_wstring(GetTickCount64());

    // 1) Primair: raw + cachebust
    std::string v = httpGetText(
        L"raw.githubusercontent.com",
        LR"(/thomasdrabbe/SpaceRockBreaker/main/version.txt)" + cacheBust,
        true);
    if (!v.empty())
        return v;

    // 2) raw zonder query
    v = httpGetText(
        L"raw.githubusercontent.com",
        LR"(/thomasdrabbe/SpaceRockBreaker/main/version.txt)",
        true);
    if (!v.empty())
        return v;

    // 3) github raw redirect endpoint
    v = httpGetText(
        L"github.com",
        LR"(/thomasdrabbe/SpaceRockBreaker/raw/main/version.txt)",
        true);
    if (!v.empty())
        return v;

    // 4) jsDelivr mirror van GitHub (extra fallback)
    v = httpGetText(
        L"cdn.jsdelivr.net",
        LR"(/gh/thomasdrabbe/SpaceRockBreaker@main/version.txt)" + cacheBust,
        true);
    if (!v.empty())
        return v;

    // 5) jsDelivr zonder query
    v = httpGetText(
        L"cdn.jsdelivr.net",
        LR"(/gh/thomasdrabbe/SpaceRockBreaker@main/version.txt)",
        true);
    return v;
}

static bool runExpandArchive(const fs::path& zip, const fs::path& dest) {
    std::wstring cmd =
        L"powershell.exe -NoProfile -NonInteractive -Command "
        L"\"$ErrorActionPreference='Stop'; Expand-Archive -LiteralPath '";
    cmd += zip.wstring();
    cmd += L"' -DestinationPath '";
    cmd += dest.wstring();
    cmd += L"' -Force\"";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(L'\0');
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &si, &pi))
        return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

static bool launchDetachedCmd(const std::wstring& cmdLine,
                              const std::wstring& workingDir) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back(L'\0');
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, workingDir.c_str(), &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

static bool scheduleApplyAndStart(const fs::path& installDir,
                                  const fs::path& stageDir,
                                  const fs::path& startExe,
                                  DWORD           launcherPid) {
    const fs::path script =
        fs::temp_directory_path()
        / ("srb_apply_update_" + std::to_string(GetCurrentProcessId()) + ".cmd");

    std::ofstream out(script);
    if (!out)
        return false;

    out << "@echo off\r\n";
    out << "setlocal\r\n";
    out << "set \"SRC=" << stageDir.string() << "\"\r\n";
    out << "set \"DST=" << installDir.string() << "\"\r\n";
    out << "set \"START_EXE=" << startExe.string() << "\"\r\n";
    out << "set \"LPID=" << launcherPid << "\"\r\n";
    out << ":waitlauncher\r\n";
    out << "tasklist /FI \"PID eq %LPID%\" | find \"%LPID%\" >nul\r\n";
    out << "if not errorlevel 1 (\r\n";
    out << "  timeout /t 1 /nobreak >nul\r\n";
    out << "  goto waitlauncher\r\n";
    out << ")\r\n";
    out << "robocopy \"%SRC%\" \"%DST%\" /E /IS /IT /R:2 /W:1 /NFL /NDL /NJH /NJS /NP >nul\r\n";
    out << "start \"\" \"%START_EXE%\"\r\n";
    out << "rmdir /S /Q \"%SRC%\" >nul 2>&1\r\n";
    out << "del \"%~f0\" >nul 2>&1\r\n";
    out.close();

    const std::wstring cmd = L"cmd.exe /C \"" + script.wstring() + L"\"";
    return launchDetachedCmd(cmd, installDir.wstring());
}

static void startGame(const fs::path& exe) {
    std::wstring p = exe.wstring();
    std::vector<wchar_t> args(p.begin(), p.end());
    args.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    CreateProcessW(exe.c_str(), args.data(), nullptr, nullptr, FALSE, 0,
                   nullptr, exeDir().c_str(), &si, &pi);
    if (pi.hThread)
        CloseHandle(pi.hThread);
    if (pi.hProcess)
        CloseHandle(pi.hProcess);
}

int main() {
    const fs::path dir     = exeDir();
    const fs::path gameExe = dir / "SpaceRockBreaker.exe";
    if (!fs::exists(gameExe))
        return 0;

    const std::string localVer = readLocalVersion(dir);
    std::string       remoteVer;
    try {
        remoteVer = fetchRemoteVersionText();
    } catch (...) {
        remoteVer.clear();
    }

    const auto locT = semverTuple(localVer);
    const auto remT = semverTuple(remoteVer);
    const bool remoteAvailable = !remoteVer.empty();
    const bool upToDate = remoteAvailable
        && semverParsed(locT) && semverParsed(remT)
        && compareSemver(remT, locT) <= 0;

    // State 1: up-to-date -> meteen starten zonder venster.
    if (upToDate) {
        startGame(gameExe);
        return 0;
    }

    // State 3: offline -> korte melding en daarna starten zonder update.
    if (!remoteAvailable) {
        sf::RenderWindow w(
            sf::VideoMode(sf::Vector2u{ 480u, 120u }),
            sf::String{ L"Space Rock Breaker — Launcher" },
            sf::Style::None);
        w.setFramerateLimit(60);

        sf::Font font;
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
            (void)font.openFromFile("C:/Windows/Fonts/segoeui.ttf");

        sf::Clock closeAfter;
        while (w.isOpen()) {
            while (const std::optional ev = w.pollEvent()) {
                if (ev->is<sf::Event::Closed>())
                    w.close();
            }
            if (closeAfter.getElapsedTime().asSeconds() >= 2.f)
                w.close();

            w.clear(sf::Color(12, 14, 28));

            sf::Text t1(font);
            t1.setCharacterSize(18);
            t1.setStyle(sf::Text::Bold);
            t1.setString("Kan geen verbinding maken");
            t1.setFillColor(sf::Color::White);
            t1.setPosition({ 20.f, 28.f });
            w.draw(t1);

            sf::Text t2(font);
            t2.setCharacterSize(14);
            t2.setString("Game wordt gestart zonder update.");
            t2.setFillColor(sf::Color(180, 190, 210));
            t2.setPosition({ 20.f, 64.f });
            w.draw(t2);

            w.display();
        }
        startGame(gameExe);
        return 0;
    }

    // State 2: update beschikbaar -> compact progress-venster.
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u{ 480u, 180u }),
        sf::String{ L"Space Rock Breaker — Launcher" },
        sf::Style::None);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
        (void)font.openFromFile("C:/Windows/Fonts/segoeui.ttf");

    std::atomic<std::uint64_t> downloaded{ 0 };
    std::atomic<std::uint64_t> total{ 0 };
    std::atomic<int>           status{ 0 }; // 0=running, 2=staged, <0 fail
    std::string                logLineUi = "Verbinding maken met Github...";

    const fs::path zipPath =
        fs::temp_directory_path()
        / ("SpaceRockBreaker_update_" + std::to_string(GetCurrentProcessId())
           + ".zip");

    std::thread worker;
    bool        workerRunning = true;
    bool        workerJoined  = false;
    fs::path    stagedDir;

    worker = std::thread([&]() {
        bool ok = false;
        downloaded.store(0);
        total.store(0);

        const std::wstring zipUrlA =
            L"https://raw.githubusercontent.com/thomasdrabbe/SpaceRockBreaker/main/installer_output/SpaceRockBreaker.zip?ts="
            + std::to_wstring(GetTickCount64());
        const std::wstring zipPathA =
            LR"(/thomasdrabbe/SpaceRockBreaker/main/installer_output/SpaceRockBreaker.zip?ts=)"
            + std::to_wstring(GetTickCount64());
        ok = winHttpDownload(
            L"raw.githubusercontent.com",
            zipPathA,
            INTERNET_DEFAULT_HTTPS_PORT, true, zipPath, &downloaded, &total);
        if (!ok)
            ok = psDownloadToFile(zipUrlA, zipPath);

        if (!ok) {
            downloaded.store(0);
            total.store(0);
            const std::wstring zipUrlB =
                L"https://raw.githubusercontent.com/thomasdrabbe/SpaceRockBreaker/main/SpaceRockBreaker.zip?ts="
                + std::to_wstring(GetTickCount64());
            const std::wstring zipPathB =
                LR"(/thomasdrabbe/SpaceRockBreaker/main/SpaceRockBreaker.zip?ts=)"
                + std::to_wstring(GetTickCount64());
            ok = winHttpDownload(
                L"raw.githubusercontent.com",
                zipPathB,
                INTERNET_DEFAULT_HTTPS_PORT, true, zipPath, &downloaded, &total);
            if (!ok)
                ok = psDownloadToFile(zipUrlB, zipPath);
        }

        if (!ok) {
            status.store(-1);
            return;
        }

        const fs::path stage =
            fs::temp_directory_path()
            / ("srb_stage_" + std::to_string(GetCurrentProcessId())
               + "_" + std::to_string(GetTickCount64()));
        std::error_code ec;
        fs::create_directories(stage, ec);
        if (ec || !runExpandArchive(zipPath, stage)) {
            status.store(-2);
            fs::remove(zipPath);
            return;
        }

        const std::string installedVer = readLocalVersion(dir);
        const std::string packageVer   = readVersionFile(stage / "version.txt");
        const auto        ilocT        = semverTuple(installedVer);
        const auto        pkgT         = semverTuple(packageVer);
        if (semverParsed(ilocT) && semverParsed(pkgT)
            && compareSemver(pkgT, ilocT) <= 0) {
            fs::remove_all(stage);
            fs::remove(zipPath);
            status.store(-4);
            return;
        }

        stagedDir = stage;
        fs::remove(zipPath);
        status.store(2);
    });

    while (window.isOpen()) {
        while (const std::optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                if (workerRunning && worker.joinable()) {
                    worker.detach();
                    workerJoined  = true;
                    workerRunning = false;
                }
                window.close();
            }
        }

        if (workerRunning) {
            const int st = status.load();
            if (st != 0 && !workerJoined) {
                worker.join();
                workerJoined  = true;
                workerRunning = false;
                if (st == 2) {
                    if (!scheduleApplyAndStart(
                            dir, stagedDir, gameExe, GetCurrentProcessId())) {
                        startGame(gameExe);
                        return 0;
                    }
                    return 0;
                } else {
                    startGame(gameExe);
                    return 0;
                }
            }
        }

        const std::uint64_t t = total.load();
        const std::uint64_t d = downloaded.load();
        float p = 0.f;
        if (t > 0)
            p = std::min(
                1.f, static_cast<float>(
                    static_cast<double>(d) / static_cast<double>(t)));

        std::ostringstream status;
        status << "Update wordt gedownload... ("
               << std::fixed << std::setprecision(1)
               << (static_cast<double>(d) / (1024.0 * 1024.0)) << " MB)";

        window.clear(sf::Color(12, 14, 28));

        sf::Text title(font);
        title.setCharacterSize(22);
        title.setStyle(sf::Text::Bold);
        title.setString("SPACE ROCK BREAKER");
        title.setFillColor(sf::Color::White);
        const auto tb = title.getLocalBounds();
        title.setPosition({
            240.f - (tb.position.x + tb.size.x * 0.5f),
            30.f - tb.position.y });
        window.draw(title);

        sf::Text ver(font);
        ver.setCharacterSize(13);
        ver.setString("v" + localVer + " -> v" + remoteVer);
        ver.setFillColor(sf::Color(140, 150, 180));
        const auto vb = ver.getLocalBounds();
        ver.setPosition({
            240.f - (vb.position.x + vb.size.x * 0.5f),
            60.f - vb.position.y });
        window.draw(ver);

        sf::Text st(font);
        st.setCharacterSize(13);
        st.setString(status.str());
        st.setFillColor(sf::Color(180, 190, 210));
        const auto sb = st.getLocalBounds();
        st.setPosition({
            240.f - (sb.position.x + sb.size.x * 0.5f),
            92.f - sb.position.y });
        window.draw(st);

        sf::RectangleShape track(sf::Vector2f{ 440.f, 8.f });
        track.setPosition({ 20.f, 114.f });
        track.setFillColor(sf::Color(26, 28, 46));
        window.draw(track);

        sf::RectangleShape fill(sf::Vector2f{ p * 440.f, 8.f });
        fill.setPosition({ 20.f, 114.f });
        fill.setFillColor(sf::Color(0, 120, 255));
        window.draw(fill);

        sf::Text pct(font);
        pct.setCharacterSize(12);
        pct.setString(std::to_string(static_cast<int>(p * 100.f + 0.5f)) + "%");
        pct.setFillColor(sf::Color(140, 150, 180));
        const auto pb = pct.getLocalBounds();
        pct.setPosition({ 460.f - pb.size.x - pb.position.x, 126.f - pb.position.y });
        window.draw(pct);

        sf::Text lg(font);
        lg.setCharacterSize(11);
        lg.setString(logLineUi);
        lg.setFillColor(sf::Color(80, 90, 110));
        const auto lb = lg.getLocalBounds();
        lg.setPosition({
            240.f - (lb.position.x + lb.size.x * 0.5f),
            154.f - lb.position.y });
        window.draw(lg);

        window.display();
    }

    if (!workerJoined && worker.joinable())
        worker.join();
    return 0;
}
